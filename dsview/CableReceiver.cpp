#include <cstring>
#include <cassert>
#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <iostream>

#include "CableReceiver.h"
#include "Xcept.h"
#include "MediaFrame.h"

using namespace std;

// interface

ICableReceiver::ICableReceiver()
    : rbuf(0x100000, true), dbuf(0x10000, rbuf)
{
    shutdown = false;
}

void ICableReceiver::Receive(Image& top_screen, Image& bottom_screen, TRingbuffer<float>& audio_buffer)
{
    uint32_t cur_col = 0, cur_row = 0;
    audio_target_data.clear();
    if (hasStopped)
        return;
    assert(top_screen.Width() == bottom_screen.Width());
    assert(top_screen.Height() == bottom_screen.Height());


    MediaFrame fr;
    static_assert(sizeof(MediaFrame) == 4, "Media Frames must be 4 byte big");
    dbuf.Take((uint8_t *)&fr, sizeof(fr));

    while (true) {
        if (fr.IsValid())
        {
            if (fr.IsVideo())
            {
                if (fr.IsVSync())
                    break;

                    if (fr.IsTopScr())
                    {
                        top_screen(cur_row, cur_col) = fr.pframe.GetColor();
                    }
                    else
                    {
                        bottom_screen(cur_row, cur_col++) = fr.pframe.GetColor();
                        if (cur_col >= 256)
                        {
                            cur_col = 0;
                            if (cur_row == 191)
                                cur_row = 0;
                            else 
                                cur_row++;
                        }
                    }
            }
            else
            {
                float l, r;
                fr.aframe.GetAudio(l, r);
                if (audio_target_data.size() >= 3200)
                {
                    audio_buffer.Put(audio_target_data.data(), audio_target_data.size());
                    audio_target_data.clear();
                }
                audio_target_data.push_back(l);
                audio_target_data.push_back(r);
            }
            dbuf.Take((uint8_t *)&fr, sizeof(fr));
        }
        else
        {
            cerr << "Warning, empty frame detected on stream!" << endl;
            uint8_t *fr_ptr = (uint8_t *)&fr;
            fr_ptr[0] = fr_ptr[1];
            fr_ptr[1] = fr_ptr[2];
            fr_ptr[2] = fr_ptr[3];
            dbuf.Take(fr_ptr + 3, 1);
        }
    }

    audio_buffer.Put(audio_target_data.data(), audio_target_data.size());
}

void ICableReceiver::Stop()
{
    shutdown = true;
}

bool ICableReceiver::HasStopped() const
{
    return hasStopped;
}

// dummy receiver

DummyReceiver::DummyReceiver()
    : receiver_thread(DummyReceiver::receiverThreadHandler, &rbuf, &shutdown, &hasStopped)
{
    hasStopped = false;
}

DummyReceiver::~DummyReceiver()
{
    receiver_thread.join();
}

void DummyReceiver::receiverThreadHandler(TRingbuffer<uint8_t> *rbuf, volatile bool *shutdown, volatile bool *is_shutdown)
{
    std::cout << "thread started" << endl;
    float p_left = 0.f, p_right = 0.f;
    vector<uint8_t> v_data;
    vector<uint8_t> a_data;
    while (!*shutdown)
    {
        v_data.clear();
        a_data.clear();


        // send video
        for (size_t i = 0; i < VIDEO_HEIGHT; i++)
        {
            for (size_t j = 0; j < VIDEO_WIDTH; j++)
            {
                // very basic color fade
                Color c;
                if ((i & 1) | (j & 1))
                    c = Color(0, 0, 0);
                else
                    c = Color(255, 255, 255);
                v_data.push_back(0b11000000);
                v_data.push_back(c.r);
                v_data.push_back(c.g);
                v_data.push_back(c.b);
                v_data.push_back(0b11000010);
                v_data.push_back(c.r);
                v_data.push_back(c.g);
                v_data.push_back(c.b);
            }
        }

        //printf("Putting video data bytes: %llu\n", v_data.size());
        rbuf->Put(v_data.data(), v_data.size());

        // send vsync
        uint8_t p_begin[4] = { 0b11000001,0x00,0x00,0x00 };
        rbuf->Put(p_begin, sizeof(p_begin));

        // send audio
        for (size_t i = 0; i < 800; i++)
        {
            uint32_t l = uint32_t(int((p_left += 0.01f) * float(0x800) * 0.03f) + 0x800);
            uint32_t r = uint32_t(int((p_right += 0.01005f) * float(0x800) * 0.03f) + 0x800);
            if (p_left > 1.f) p_left -= 2.f;
            if (p_right > 1.f) p_right -= 2.f;
            a_data.push_back(0b10000000);
            a_data.push_back(uint8_t(l >> 4));
            a_data.push_back(uint8_t(l << 4) | uint8_t(r >> 8));
            a_data.push_back(uint8_t(r));
        }

        //printf("Putting audio data bytes: %llu\n", a_data.size());
        rbuf->Put(a_data.data(), a_data.size());

        // send some random zero bytes

        //uint8_t zeroes[400];
        //for (size_t i = 0; i < sizeof(zeroes); i++)
        //    zeroes[i] = uint8_t(0);

        //printf("Putting zeroes: %llu\n", sizeof(zeroes));
        //rbuf->Put(zeroes, sizeof(zeroes));
    }
    *is_shutdown = true;
    cout << "Thread stopped" << endl;
}
