#pragma once


#include <iostream>
#include <Windows.h>


namespace templatequeue
{
    constexpr int MAX_QUEUE_SIZE = 10000;
}

template <typename T>
class CTemplateQueue
{
public:

    CTemplateQueue(void)
        : mpRingBuffer((T*)malloc(templatequeue::MAX_QUEUE_SIZE * sizeof(T)))
        , mBufferSize(templatequeue::MAX_QUEUE_SIZE)
        , mFront(0)
        , mRear(0)
    {
    }

    explicit CTemplateQueue(int bufferSize)
        : mpRingBuffer((T*)malloc(bufferSize * sizeof(T)))
        , mBufferSize(bufferSize)
        , mFront(0)
        , mRear(0)
    {}

    CTemplateQueue(const CTemplateQueue&) = delete;
    CTemplateQueue& operator=(const CTemplateQueue&) = delete; 

    ~CTemplateQueue(void)
    {
        release();
    }

    int GetBufferSize(void) const
    {
        return mBufferSize;
    }

    int GetUseSize(void) const
    {
        int front = mFront;
        int rear = mRear;

        if (front > rear)
            return mBufferSize - (front - rear);

        return rear - front;
    }

    int GetFreeSize(void) const
    {
        int front = mFront;
        int rear = mRear;

        // Front 뒤는 꽉참 판단을 하기위해 한칸 비워놓기 때문에 -1을 해야한다.
        if (front > rear)
            return front - rear - 1;

        return mBufferSize - (rear - front) - 1;
    }


    int GetDirectEnqueueSize(void) const
    {
        int front = mFront;
        int rear = mRear;

        // front 뒤는 한칸 비워야하기 때문에 1을 뺌
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

    int Enqueue(const T* pData, int size)
    {
        int rear = mRear;
        int bufferSize = mBufferSize;

        int freeSize = GetFreeSize();
        if (size > freeSize)
            size = freeSize;

        // size + rear이 buffer size를 초과했다면 2번 나누어 Enqeue
        if ((size + rear) <= bufferSize)
            memcpy(&mpRingBuffer[rear], pData, size * sizeof(T));
        else
        {
            int directEnqueueSize = bufferSize - rear;

            memcpy(&mpRingBuffer[rear], pData, directEnqueueSize * sizeof(T));

            int remainingSize = size - directEnqueueSize;

            memcpy(&mpRingBuffer[0], &pData[directEnqueueSize], remainingSize * sizeof(T));
        }

        mRear = (rear + size) % bufferSize;

        return size;
    }

    bool Enqueue(T pData)
    {
        if (IsFull() == true)
            return false;

        int rear = mRear;

        memcpy(&mpRingBuffer[rear], &pData, sizeof(T));

        mRear = (rear + 1) % mBufferSize;

        return true;
    }


    int Dequeue(T* pDest, int size)
    {
        int front = mFront;

        int bufferSize = mBufferSize;

        int useSize = GetUseSize();
        if (size > useSize)
            size = useSize;

        // Dequeue할 Data가 buffer size를 초과했다면 2번 나누어서 넣는다.
        if (size + front <= bufferSize)
            memcpy(pDest, &mpRingBuffer[front], size * sizeof(T));
        else
        {
            int directDequeueSize = bufferSize - front;

            memcpy(pDest, &mpRingBuffer[front], directDequeueSize * sizeof(T));

            int remainingSize = size - directDequeueSize;

            memcpy(&pDest[directDequeueSize], &mpRingBuffer[0], remainingSize * sizeof(T));
        }

        mFront = (front + size) % bufferSize;

        return size;
    }

    bool Dequeue(T* pDest)
    {
        if (IsEmpty() == true)
            return false;
    
        int front = mFront;

        memcpy(pDest, &mpRingBuffer[front], sizeof(T));

        mFront = (front + 1) % mBufferSize;

        return true;
    }

    int Peek(T* pDest, int size) const
    {
        int front = mFront;

        int rear = mRear;

        int bufferSize = mBufferSize;

        int useSize = GetUseSize();
        if (size > useSize)
            size = useSize;

        // Peek할 Data가 buffer size를 초과했다면 2번 나누어서 넣는다.
        if (size + front <= bufferSize)
            memcpy(pDest, &mpRingBuffer[front], size * sizeof(T));
        else
        {
            int directDequeueSize = bufferSize - front;

            memcpy(pDest, &mpRingBuffer[front], directDequeueSize * sizeof(T));

            int remainingSize = size - directDequeueSize;

            memcpy(&pDest[directDequeueSize], &mpRingBuffer[0], remainingSize * sizeof(T));
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

    T* GetBufferPtr(void) const
    {
        return mpRingBuffer;
    }

    T* GetFrontBufferPtr(void) const
    {
        return &mpRingBuffer[mFront];
    }


    T* GetRearBufferPtr(void) const
    {
        return &mpRingBuffer[mRear];
    }

    bool IsFull(void) const
    {
        return mFront == (mRear + 1) % mBufferSize;
    }

    bool IsEmpty(void) const
    {
        return mFront == mRear;
    }

private:

    void release(void)
    {
        free(mpRingBuffer);

        return;
    }

    int mFront;

    int mRear;

    int mBufferSize; 

    T* mpRingBuffer;
};