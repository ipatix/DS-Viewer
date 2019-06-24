#pragma once

#include <vector>
#include <cstdint>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <boost/lockfree/spsc_queue.hpp>

#include "Ringbuffer.h"
#include "Image.h"
#include "Config.h"
#include "BiQuad.h"
#include "ftdipp.h"
#include "MediaFrame.h"

#define IMAGE_QUEUE_LEN 2

/*
 * This is supposed to be an interface class to implement multiple different
 * hardware devices which might be supported later
 */

class ICableReceiver
{
public:
    virtual void LockFrame(const void **top_screen, const void **bot_screen) = 0;
    virtual void UnlockFrame() = 0;
protected:
    ICableReceiver(boost::lockfree::spsc_queue<stereo_sample>& audio_buffer);
    boost::lockfree::spsc_queue<stereo_sample>& audio_buffer;
};

class DSCableReceiver : public ICableReceiver
{
public:
	DSCableReceiver(boost::lockfree::spsc_queue<stereo_sample>& audio_buffer);
    ~DSCableReceiver();

    void LockFrame(const void **top_screen, const void **bot_screen) override;
    void UnlockFrame() override;
protected:
    static void readerThread(DSCableReceiver *_this);
    static void decoderThread(DSCableReceiver *_this);

    struct image_buffer {
        image_buffer() : top(NDS_H, NDS_W), bot(NDS_H, NDS_W) {}
        Image top;
        Image bot;
    };
    std::vector<image_buffer> image_buffers;
    std::mutex display_index_mutex;
    std::unique_lock<std::mutex> display_index_lock;
    std::condition_variable display_index_cv;
    size_t display_index, decoder_index;
    size_t last_display_index;

    BiQuad leftHPFilter;
    BiQuad rightHPFilter;

    boost::lockfree::spsc_queue<uint8_t> read_buf;
    std::vector<MediaFrame> decoder_cache;
    std::vector<stereo_sample> audio_cache;
    std::atomic_bool terminate;
    std::atomic_bool reader_error;
    std::atomic_bool decoder_error;

    std::thread reader;
    std::thread decoder;
};
