#include <cstring>
#include <cassert>
#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <iostream>

#include "CableReceiver.h"
#include "Xcept.h"

using namespace std;

// interface

ICableReceiver::ICableReceiver()
    : rbuf(0x100000, true)
{
    shutdown = false;
    cur_col = 0;
    cur_row = 0;
}

/*
 * The Cable Protocol begins with this 4 byte magic block:
 * - 0xAA 0x55 0xAA 0x55
 * 
 * The amount of pixel sets (2 top, 2 bot pixels each) (4 bytes LE) of the video block follows (MSB = flags)
 * - 0xXXYYYYYY
 * - YYYYYY pixel sets, 12 bytes each
 *
 * Then the amount of audio sample pairs (1 pair = stereo, 3 bytes LE). 
 * - 0xXXXX
 * - 0xLLLRRRR
 *
 * After the data, zeroes are sent until the start frame is recieved
 *
 */

void ICableReceiver::Receive(Image& top_screen, Image& bottom_screen, Ringbuffer<float>& audio_buffer)
{
    if (hasStopped)
        return;
    assert(top_screen.Width() == bottom_screen.Width());
    assert(top_screen.Height() == bottom_screen.Height());
    do
    {
        // read until magic block is reached
        for (size_t i = 0; i < 4;) 
        {
            uint8_t check = (i & 1) ? 0x55 : 0xAA;
            uint8_t input;
			//printf("Taking and checking protocol init bytes: %llu\n", size_t(1));
            rbuf.Take(&input, 1);
            (input == check) ? i++ : i = 0;
        }

        // process video
        uint8_t video_num_sets_bytes[4];
		//printf("Taking video length bytes: %llu\n", sizeof(video_num_sets_bytes));
        rbuf.Take(video_num_sets_bytes, sizeof(video_num_sets_bytes));
        size_t video_num_sets = video_num_sets_bytes[0] | (video_num_sets_bytes[1] << 8) |
            (video_num_sets_bytes[2] << 16);
        uint8_t flags = video_num_sets_bytes[3];

        // 4 pixels, 18 bits per pixel
		video_data.resize(video_num_sets * (4 * 18 / 8));

		//printf("Taking video data bytes: %llu\n", video_data.size());
        rbuf.Take(video_data.data(), video_data.size());

        if (flags & FLAG_VSYNC)
        {
            cur_col = 0;
            cur_row = 0;
        }

        for (size_t i = 0, block_offset = 0; i < video_num_sets; i++, block_offset += 9)
        {
            uint128_t pixel_set = 0;
            pixel_set = uint128_t(video_data[block_offset + 0]) |
                (uint128_t(video_data[block_offset + 1]) << uint128_t(8) ) |
                (uint128_t(video_data[block_offset + 2]) << uint128_t(16)) |
                (uint128_t(video_data[block_offset + 3]) << uint128_t(24)) |
                (uint128_t(video_data[block_offset + 4]) << uint128_t(32)) |
                (uint128_t(video_data[block_offset + 5]) << uint128_t(40)) |
                (uint128_t(video_data[block_offset + 6]) << uint128_t(48)) |
                (uint128_t(video_data[block_offset + 7]) << uint128_t(56)) |
                (uint128_t(video_data[block_offset + 8]) << uint128_t(64));
            Color tl, bl, tr, br;
            BlockToCol(tl, bl, tr, br, pixel_set);
            top_screen(cur_row, cur_col) = tl;
            top_screen(cur_row, cur_col + 1) = tr;
            bottom_screen(cur_row, cur_col) = bl;
            bottom_screen(cur_row, cur_col + 1) = br;

            cur_col += 2;
            if (cur_col >= top_screen.Width())
            {
                cur_col = 0;
                cur_row += 1;
                if (cur_row >= top_screen.Height())
                    cur_row = 0;
            }
        }

        // read audio data

        uint8_t audio_num_sets_bytes[2];
		//printf("Taking audio length bytes: %llu\n", sizeof(audio_num_sets_bytes));
        rbuf.Take(audio_num_sets_bytes, sizeof(audio_num_sets_bytes));
		size_t audio_num_sets = audio_num_sets_bytes[0] | (audio_num_sets_bytes[1] << 8);

		audio_data.resize(audio_num_sets * 3);
		//printf("Taking audio data bytes: %llu\n", audio_data.size());
        rbuf.Take(audio_data.data(), audio_data.size());

		audio_target_data.resize(audio_num_sets * 2);

		// convert from packed ints to float buffer
        for (size_t i = 0, block_offset = 0; i < audio_num_sets; i++, block_offset += 3)
        {
            uint32_t audio_set = 
                audio_data[block_offset + 0] |
                (audio_data[block_offset + 1] << 8) |
                (audio_data[block_offset + 2] << 16);
            int32_t left = int32_t(audio_set & 0xFFF) - 0x800;
            int32_t right = int32_t(audio_set >> 12) - 0x800;
            audio_target_data[i*2] = float(left) / float(0x800);
            audio_target_data[i*2+1] = float(right) / float(0x800);
        }

        audio_buffer.Put(audio_target_data.data(), audio_target_data.size());
    } while (cur_col > 0 || cur_row > 0);
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

void DummyReceiver::receiverThreadHandler(Ringbuffer<uint8_t> *rbuf, volatile bool *shutdown, volatile bool *is_shutdown)
{
	std::cout << "thread started" << endl;
    uint32_t col = 0, row = 0;
    float p_left = 0.f, p_right = 0.f;
    vector<uint8_t> v_data;
    vector<uint8_t> a_data;
	uint16_t status = 0;
    while (!*shutdown)
    {
        // send protocol begin: 0xAA 0x55 0xAA 0x55
        uint8_t p_begin[4] = {0xAA,0x55,0xAA,0x55};
		//printf("Putting protocol init data: %llu\n", size_t(4));
        rbuf->Put(p_begin, sizeof(p_begin));

        // send video
        uint32_t v_sets = 48 * VIDEO_WIDTH / 2; // send a quarter frame of two screens
        uint8_t v_len_bytes[4];
        v_len_bytes[0] = uint8_t(v_sets);
        v_len_bytes[1] = uint8_t(v_sets >> 8);
        v_len_bytes[2] = uint8_t(v_sets >> 16);
        v_len_bytes[3] = (!col && !row) ? FLAG_VSYNC : 0;

		//printf("Putting video length bytes: %llu\n", sizeof(v_len_bytes));
        rbuf->Put(v_len_bytes, sizeof(v_len_bytes));

		v_data.resize(v_sets * 9);

		//printf("sending row: %llu\n", size_t(row));
        for (size_t i = 0; i < 48; i++)
        {
            for (size_t j = 0; j < (VIDEO_WIDTH / 2); j++)
            {
                // very basic color fade
				Color c;
				if (row & 1)
					c = Color(0, 255, 0);
				else
					c = Color(0, 0, 255);
                uint128_t set = ColToBlock(c, c, c, c);
                size_t index = (i * (VIDEO_WIDTH / 2) + j) * 9;
                v_data[index] = uint8_t(set);
                v_data[index+1] = uint8_t(set >> uint128_t(8));
                v_data[index+2] = uint8_t(set >> uint128_t(16));
                v_data[index+3] = uint8_t(set >> uint128_t(24));
                v_data[index+4] = uint8_t(set >> uint128_t(32));
                v_data[index+5] = uint8_t(set >> uint128_t(40));
                v_data[index+6] = uint8_t(set >> uint128_t(48));
                v_data[index+7] = uint8_t(set >> uint128_t(56));
                v_data[index+8] = uint8_t(set >> uint128_t(64));

                if ((col += 2) >= VIDEO_WIDTH) 
                    col = 0;
            }
            if (++row >= VIDEO_HEIGHT) 
                row = 0;
        }

		//printf("Putting video data bytes: %llu\n", v_data.size());
        rbuf->Put(v_data.data(), v_data.size());

        // send audio

        size_t a_count = 200;
        uint8_t a_len_bytes[2];
        a_len_bytes[0] = uint8_t(a_count);
        a_len_bytes[1] = uint8_t(a_count >> 8);

		//printf("Putting audio length bytes: %llu\n", sizeof(a_len_bytes));
        rbuf->Put(a_len_bytes, sizeof(a_len_bytes));

		a_data.resize(a_count * 3);// 200 samples per quarter frame, stereo samples 12 bit each

        for (size_t i = 0; i < a_count; i++)
        {
            size_t index = i*3;
            uint32_t l = uint32_t(int((p_left += 0.01f) * float(0x800) * 0.03f) + 0x800);
            uint32_t r = uint32_t(int((p_right += 0.01005f) * float(0x800) * 0.03f) + 0x800);
			if (p_left > 1.f) p_left -= 2.f;
			if (p_right > 1.f) p_right -= 2.f;
            uint32_t d = (l & 0xFFF) | ((r & 0xFFF) << 12);
            a_data[index+0] = uint8_t(d);
            a_data[index+1] = uint8_t(d >> 8);
            a_data[index+2] = uint8_t(d >> 16);
        }

		//printf("Putting audio data bytes: %llu\n", a_data.size());
        rbuf->Put(a_data.data(), a_data.size());

        // send some random zero bytes

        uint8_t zeroes[1746];
        for (size_t i = 0; i < sizeof(zeroes); i++)
            zeroes[i] = 0;

		//printf("Putting zeroes: %llu\n", sizeof(zeroes));
        rbuf->Put(zeroes, sizeof(zeroes));
    }
	*is_shutdown = true;
	cout << "Thread stopped" << endl;
}