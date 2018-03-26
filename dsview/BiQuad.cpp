#include "BiQuad.h"

#define _USE_MATH_DEFINES
#include <math.h>

BiQuad::BiQuad() {
	type = BQ_TYPE_LOWPASS;
	a0 = 1.0;
	a1 = a2 = b1 = b2 = 0.0;
	Fc = 0.50;
	Q = 0.707;
	peakGain = 0.0;
	z1 = z2 = 0.0;
}

BiQuad::BiQuad(int type, double Fc, double Q, double peakGainDB) {
	SetBiQuad(type, Fc, Q, peakGainDB);
	z1 = z2 = 0.0;
}

BiQuad::~BiQuad() {
}

void BiQuad::SetType(int type) {
	this->type = type;
	calcBiQuad();
}

void BiQuad::SetQ(double Q) {
	this->Q = Q;
	calcBiQuad();
}

void BiQuad::SetFc(double Fc) {
	this->Fc = Fc;
	calcBiQuad();
}

void BiQuad::SetPeakGain(double peakGainDB) {
	this->peakGain = peakGainDB;
	calcBiQuad();
}

void BiQuad::SetBiQuad(int type, double Fc, double Q, double peakGainDB) {
	this->type = type;
	this->Q = Q;
	this->Fc = Fc;
	SetPeakGain(peakGainDB);
}

void BiQuad::calcBiQuad(void) {
	double norm;
	double V = pow(10, fabs(peakGain) / 20.0);
	double K = tan(M_PI * Fc);
	switch (this->type) {
	case BQ_TYPE_LOWPASS:
		norm = 1 / (1 + K / Q + K * K);
		a0 = K * K * norm;
		a1 = 2 * a0;
		a2 = a0;
		b1 = 2 * (K * K - 1) * norm;
		b2 = (1 - K / Q + K * K) * norm;
		break;

	case BQ_TYPE_HIGHPASS:
		norm = 1 / (1 + K / Q + K * K);
		a0 = 1 * norm;
		a1 = -2 * a0;
		a2 = a0;
		b1 = 2 * (K * K - 1) * norm;
		b2 = (1 - K / Q + K * K) * norm;
		break;

	case BQ_TYPE_BANDPASS:
		norm = 1 / (1 + K / Q + K * K);
		a0 = K / Q * norm;
		a1 = 0;
		a2 = -a0;
		b1 = 2 * (K * K - 1) * norm;
		b2 = (1 - K / Q + K * K) * norm;
		break;

	case BQ_TYPE_NOTCH:
		norm = 1 / (1 + K / Q + K * K);
		a0 = (1 + K * K) * norm;
		a1 = 2 * (K * K - 1) * norm;
		a2 = a0;
		b1 = a1;
		b2 = (1 - K / Q + K * K) * norm;
		break;

	case BQ_TYPE_PEAK:
		if (peakGain >= 0) {    // boost
			norm = 1 / (1 + 1 / Q * K + K * K);
			a0 = (1 + V / Q * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - V / Q * K + K * K) * norm;
			b1 = a1;
			b2 = (1 - 1 / Q * K + K * K) * norm;
		}
		else {    // cut
			norm = 1 / (1 + V / Q * K + K * K);
			a0 = (1 + 1 / Q * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - 1 / Q * K + K * K) * norm;
			b1 = a1;
			b2 = (1 - V / Q * K + K * K) * norm;
		}
		break;
	case BQ_TYPE_LOWSHELF:
		if (peakGain >= 0) {    // boost
			norm = 1 / (1 + sqrt(2) * K + K * K);
			a0 = (1 + sqrt(2 * V) * K + V * K * K) * norm;
			a1 = 2 * (V * K * K - 1) * norm;
			a2 = (1 - sqrt(2 * V) * K + V * K * K) * norm;
			b1 = 2 * (K * K - 1) * norm;
			b2 = (1 - sqrt(2) * K + K * K) * norm;
		}
		else {    // cut
			norm = 1 / (1 + sqrt(2 * V) * K + V * K * K);
			a0 = (1 + sqrt(2) * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - sqrt(2) * K + K * K) * norm;
			b1 = 2 * (V * K * K - 1) * norm;
			b2 = (1 - sqrt(2 * V) * K + V * K * K) * norm;
		}
		break;
	case BQ_TYPE_HIGHSHELF:
		if (peakGain >= 0) {    // boost
			norm = 1 / (1 + sqrt(2) * K + K * K);
			a0 = (V + sqrt(2 * V) * K + K * K) * norm;
			a1 = 2 * (K * K - V) * norm;
			a2 = (V - sqrt(2 * V) * K + K * K) * norm;
			b1 = 2 * (K * K - 1) * norm;
			b2 = (1 - sqrt(2) * K + K * K) * norm;
		}
		else {    // cut
			norm = 1 / (V + sqrt(2 * V) * K + K * K);
			a0 = (1 + sqrt(2) * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - sqrt(2) * K + K * K) * norm;
			b1 = 2 * (K * K - V) * norm;
			b2 = (V - sqrt(2 * V) * K + K * K) * norm;
		}
		break;
	}

	return;
}
