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

ICableReceiver::ICableReceiver(Image& _top_screen, Image& _bot_screen, boost::lockfree::spsc_queue<float>& _audio_buffer)
    : top_screen(_top_screen), bot_screen(_bot_screen), audio_buffer(_audio_buffer),
	data_stream(0x20000),
	leftHPFilter(bq_type_highpass, 10.f / float(AUDIO_SAMPLERATE), 0.707f, 0),
	rightHPFilter(bq_type_highpass, 10.f / float(AUDIO_SAMPLERATE), 0.707f, 0),
	leftLPFilter(bq_type_lowpass, 15000.f / float(AUDIO_SAMPLERATE), 0.707f, 0),
	rightLPFilter(bq_type_lowpass, 15000.f / float(AUDIO_SAMPLERATE), 0.707f, 0) 
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
	uint8_t *fr_ptr = (uint8_t *)&fr;
    static_assert(sizeof(MediaFrame) == 4, "Media Frames must be 4 byte big");

	while (data_stream.DataCount() < sizeof(fr)) {
		fetchData();
	}
	data_stream.Take((uint8_t *)&fr, sizeof(fr));

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
                    bot_screen(row_bot, col_bot++) = fr.pframe.GetColor();
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
                fr.aframe.GetAudio(r, l);

				audio_target_data.push_back(l);
				audio_target_data.push_back(r);

				if (audio_target_data.size() >= 2400) {
					cout << "Early breaking...\n";
					cout << "Audio count: " << audio_target_data.size() / 2 << endl;
					break;
				}
            }
			while (data_stream.DataCount() < sizeof(fr)) {
				fetchData();
			}
            data_stream.Take((uint8_t *)&fr, sizeof(fr));
        }
        else
        {
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

	for (size_t i = 0; i < audio_target_data.size(); i += 2)
	{
        /*
		audio_target_data[i] = leftLPFilter.Process(leftHPFilter.Process(audio_target_data[i]));
		audio_target_data[i+1] = rightLPFilter.Process(rightHPFilter.Process(audio_target_data[i+1]));
        */
        audio_target_data[i] = leftLPFilter.Process(audio_target_data[i]);
        audio_target_data[i + 1] = rightLPFilter.Process(audio_target_data[i + 1]);
	}
    audio_buffer.push(audio_target_data.data(), audio_target_data.size());
}

/*
 * DSReceiver
 */

DSReciever::DSReciever(Image& _top_screen, Image& _bot_screen, boost::lockfree::spsc_queue<float>& _audio_buffer)
	: ICableReceiver::ICableReceiver(_top_screen, _bot_screen, _audio_buffer)
{
	usb_device = new Ftd2xxDevice();
	if ((usb_device->GetDeviceInfoList().at(0).Flags & 2) == 0)
		throw Xcept("ERROR: FT232 running in USB 1.1 mode, not in USB 2.0 mode");
	usb_device->SetBitMode(0xFF, 0x40);
	usb_device->SetLatencyTimer(2);
	usb_device->SetUSBParameters(0x10000, 0x10000);
	usb_device->SetFlowControl(FT_FLOW_RTS_CTS, 0, 0);
	usb_device->Purge(FT_PURGE_RX | FT_PURGE_TX);
	usb_device->SetTimeouts(50, 50);
}

DSReciever::~DSReciever()
{
	usb_device->Close();
	delete usb_device;
}

void DSReciever::fetchData()
{
	uint8_t buffer[0x10000];
	size_t read = usb_device->Read(buffer, sizeof(buffer));
	data_stream.Put(buffer, read);
}











