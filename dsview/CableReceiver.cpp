#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#ifdef __linux__
#include <sys/syscall.h>
#include <unistd.h>
#endif

#include <chrono>
#include <cmath>
#include <iostream>

#include "CableReceiver.h"
#include "MediaFrame.h"
#include "Xcept.h"

static const size_t READ_BLOCK_SIZE = 0x10000;

// interface

ICableReceiver::ICableReceiver(boost::lockfree::spsc_queue<stereo_sample>& audio_buffer)
    : audio_buffer(audio_buffer)
{
}

/*
* DSReceiver
*/

DSCableReceiver::DSCableReceiver(boost::lockfree::spsc_queue<stereo_sample>& audio_buffer)
    : ICableReceiver(audio_buffer)
    , image_buffers(IMAGE_QUEUE_LEN)
    , display_index(1)
    , decoder_index(0)
    , leftHPFilter(BQ_TYPE_HIGHPASS, 10.0f / float(AUDIO_SAMPLERATE), 0.707f, 0)
    , rightHPFilter(BQ_TYPE_HIGHPASS, 10.0f / float(AUDIO_SAMPLERATE), 0.707f, 0)
    , read_buf(0x200000)
    , decoder_cache(READ_BLOCK_SIZE / sizeof(MediaFrame))
    , audio_cache(512)
    , terminate(false)
    , reader_error(false)
    , decoder_error(false)
    , reader(readerThread, this)
    , decoder(decoderThread, this)
{
#ifdef __linux__
    pthread_setname_np(reader.native_handle(), "usb reader");
    pthread_setname_np(decoder.native_handle(), "decoder");
#endif
}

DSCableReceiver::~DSCableReceiver()
{
    terminate.store(true);
    reader.join();
    decoder.join();
}

void DSCableReceiver::LockFrame(const void **top_screen, const void **bot_screen)
{
    display_index_mutex.lock();
    *top_screen = image_buffers[display_index].top.getData();
    *bot_screen = image_buffers[display_index].bot.getData();
    return;
}

void DSCableReceiver::UnlockFrame()
{
    display_index_mutex.unlock();
}

void DSCableReceiver::decoderThread(DSCableReceiver *_this)
{
    for (size_t i = 0; i < IMAGE_QUEUE_LEN; i++) {
        assert(_this->image_buffers[i].top.Height() == _this->image_buffers[i].bot.Height());
    }
    static_assert(READ_BLOCK_SIZE % sizeof(MediaFrame) == 0, "READ_BLOCK_SIZE must be a multiple of sizeof(MediaFrame)");
    static_assert(sizeof(MediaFrame) == 4, "Media Frames must be 4 byte big");
    static_assert(sizeof(Color) == 3, "sizeof(Color) must be 3");

    Color *top_color = static_cast<Color *>(_this->image_buffers[_this->decoder_index].top.getData());
    Color *bot_color = static_cast<Color *>(_this->image_buffers[_this->decoder_index].bot.getData());

    static const int left_blank = 4;
    static const int right_blank = 3;

    int top_col = 0;
    int bot_col = 0;
    int top_row = 0;
    int bot_row = 0;

    size_t audio_index = 0;

    while (!_this->terminate.load()) {
        if (_this->read_buf.read_available() < READ_BLOCK_SIZE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        _this->read_buf.pop(reinterpret_cast<uint8_t *>(_this->decoder_cache.data()), READ_BLOCK_SIZE);

        for (const MediaFrame& mf : _this->decoder_cache) {
            if (!mf.IsValid()) {
                // try to realign the buffer by one byte incase there is a bad byte
                // don't care about dropping this 64k chunk because it'll usually stay in sync and this
                // is just a fraction of a frame
                _this->read_buf.pop();
                static int bad_count = 0;
                fprintf(stderr, "Bad frame detected: %d\n", bad_count++);
                break;
            }
            if (mf.IsVideo()) {
                // video frame
                if (mf.IsVSync()) {
                    top_col = 0;
                    bot_col = 0;
                    top_row = 0;
                    bot_row = 0;
                    // read frame successfully, now send off frame to viewer
                    _this->display_index_mutex.lock();
                    _this->decoder_index += 1;
                    if (_this->decoder_index >= IMAGE_QUEUE_LEN)
                        _this->decoder_index = 0;
                    _this->display_index += 1;
                    if (_this->display_index >= IMAGE_QUEUE_LEN)
                        _this->display_index = 0;
                    _this->display_index_mutex.unlock();
                    top_color = static_cast<Color *>(_this->image_buffers[_this->decoder_index].top.getData());
                    bot_color = static_cast<Color *>(_this->image_buffers[_this->decoder_index].bot.getData());
                }
                else if (mf.IsTopScr()) {
                    if (top_col >= left_blank && top_col < (NDS_W + left_blank) && top_row < NDS_H)
                        *top_color++ = mf.pframe.GetColor();
                    top_col += 1;
                    if (top_col >= left_blank + NDS_W + right_blank) {
                        top_col = 0;
                        top_row += 1;
                    }
                }
                else {
                    if (bot_col >= left_blank && top_col < (NDS_W + left_blank) && bot_row < NDS_H)
                        *bot_color++ = mf.pframe.GetColor();
                    bot_col += 1;
                    if (bot_col >= left_blank + NDS_W + right_blank) {
                        bot_col = 0;
                        bot_row += 1;
                    }
                }
            }
            else {
                // audio frame
                stereo_sample sample;
                mf.aframe.GetAudio(sample.l, sample.r);
                _this->audio_cache[audio_index++] = sample;
                if (audio_index >= _this->audio_cache.size()) {
                    audio_index = 0;
                    _this->audio_buffer.push(_this->audio_cache.data(), _this->audio_cache.size());
                }
            }
        }
    }
}

void DSCableReceiver::readerThread(DSCableReceiver *_this)
{
    printf("reader thread started\n");
    try {
#ifdef __linux__
        errno = 0;
        nice(-20);
        if (errno != 0)
            printf("Warning, couldn't set nice to -20\n");
#endif
        ftdi_device* usb_device = new ftdi_device();
#ifdef __unix__
        usb_device->usb_open(0x0403, 0x6014);
        usb_device->set_bitmode(0xFF, BITMODE_RESET);
        usb_device->set_bitmode(0xFF, BITMODE_SYNCFF);
        usb_device->set_flowctrl(SIO_RTS_CTS_HS, 0, 0);
        usb_device->usb_purge_buffers();
#elif _WIN32
        usb_device->usb_open();
        usb_device->set_bitmode(0xFF, FT_BITMODE_RESET);
        usb_device->set_bitmode(0xFF, FT_BITMODE_SYNC_FIFO);
        usb_device->set_flowctrl(FT_FLOW_RTS_CTS, 0, 0);
        usb_device->usb_purge_buffers(FT_PURGE_RX | FT_PURGE_TX);
#else
#error "Invalid Architecture"
#endif
        usb_device->set_latency_timer(2);
        usb_device->data_set_chunksize(0x10000, 0x10000);
        usb_device->set_timeouts(10, 10);

        //auto tstart = std::chrono::high_resolution_clock::now();

        atomic_thread_fence(std::memory_order_acquire);
        while (!_this->terminate.load()) {
            uint8_t buffer[0x10000];
            //auto start = std::chrono::high_resolution_clock::now();
            size_t read = static_cast<size_t>(usb_device->read_data(buffer, sizeof(buffer)));
            //auto end = std::chrono::high_resolution_clock::now();
            //printf("issues read at %f, took %f milliseconds\n",
            //        std::chrono::duration_cast<std::chrono::nanoseconds>(start - tstart).count() / 1e6f,
            //        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1e6f);
            //printf("read count: %zx\n", read);
            size_t written = _this->read_buf.push(buffer, read);
            if (written != read)
                printf("warning, buffer overflow\n");
            atomic_thread_fence(std::memory_order_acquire);
        }

        usb_device->usb_close();
        delete usb_device;
    }
    catch (const std::exception& e) {
        fprintf(stderr, "Fatal Eror:\n%s", e.what());
        _this->decoder_error.store(1);
    }
    printf("reader thread quit\n");
}
