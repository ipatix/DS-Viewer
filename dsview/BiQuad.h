#pragma once

enum {
	BQ_TYPE_LOWPASS = 0,
	BQ_TYPE_HIGHPASS,
	BQ_TYPE_BANDPASS,
	BQ_TYPE_NOTCH,
	BQ_TYPE_PEAK,
	BQ_TYPE_LOWSHELF,
	BQ_TYPE_HIGHSHELF
};

class BiQuad {
public:
	BiQuad();
	BiQuad(int type, double Fc, double Q, double peakGainDB);
	~BiQuad();
	void SetType(int type);
	void SetQ(double Q);
	void SetFc(double Fc);
	void SetPeakGain(double peakGainDB);
	void SetBiQuad(int type, double Fc, double Q, double peakGain);
	float Process(float in);

protected:
	void calcBiQuad(void);

	int type;
	double a0, a1, a2, b1, b2;
	double Fc, Q, peakGain;
	double z1, z2;
};

inline float BiQuad::Process(float in) {
	double out = in * a0 + z1;
	z1 = in * a1 + z2 - b1 * out;
	z2 = in * a2 - b2 * out;
	return float(out);
}
