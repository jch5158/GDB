#pragma once

#include <iostream>
#include <Windows.h>

namespace ringbuffer
{
    constexpr int MAX_QUEUE_SIZE = 10000;
};


class CRingBuffer
{
public:

    CRingBuffer(void)
        : mFront(0)
        , mRear(0)
        , mpBufferPtr((char*)malloc(ringbuffer::MAX_QUEUE_SIZE))
        , mBufferSize(ringbuffer::MAX_QUEUE_SIZE)
    {}

    explicit CRingBuffer(int bufferSize)
        : mFront(0)
        , mRear(0)
        , mpBufferPtr((char*)malloc(bufferSize))
        , mBufferSize(bufferSize)
    {}

    ~CRingBuffer(void)
    {
        release();
    }

    CRingBuffer(const CRingBuffer&) = delete;
    CRingBuffer& operator=(const CRingBuffer&) = delete;


    int GetBufferSize(void) const
    {
        return mBufferSize;
    }

    int GetUseSize(void) const
    {
        int front = mFront;
        int rear = mRear;

        // front 위치가 rear 보다 앞에 있다면
        if (front > rear)
            return mBufferSize - (front - rear);

        return rear - front;
    }

    int GetFreeSize(void) const
    {
        int front = mFront;
        int rear = mRear;

        // front 위치가 rear 보다 앞에 있다면
        if (front > rear)
            return front - rear - 1;

        return mBufferSize - (rear - front) - 1;
    }

    bool IsFull(void) const
    {
        // front 가 0 rear가 경계에 있다면은 +1 후 mBufferSize로 나머지 연산을 해주어야 함
        return mFront == (mRear + 1) % mBufferSize;
    }

    bool IsEmpty(void) const
	{
        // front와 rear 가 같은 index에 있으면은 비어있음
        return mFront == mRear;
	}

    int GetDirectEnqueueSize(void) const
    {
        int front = mFront;
        int rear = mRear;

        if (front > rear)
            return front - rear - 1;

        // front가 0만 아니라면 더 넣을 수 있음
        if (front == 0)
            return mBufferSize - rear - 1;

        return mBufferSize - rear;
    }


    int GetDirectDequeueSize(void) const
    {
        int front = mFront;
        int rear = mRear;

        if (front > rear)
            return mBufferSize - front;

        return rear - front; 
    }


    int Enqueue(const char* pData, int size)
    {
        int rear = mRear;

        int bufferSize = mBufferSize;

        int freeSize = GetFreeSize();
        if (size > freeSize)
            size = freeSize;

        // size + rear이 buffer size를 초과했다면 2번 나누어 Enqeue
        if (size + rear <= bufferSize)
            memcpy(&mpBufferPtr[rear], pData, size);
        else
        {
            int directEnqueueSize = bufferSize - rear;

            memcpy(&mpBufferPtr[rear], pData, directEnqueueSize);

            int remainSize = size - directEnqueueSize;

            memcpy(&mpBufferPtr[0], &pData[directEnqueueSize], remainSize);
        }

        mRear = (rear + size) % bufferSize;

        return size;
    }


    int Dequeue(char* pDest, int size)
    {
        int front = mFront;

        int bufferSize = mBufferSize;
        
        int useSize = GetUseSize();
        if (size > useSize)
            size = useSize;

        // size + front이 buffer size를 초과했다면 2번 나누어 Dequeue
        if (size + front <= bufferSize)
            memcpy(pDest, &mpBufferPtr[front], size);
        else
        {
            int directDequeueSize = bufferSize - front;

            memcpy(pDest, &mpBufferPtr[front], directDequeueSize);

            int remainSize = size - directDequeueSize;

            memcpy(&pDest[directDequeueSize], &mpBufferPtr[0], remainSize);
        }

        mFront = (front + size) % bufferSize;

        return size;
    }

    // RingBuffer 데이터를 복사하고 Dequeue와 달리 Front를 움직이 않는다.
    int Peek(char* pDest, int size) const
    {
        int front = mFront;
        int rear = mRear;

        int bufferSize = mBufferSize;

        int useSize = GetUseSize();
        if (size > useSize)
            size = useSize;

        // size + front이 buffer size를 초과했다면 2번 나누어서 Peek
        if (size + front <= bufferSize)
            memcpy(pDest, &mpBufferPtr[front], size);
        else
        {
            int directDequeueSize = bufferSize - front;

            memcpy(pDest, &mpBufferPtr[front], directDequeueSize);

            int remainingSize = size - directDequeueSize;

            memcpy(&pDest[directDequeueSize], &mpBufferPtr[0], remainingSize);
        }

        return size;
    }


    void MoveRear(int size)
    {
        mRear = (mRear + size) % mBufferSize;

        return;
    }

    void MoveFront(int size)
    {
        mFront = (mFront + size) % mBufferSize;

        return;
    }

    void ClearBuffer(void)
    {
        mFront = 0;

        mRear = 0;

        return;
    }

    char* GetBufferPtr(void) const
    {
        return mpBufferPtr;
    }

    char* GetFrontBufferPtr(void) const
    {
        return &mpBufferPtr[mFront];
    }

    char* GetRearBufferPtr(void) const
    {
        return &mpBufferPtr[mRear];
    }

private:

    void release(void)
    {
        free(mpBufferPtr);

        return;
    }

    int mFront;

    int mRear;

    int mBufferSize; // 생성된 RingBuffer 크기

    char* mpBufferPtr;
};


