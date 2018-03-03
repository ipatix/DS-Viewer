#pragma once

#ifndef _RINGBUFFER_H
#define _RINGBUFFER_H

#include <vector>
#include <cstddef>

template <typename T>
class Ringbuffer
{
	private:
		std::vector<T> bufData;
		size_t freePos;
		size_t dataPos;
		size_t freeCount;
		size_t dataCount;
    public:
        Ringbuffer(size_t elementCount)
			: bufData(elementCount)
		{
			freePos = 0;
			dataPos = 0;
			freeCount = elementCount;
			dataCount = 0;
		}
		~Ringbuffer() { }

        size_t Put(T *inData, size_t nElements)
		{
			if (nElements > freeCount)
				nElements = freeCount;

			size_t total = nElements;
			while (nElements > 0) {
				size_t count = put(inData, nElements);
				inData += count;
				nElements -= count;
			}
			return total;
		}
		size_t Take(T *outData, size_t nElements)
		{
			if (nElements > dataCount)
				nElements = dataCount;

			size_t total = nElements;
			while (nElements > 0) {
				size_t count = take(outData, nElements);
				outData += count;
				nElements -= count;
			}
			return total;
		}
		size_t DataCount()
		{
			return dataCount;
		}
		void Clear()
		{
			std::fill(bufData.begin(), bufData.end(), T());
		}
    private:
		size_t put(T *inData, size_t nElements)
		{
			bool wrap = nElements >= bufData.size() - freePos;
			size_t count;
			size_t newfree;
			if (wrap) {
				count = size_t(bufData.size() - freePos);
				newfree = 0;
			}
			else {
				count = nElements;
				newfree = freePos + count;
			}
			std::copy(inData, inData + count, &bufData[freePos]);
			freePos = newfree;
			freeCount -= count;
			dataCount += count;
			return count;
		}
		size_t take(T *outData, size_t nElements)
		{
			bool wrap = nElements >= bufData.size() - dataPos;
			size_t count;
			size_t newdata;
			if (wrap) {
				count = size_t(bufData.size() - dataPos);
				newdata = 0;
			}
			else {
				count = nElements;
				newdata = dataPos + count;
			}
			std::copy(&bufData[dataPos], &bufData[dataPos] + count, outData);
			dataPos = newdata;
			freeCount += count;
			dataCount -= count;
			return count;
		}
};

#endif
