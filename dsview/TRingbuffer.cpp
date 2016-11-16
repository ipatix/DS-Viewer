#ifdef _RINGBUFFER_H

#include <algorithm>
#include <iostream>

#include "TRingbuffer.h"

/*
 * public Ringbuffer
 */

template <typename T>
TRingbuffer<T>::TRingbuffer(size_t elementCount, bool blockOnTake, bool blockOnPut)
    : bufData(elementCount)
{
    freePos = 0;
    dataPos = 0;
    freeCount = elementCount;
    dataCount = 0;
    this->blockOnTake = blockOnTake;
    this->blockOnPut = blockOnPut;
}

template <typename T>
TRingbuffer<T>::~TRingbuffer()
{
}

template <typename T>
void TRingbuffer<T>::Put(T *inData, size_t nElements)
{
    if (blockOnPut) {
        std::unique_lock<std::mutex> lock(countLock);
        while (freeCount < nElements){
            sigToFeeder.wait(lock);
        }
        while (nElements > 0) {
            size_t count = put(inData, nElements);
            inData += count;
            nElements -= count;
        }
        if (blockOnTake)
            sigToConsumer.notify_one();
    } else {
        if (freeCount >= nElements) {
            while (nElements > 0) {
                size_t count = put(inData, nElements);
                inData += count;
                nElements -= count;
            }
            if (blockOnTake)
                sigToConsumer.notify_one();
        }
    }
}

template <typename T>
void TRingbuffer<T>::Take(T *outData, size_t nElements)
{
    if (blockOnTake) {
        std::unique_lock<std::mutex> lock(countLock);
        while (dataCount < nElements) {
            sigToConsumer.wait(lock);
        }
        while (nElements > 0) {
            size_t count = take(outData, nElements);
            outData += count;
            nElements -= count;
        }
        sigToFeeder.notify_one();
    } else {
        if (dataCount < nElements) {
            std::fill(outData, outData + nElements, T());
			std::cout << "BUFFER UNDERRUN\n";
        } else {
            // output
            std::unique_lock<std::mutex> lock(countLock);
            while (nElements > 0) {
                size_t count = take(outData, nElements);
                outData += count;
                nElements -= count;
            }
            sigToFeeder.notify_one();
        }
    }
}

template <typename T>
void TRingbuffer<T>::Clear()
{
    std::unique_lock<std::mutex>(countLock);
    std::fill(bufData.begin(), bufData.end(), T());
}

/*
 * private Ringbuffer
 */

template <typename T>
size_t TRingbuffer<T>::put(T *inData, size_t nElements)
{
    bool wrap = nElements >= bufData.size() - freePos;
    size_t count;
    size_t newfree;
    if (wrap) {
        count = size_t(bufData.size() - freePos);
        newfree = 0;
    } else {
        count = nElements;
        newfree = freePos + count;
    }
    std::copy(inData, inData + count, &bufData[freePos]);
    freePos = newfree;
    freeCount -= count;
    dataCount += count;
    return count;
}

template <typename T>
size_t TRingbuffer<T>::take(T *outData, size_t nElements)
{
    bool wrap = nElements >= bufData.size() - dataPos;
    size_t count;
    size_t newdata;
    if (wrap) {
        count = size_t(bufData.size() - dataPos);
        newdata = 0;
    } else {
        count = nElements;
        newdata = dataPos + count;
    }
    std::copy(&bufData[dataPos], &bufData[dataPos] + count, outData);
    dataPos = newdata;
    freeCount += count;
    dataCount -= count;
    return count;
}

#endif
