#pragma once

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <cstddef>
#include <vector>

#include "TRingbuffer.h"

template <typename T>
class Buffer
{
public:
	Buffer(size_t elementCount, TRingbuffer<T>& _input);
	~Buffer();

	void Take(T *outData, size_t nElements);
	void Clear();
private:
	size_t take(T *outData, size_t nElements);

	std::vector<T> bufData;
	TRingbuffer<T>& input;

	size_t dataPos;
	bool filled;
};

#include "Buffer.cpp"
#endif