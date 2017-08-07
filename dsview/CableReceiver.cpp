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
                fr.aframe.GetAudio(l, r);

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
        cout << "Bad count: " << bad_count << endl;

    for (size_t i = 0; i < audio_target_data.size(); i += 2) {
        audio_target_data[i] = leftLPFilter.Process(leftHPFilter.Process(audio_target_data[i]));
        audio_target_data[i + 1] = rightLPFilter.Process(rightHPFilter.Process(audio_target_data[i + 1]));
    }
    audio_buffer.push(audio_target_data.data(), audio_target_data.size());
}
/*
 * DSReceiver
 */

DSReciever::DSReciever(Image& _top_screen, Image& _bot_screen, boost::lockfree::spsc_queue<float>& _audio_buffer)
    : ICableReceiver::ICableReceiver(_top_screen, _bot_screen, _audio_buffer)
    , terminate(false)
    , readBuf(0x100000)
    , reader(readerThread, &readBuf, &terminate)
{
#ifdef __linux__
    pthread_setname_np(reader.native_handle(), "usb reader");
#endif
}

DSReciever::~DSReciever()
{
    terminate = true;
    atomic_thread_fence(std::memory_order_release);
    reader.join();
}

void DSReciever::fetchData()
{
    uint8_t buffer[0x10000];
    size_t read = readBuf.pop(buffer, sizeof(buffer));
    if (read == 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }
    data_stream.Put(buffer, read);
}

void DSReciever::readerThread(
    boost::lockfree::spsc_queue<uint8_t>* readBuf,
    volatile bool* terminate)
{
    printf("reader thread started\n");

#ifdef __linux__
    errno = 0;
    nice(-20);
    if (errno != 0)
        printf("Warning, couldn't set nice to -20\n");
#endif

    ftdi_device* usb_device = new ftdi_device();
    //if ((usb_device->GetDeviceInfoList().at(0).Flags & 2) == 0)
    //    throw Xcept("ERROR: FT232 running in USB 1.1 mode, not in USB 2.0 mode");
    usb_device->usb_open(0x0403, 0x6014);
    usb_device->set_bitmode(0xFF, BITMODE_RESET);
    usb_device->set_bitmode(0xFF, BITMODE_SYNCFF);
    usb_device->set_latency_timer(2);
    usb_device->read_data_set_chunksize(0x10000);
    usb_device->write_data_set_chunksize(0x10000);
    usb_device->set_flowctrl(SIO_RTS_CTS_HS);
    usb_device->usb_purge_buffers();
    usb_device->get_context()->usb_read_timeout = 10;
    usb_device->get_context()->usb_write_timeout = 10;

    while (!*terminate) {
        uint8_t buffer[0x10000];
        size_t read = static_cast<size_t>(usb_device->read_data(buffer, sizeof(buffer)));
        //printf("read count: %zx\n", read);
        size_t written = readBuf->push(buffer, read);
        if (written != read)
            printf("warning, buffer overflow\n");
    }

    usb_device->usb_close();
    delete usb_device;
    printf("reader thread quit\n");
}
