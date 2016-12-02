#include <cstring>
#include <cassert>
#include <cstdio>
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
    : rbuf(0x200000, true, true), dbuf(0x10000, rbuf), 
	leftHPFilter(bq_type_highpass, 10.f / float(AUDIO_SAMPLERATE), 0.707f, 0),
	rightHPFilter(bq_type_highpass, 10.f / float(AUDIO_SAMPLERATE), 0.707f, 0),
	leftLPFilter(bq_type_lowpass, 15000.f / float(AUDIO_SAMPLERATE), 0.707f, 0),
	rightLPFilter(bq_type_lowpass, 15000.f / float(AUDIO_SAMPLERATE), 0.707f, 0)
{
    shutdown = false;
}

void ICableReceiver::Receive(Image& top_screen, Image& bottom_screen, TRingbuffer<float>& audio_buffer)
{
    uint32_t col_top = 0, row_top = 0;
    uint32_t col_bot = 0, row_bot = 0;
    audio_target_data.clear();
    if (hasStopped)
        return;
    assert(top_screen.Width() == bottom_screen.Width());
    assert(top_screen.Height() == bottom_screen.Height());


    MediaFrame fr;
	uint8_t *fr_ptr = (uint8_t *)&fr;
    static_assert(sizeof(MediaFrame) == 4, "Media Frames must be 4 byte big");
    dbuf.Take((uint8_t *)&fr, sizeof(fr));

	int bad_count = 0;

    while (true) {
		//fprintf(stderr, "%02x:%02x:%02x:%02x\n", fr_ptr[0], fr_ptr[1], fr_ptr[2], fr_ptr[3]);
        if (fr.IsValid())
        {
            if (fr.IsVideo())
            {
				if (fr.IsVSync()) {
					//cout << "Recieved VSync\n";
					break;
				}

                if (fr.IsTopScr())
                {
                    top_screen(row_top, col_top++) = fr.pframe.GetColor();
                    if (col_top >= top_screen.Width())
                    {
                        col_top = 0;
                        if (row_top == top_screen.Height() - 1)
                            row_top = 0;
                        else 
                            row_top++;
                    }
                }
                else
                {
                    bottom_screen(row_bot, col_bot++) = fr.pframe.GetColor();
                    if (col_bot >= top_screen.Width())
                    {
                        col_bot = 0;
                        if (row_bot == top_screen.Height() - 1)
                            row_bot = 0;
                        else 
                            row_bot++;
                    }
                }
            }
            else
            {
                float l, r;
                fr.aframe.GetAudio(l, r);

				audio_target_data.push_back(l);
				audio_target_data.push_back(r);

				if (audio_target_data.size() >= 2400) {
					cout << "Early breaking...\n";
					cout << "Audio count: " << audio_target_data.size() / 2 << endl;
					break;
				}
            }
            dbuf.Take((uint8_t *)&fr, sizeof(fr));
        }
        else
        {
            //fprintf(stderr, "Error: broken frame %02x:%02x:%02x:%02x\n", fr_ptr[0], fr_ptr[1], fr_ptr[2], fr_ptr[3]);
            fr_ptr[0] = fr_ptr[1];
			fr_ptr[1] = fr_ptr[2];
			fr_ptr[2] = fr_ptr[3];
            dbuf.Take(fr_ptr + 3, 1);
			bad_count++;
        }
    }
	if (bad_count > 0)
		cout << "Bad count: " << bad_count << endl;

	for (size_t i = 0; i < audio_target_data.size(); i += 2)
	{
		audio_target_data[i] = leftLPFilter.Process(leftHPFilter.Process(audio_target_data[i]));
		audio_target_data[i+1] = rightLPFilter.Process(rightHPFilter.Process(audio_target_data[i+1]));
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

/*
 * DSReceiver
 */

DSReciever::DSReciever()
{
	usb_device = new Ftd2xxDevice();
	if ((usb_device->GetDeviceInfoList().at(0).Flags & 2) == 0)
		throw Xcept("ERROR: FT232 running in USB 1.1 mode, not in USB 2.0 mode");
	usb_device->SetBitMode(0xFF, 0x40);
	usb_device->SetLatencyTimer(2);
	usb_device->SetUSBParameters(0x10000, 0x10000);
	usb_device->SetFlowControl(FT_FLOW_RTS_CTS, 0, 0);
	usb_device->Purge(FT_PURGE_RX | FT_PURGE_TX);
	usb_device->SetTimeouts(1000, 1000);
	receiver_thread = new std::thread(DSReciever::receiverThreadHandler, &rbuf, &shutdown, &hasStopped, usb_device);
	hasStopped = false;
}

DSReciever::~DSReciever()
{
	receiver_thread->join();
	delete usb_device;
	delete receiver_thread;
}

void DSReciever::receiverThreadHandler(TRingbuffer<uint8_t>* rbuf, volatile bool *shutdown, volatile bool *is_shutdown,
	Ftd2xxDevice *usb_device)
{
	cout << "Started Thread..." << endl;
	std::vector<uint8_t> cable_buf(0x10000, 0);
	try
	{
		while (!*shutdown)
		{
			//cout << "Reading Block..." << endl;
			size_t read = usb_device->Read(cable_buf.data(), cable_buf.size());
			std::fill(cable_buf.begin() + long(read), cable_buf.end(), 0);
			if (read != cable_buf.size())
				cerr << "Warning: USB Read Data timeout" << endl;
			rbuf->Put(cable_buf.data(), cable_buf.size());
		}
	} 
	catch (const exception& e)
	{
		cerr << "An error occured while running: " << e.what() << endl;
	}
	cout << "Closing USB connection..." << endl;
	usb_device->Close();
	*is_shutdown = true;
	cout << "Shutdown!" << endl;
}












