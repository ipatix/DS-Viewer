#pragma once

enum {
	bq_type_lowpass = 0,
	bq_type_highpass,
	bq_type_bandpass,
	bq_type_notch,
	bq_type_peak,
	bq_type_lowshelf,
	bq_type_highshelf
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
