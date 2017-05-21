#pragma once

#include <cstdint>
#include <thread>
#include <boost/lockfree/spsc_queue.hpp>

#include "Ringbuffer.h"
#include "Image.h"
#include "Config.h"
#include "BiQuad.h"
#include "ftdipp.h"

class ICableReceiver
{
    public:
        ICableReceiver(Image& _top_screen, Image& _bot_screen, boost::lockfree::spsc_queue<float>& _audio_buffer);
        void Receive();
    protected:
		virtual void fetchData() = 0;

        std::vector<float> audio_target_data;
		Image& top_screen;
		Image& bot_screen;
		boost::lockfree::spsc_queue<float>& audio_buffer;
		Ringbuffer<uint8_t> data_stream;
		BiQuad leftHPFilter;
		BiQuad rightHPFilter;
		BiQuad leftLPFilter;
		BiQuad rightLPFilter;
};

class DSReciever : public ICableReceiver
{
public:
	DSReciever(Image& _top_screen, Image& _bot_screen, boost::lockfree::spsc_queue<float>& _audio_buffer);
	~DSReciever();
protected:
	void fetchData() override;

	ftdi_device *usb_device;
};
