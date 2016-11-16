#pragma once

#include "Config.h"

template <unsigned int channels>
class HighPass
{
public:
	HighPass(float freq)
	{
		float rc = 1.0f / (freq * 2.0f * float(M_PI));
		float dt = 1.0f / float(AUDIO_SAMPLERATE);
		alpha = dt / (rc + dt);
	}
	void process(float *data, size_t frames)
	{
		for (size_t i = 0; i < frames; i++)
		{
			for (unsigned int j = 0; j < channels; j++)
			{
				state[j] = *data - alpha * (*data - state[j]);
				*data++ = state[j];
			}
		}
	}
private:
	float alpha;
	float state[channels];
};