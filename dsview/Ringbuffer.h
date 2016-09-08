#pragma once

#ifndef _RINGBUFFER_H
#define _RINGBUFFER_H

#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstddef>

template <typename T>
class Ringbuffer
{
    public:
        Ringbuffer(size_t elementCount, bool blockOnTake);
        ~Ringbuffer();

        void Put(T *inData, size_t nElements);
        void Take(T *outData, size_t nElements);
        void Clear();
    private:
        size_t put(T *inData, size_t nElements);
        size_t take(T *outData, size_t nElements);

        std::vector<T> bufData;
        std::mutex countLock;
        std::condition_variable sigToFeeder;
        std::condition_variable sigToConsumer;
        size_t freePos;
        size_t dataPos;
        size_t freeCount;
        size_t dataCount;
        bool blockOnTake;
};

#include "Ringbuffer.cpp"
#endif
