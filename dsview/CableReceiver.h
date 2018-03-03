#pragma once

#include <cstdint>
#include <thread>
#include <atomic>
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

struct ReaderData {
    ReaderData() : terminate(false), readBuf(0x100000), error(false) {}

    boost::lockfree::spsc_queue<uint8_t> readBuf;
    std::atomic_bool terminate;
    std::atomic_bool error;
};

class DSReciever : public ICableReceiver
{
public:
	DSReciever(Image& _top_screen, Image& _bot_screen, boost::lockfree::spsc_queue<float>& _audio_buffer);
	~DSReciever();
protected:
	void fetchData() override;
    static void readerThread(ReaderData *rdata);

    ReaderData rdata;
    std::thread reader;
};
