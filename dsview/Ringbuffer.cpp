#ifdef _RINGBUFFER_H

#include <algorithm>

#include "Ringbuffer.h"

using namespace std;

/*
 * public Ringbuffer
 */

template <typename T>
Ringbuffer<T>::Ringbuffer(size_t elementCount, bool blockOnTake)
    : bufData(elementCount)
{
    freePos = 0;
    dataPos = 0;
    freeCount = elementCount;
    dataCount = 0;
    this->blockOnTake = blockOnTake;
}

template <typename T>
Ringbuffer<T>::~Ringbuffer()
{
}

template <typename T>
void Ringbuffer<T>::Put(T *inData, size_t nElements)
{
    unique_lock<mutex> lock(countLock);
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
}

template <typename T>
void Ringbuffer<T>::Take(T *outData, size_t nElements)
{
    if (blockOnTake) {
        unique_lock<mutex> lock(countLock);
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
            fill(outData, outData + nElements, T());
        } else {
            // output
            unique_lock<mutex> lock(countLock);
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
void Ringbuffer<T>::Clear()
{
    unique_lock<mutex>(countLock);
    fill(bufData.begin(), bufData.end(), T());
}

/*
 * private Ringbuffer
 */

    template <typename T>
size_t Ringbuffer<T>::put(T *inData, size_t nElements)
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
    copy(inData, inData + count, &bufData[freePos]);
    freePos = newfree;
    freeCount -= count;
    dataCount += count;
    return count;
}

    template <typename T>
size_t Ringbuffer<T>::take(T *outData, size_t nElements)
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
    copy(&bufData[dataPos], &bufData[dataPos] + count, outData);
    dataPos = newdata;
    freeCount += count;
    dataCount -= count;
    return count;
}

#endif
