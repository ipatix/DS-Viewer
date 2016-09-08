#pragma once

#include <cstdint>
#include <thread>

#include "Ringbuffer.h"
#include "Image.h"

#define FLAG_VSYNC 0x80
#define VIDEO_WIDTH 256
#define VIDEO_HEIGHT 192

class ICableReceiver
{
    public:
        ICableReceiver();
        void Receive(Image& top_screen, Image& bottom_screen, Ringbuffer<float>& audio_buffer);
		void Stop();
		bool HasStopped() const;
    protected:
        std::vector<uint8_t> video_data;
        std::vector<uint8_t> audio_data;
        std::vector<float> audio_target_data;
        Ringbuffer<uint8_t> rbuf; 
        volatile bool shutdown;
		volatile bool hasStopped;
        uint32_t cur_col, cur_row;
};

class DummyReceiver : public ICableReceiver
{
    public:
        DummyReceiver();
		~DummyReceiver();
    protected:
        static void receiverThreadHandler(Ringbuffer<uint8_t> *rbuf, volatile bool *shutdown, volatile bool *is_shutdown);
        std::thread receiver_thread;
};
