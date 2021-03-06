#pragma once
#include "../Inc.h"
#include "Event.h"

namespace nutty
{
    class cuStream
    {
    private:
        CUstream m_stream;

        cuStream(cuStream&) {}

    public:
        cuStream(void)
        {
            CUDA_DRIVER_SAFE_CALLING_SYNC(cuStreamCreate(&m_stream, 0));
        }

        cuStream(cuStream&& s)
        {
            m_stream = s.m_stream;
            s.m_stream = NULL;
        }

        CUstream GetPointer(void) const
        {
            return m_stream;
        }

        CUstream operator()(void) const
        {
            return GetPointer();
        }

        void WaitEvent(cuEvent event)
        {
            CUevent ptr = event.Free();
            CUDA_DRIVER_SAFE_CALLING_SYNC(cuStreamWaitEvent(m_stream, ptr, 0));
            //m_events.push_back(ptr);
            CUDA_DRIVER_SAFE_CALLING_SYNC(cuEventDestroy(ptr));
        }

        cuEvent RecordEvent(void) const
        {
            cuEvent e;
            CUDA_DRIVER_SAFE_CALLING_SYNC(cuEventRecord(e(), m_stream));
            return e;
        }

        ~cuStream(void)
        {
            if(m_stream)
            {
                CUDA_DRIVER_SAFE_CALLING_SYNC(cuStreamDestroy(m_stream));
                m_stream = NULL;
            }
        }
    };

    class cuStreamPool
    {
    private:
        const static byte MAX_LIMIT = 16;
        cuStream* m_pStreams[MAX_LIMIT]; //fermi
        byte m_index;
        byte m_limit;

    public:
        cuStreamPool(byte limit = MAX_LIMIT) : m_index(0), m_limit(min(limit, MAX_LIMIT))
        {
            assert(limit > 0);
            for(byte i = 0; i < m_limit; ++i)
            {
                m_pStreams[i] = new cuStream();
            }
        }

        void Reset(void)
        {
            m_index = 0;
        }

        byte GetStreamCount(void)
        {
            return MAX_LIMIT;
        }
        
        cuStream& PeekNextStream(void)
        {
            return *m_pStreams[(m_index++) % m_limit];
        }

        cuStream& GetStream(byte index)
        {
            return *m_pStreams[index];
        }

        ~cuStreamPool(void)
        {
            for(byte i = 0; i < m_limit; ++i)
            {
                SAFE_DELETE(m_pStreams[i]);
                m_pStreams[i] = NULL;
            }
        }
    };
}