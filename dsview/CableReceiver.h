#pragma once

#include <cstdint>
#include <thread>

#include "TRingbuffer.h"
#include "Buffer.h"
#include "Image.h"

#define VIDEO_WIDTH 256
#define VIDEO_HEIGHT 192

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
