#pragma once

#include <cstdint>
#include <cstdio>

#include "Image.h"

struct MAudioFrame
{
	void GetAudio(float& fL, float& fR) 
	{
		int left = ((lr & 0xF) << 8) | r;
		left -= 0x800;
		int right = (l << 4) | (lr >> 4); 
		right -= 0x800;
		fL = float(left) / float(0x800);
		fR = float(right) / float(0x800);
	}

	uint8_t flags;
	uint8_t l;
	uint8_t lr;
	uint8_t r;
};

struct MPixelFrame
{
	Color GetColor() { return Color(uint8_t(r << 2), uint8_t(g << 2), uint8_t(b << 2)); }

	uint8_t flags;
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct MediaFrame
{
	union
	{
		uint8_t flags;
		MAudioFrame aframe;
		MPixelFrame pframe;
	};

	bool IsValid()  { return flags & 0b10000000 ? true : false; }
	bool IsVideo()  { return flags & 0b01000000 ? true : false; }
	bool IsAudio()  { return flags & 0b01000000 ? false : true; }
	bool IsVSync()  { return flags & 0b00000001 ? true : false; }
	bool IsTopScr() { return flags & 0b00000010 ? true : false; }
};

/*
* The Cable Protocol works continous:
*
* Each Frame is 4 bytes big. It may either be a an audio or video frame:
* - Video:
*		VSYNC SIG:   0b11000001, 0x00, 0x00, 0x00
*		TOP PIXEL:   0b11000000, 0xRR, 0xGG, 0xBB
*		BOT PIXEL:	 0b11000010, 0xRR, 0xGG, 0xBB
* - Audio:
*      STEREO SMPL: 0b10000000, 0xLL, 0xLR, 0xRR
* - Dummy:
*      DATA:		 0b00000000, 0x00, 0x00, 0x00
*/
