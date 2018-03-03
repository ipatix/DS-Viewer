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

using namespace std;

// interface

ICableReceiver::ICableReceiver(Image& _top_screen, Image& _bot_screen, boost::lockfree::spsc_queue<float>& _audio_buffer)
    : top_screen(_top_screen)
    , bot_screen(_bot_screen)
    , audio_buffer(_audio_buffer)
    , data_stream(0x20000)
    , leftHPFilter(bq_type_highpass, 10.f / float(AUDIO_SAMPLERATE), 0.707f, 0)
    , rightHPFilter(bq_type_highpass, 10.f / float(AUDIO_SAMPLERATE), 0.707f, 0)
    , leftLPFilter(bq_type_lowpass, 15000.f / float(AUDIO_SAMPLERATE), 0.707f, 0)
    , rightLPFilter(bq_type_lowpass, 15000.f / float(AUDIO_SAMPLERATE), 0.707f, 0)
{
}

void ICableReceiver::Receive()
{
    uint32_t col_top = 0, row_top = 0;
    uint32_t col_bot = 0, row_bot = 0;
    audio_target_data.clear();
    assert(top_screen.Width() == bot_screen.Width());
    assert(top_screen.Height() == bot_screen.Height());

    MediaFrame fr;
    uint8_t* fr_ptr = (uint8_t*)&fr;
    static_assert(sizeof(MediaFrame) == 4, "Media Frames must be 4 byte big");
    
    while (data_stream.DataCount() < sizeof(fr)) {
        fetchData();
    }
    data_stream.Take((uint8_t*)&fr, sizeof(fr));

    int bad_count = 0;

    while (true) {
        //fprintf(stderr, "%02x:%02x:%02x:%02x\n", fr_ptr[0], fr_ptr[1], fr_ptr[2], fr_ptr[3]);
        if (fr.IsValid()) {
            if (fr.IsVideo()) {
                if (fr.IsVSync()) {
                    break;
                }

                if (fr.IsTopScr()) {
                    top_screen(row_top, col_top++) = fr.pframe.GetColor();
                    if (col_top >= top_screen.Width()) {
                        col_top = 0;
                        if (row_top == top_screen.Height() - 1)
                            row_top = 0;
                        else
                            row_top++;
                    }
                } else {
                    bot_screen(row_bot, col_bot++) = fr.pframe.GetColor();
                    if (col_bot >= top_screen.Width()) {
                        col_bot = 0;
                        if (row_bot == top_screen.Height() - 1)
                            row_bot = 0;
                        else
                            row_bot++;
                    }
                }
            } else {
                float l, r;
                fr.aframe.GetAudio(r, l);

                audio_target_data.push_back(l);
                audio_target_data.push_back(r);

                if (audio_target_data.size() >= 2400) {
                    //cout << "Early breaking...\n";
                    //cout << "Audio count: " << audio_target_data.size() / 2 << endl;
                    break;
                }
            }
            while (data_stream.DataCount() < sizeof(fr)) {
                fetchData();
            }
            data_stream.Take((uint8_t*)&fr, sizeof(fr));
        } else {
            //fprintf(stderr, "Error: broken frame %02x:%02x:%02x:%02x\n", fr_ptr[0], fr_ptr[1], fr_ptr[2], fr_ptr[3]);
            fr_ptr[0] = fr_ptr[1];
            fr_ptr[1] = fr_ptr[2];
            fr_ptr[2] = fr_ptr[3];
            while (data_stream.DataCount() < sizeof(fr_ptr[3])) {
                fetchData();
            }
            data_stream.Take(fr_ptr + 3, 1);
            bad_count++;
        }
    }
    if (bad_count > 0)
        printf("Bad count: %d\n", bad_count);

    for (size_t i = 0; i < audio_target_data.size(); i += 2) {
        audio_target_data[i] = leftHPFilter.Process(audio_target_data[i]);
        audio_target_data[i + 1] = rightHPFilter.Process(audio_target_data[i + 1]);
    }

    audio_buffer.push(audio_target_data.data(), audio_target_data.size());
}
/*
 * DSReceiver
 */

DSReciever::DSReciever(Image& _top_screen, Image& _bot_screen, boost::lockfree::spsc_queue<float>& _audio_buffer)
    : ICableReceiver::ICableReceiver(_top_screen, _bot_screen, _audio_buffer)
    , reader(readerThread, &rdata)
{
#ifdef __linux__
    pthread_setname_np(reader.native_handle(), "usb reader");
#endif
}

DSReciever::~DSReciever()
{
    rdata.terminate.store(true);
    reader.join();
}

void DSReciever::fetchData()
{
    if (rdata.error.load()) {
        throw Xcept("Error on USB reader thread");
    }
    uint8_t buffer[0x10000];
    size_t read = rdata.readBuf.pop(buffer, sizeof(buffer));
    if (read == 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }
    data_stream.Put(buffer, read);
}

void DSReciever::readerThread(ReaderData *rdata)
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
#endif
        usb_device->set_latency_timer(2);
        usb_device->data_set_chunksize(0x10000, 0x10000);
        usb_device->set_timeouts(10, 10);

        atomic_thread_fence(std::memory_order_acquire);
        while (!rdata->terminate.load()) {
            uint8_t buffer[0x10000];
            size_t read = static_cast<size_t>(usb_device->read_data(buffer, sizeof(buffer)));
            //printf("read count: %zx\n", read);
            size_t written = rdata->readBuf.push(buffer, read);
            if (written != read)
                printf("warning, buffer overflow\n");
            atomic_thread_fence(std::memory_order_acquire);
        }

        usb_device->usb_close();
        delete usb_device;
    }
    catch (const std::exception& e) {
        fprintf(stderr, "Fatal Eror:\n%s", e.what());
        rdata->error.store(1);
    }
    printf("reader thread quit\n");
}
