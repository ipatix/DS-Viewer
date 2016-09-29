#ifdef _BUFFER_H_

#include "TRingbuffer.h"
#include "Buffer.h"

template <typename T>
Buffer<T>::Buffer(size_t elementCount, TRingbuffer<T>& _input)
	: bufData(elementCount), input(_input)
{
	dataPos = 0;
	filled = false;
}

template <typename T>
Buffer<T>::~Buffer()
{
}

template <typename T>
void Buffer<T>::Take(T *outData, size_t nElements)
{
	while (nElements > 0)
	{
		size_t count = take(outData, nElements);
		outData += count;
		nElements -= count;
	}
}

template <typename T>
void Buffer<T>::Clear()
{
	dataPos = 0;
	filled = false;
}

template <typename T>
size_t Buffer<T>::take(T* outData, size_t nElements)
{
	if (!filled)
		input.Take(bufData.data(), bufData.size());
	filled = true;
	bool wrap = nElements >= bufData.size() - dataPos;
	size_t count;
	size_t newdata;
	if (wrap)
	{
		count = bufData.size() - dataPos;
		newdata = 0;
		filled = false;
	}
	else
	{
		count = nElements;
		newdata = dataPos + count;
	}
	std::copy(&bufData[dataPos], &bufData[dataPos] + count, outData);
	dataPos = newdata;
	return count;
}

#endif