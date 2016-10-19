#pragma once

#include <cstdint>
#include <thread>

#include "TRingbuffer.h"
#include "Buffer.h"
#include "Image.h"
#include "ftd2xx_wrap.h"
#include "Config.h"

class ICableReceiver
{
    public:
        ICableReceiver();
        void Receive(Image& top_screen, Image& bottom_screen, TRingbuffer<float>& audio_buffer);
		void Stop();
		bool HasStopped() const;
    protected:
        std::vector<float> audio_target_data;
        TRingbuffer<uint8_t> rbuf;
		Buffer<uint8_t> dbuf;
        volatile bool shutdown;
		volatile bool hasStopped;
};

class DummyReceiver : public ICableReceiver
{
    public:
        DummyReceiver();
		~DummyReceiver();
    protected:
        static void receiverThreadHandler(TRingbuffer<uint8_t> *rbuf, volatile bool *shutdown, volatile bool *is_shutdown);
        std::thread receiver_thread;
};

class DSReciever : public ICableReceiver
{
public:
	DSReciever();
	~DSReciever();
protected:
	static void receiverThreadHandler(TRingbuffer<uint8_t> *rbuf, volatile bool *shutdown, volatile bool *is_shutdown,
		Ftd2xxDevice *usb_device);
	std::thread *receiver_thread;
	Ftd2xxDevice *usb_device;

};