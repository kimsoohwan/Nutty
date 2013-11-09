#pragma once
#include "../cuda/sort.h"
#include "../HostBuffer.h"
#include "../DeviceBuffer.h"

namespace nutty
{
    template <
        typename T
    >
    struct BinaryDescending
    {
        __device__ __host__ char operator()(T f0, T f1)
        {
            return f0 > f1;
        }
    };

    template <
        typename T
    >
    struct BinaryAscending
    {
        __device__ __host__ char operator()(T f0, T f1)
        {
            return f0 < f1;
        }
    };

    namespace base
    {
        template <
            typename T,
            typename BinaryOperation
        >
        void SortPerGroup(
        Iterator<
                T, nutty::base::Base_Buffer<T, nutty::DeviceContent<T>, nutty::CudaAllocator<T>>
                >& start, 
        Iterator<
                T, nutty::base::Base_Buffer<T, nutty::DeviceContent<T>, nutty::CudaAllocator<T>>
                >& end, 
        uint elementsPerBlock, uint startStage, uint endStage, uint startStep, uint length, BinaryOperation op)
        {
            nutty::cuda::SortPerGroup(start, end, elementsPerBlock, startStage, endStage, startStep, length, op);
        }

        template<
            typename T, 
            typename BinaryOperation
        >
        void SortStep(Iterator<T, nutty::base::Base_Buffer<T, nutty::DeviceContent<T>, nutty::CudaAllocator<T>>
        >& start, uint grid, uint block, uint stage, uint step, uint length, BinaryOperation op, uint offset = 0)
        {
            nutty::cuda::SortStep(start, grid, block, stage, step, length, op, offset);
        }

        template <
            typename T,
            typename C,
            typename BinaryOperation
        >
        void Sort(Iterator<T, C>& start, Iterator<T, C>& end, BinaryOperation op)
        {
            uint length = (uint)Distance(start, end);

            uint elemCount = length;

            const uint maxElemsBlock = 512;

            uint elemsPerBlock = maxElemsBlock;

            if(elemCount >= elemsPerBlock)
            {
                elemCount = elemsPerBlock;
            }
            else
            {
                elemsPerBlock = elemCount;
            }

            uint perGroupEndStage = elemsPerBlock;
            if(elemsPerBlock & (elemsPerBlock-1))
            {
                perGroupEndStage = 1 << (getmsb(elemsPerBlock) + 1);
            }

            SortPerGroup(start, end, elemsPerBlock, 2, perGroupEndStage, 1, length, op);

            elemCount = length;
            elemsPerBlock = maxElemsBlock;

            if(elemCount <= elemsPerBlock)
            {
                return;
            }

            uint stageStart = elemsPerBlock << 1;
            uint grid = cuda::getCudaGrid(length, elemsPerBlock);

            uint endStage = length;
            if(length & (length-1))
            {
                endStage = 1 << (getmsb(length) + 1);
            }

            for(uint pow2stage = stageStart; pow2stage <= endStage; pow2stage <<= 1)
            {
                for(uint step = pow2stage >> 1; step > 0; step = step >> 1)
                {
                    if((step << 1) <= elemsPerBlock)
                    {
                        SortPerGroup(start, end, elemsPerBlock, pow2stage, pow2stage, step, length, op);
                        break;
                    }
                    else
                    {
                        SortStep(start, grid, elemsPerBlock, pow2stage, step, length, op);
                    }
                }
            }
        }

        //key value

        /*template <
            typename K,
            typename T,
            typename BinaryOperation
        >
        void SortKeyPerGroup(
        Iterator<
                K, nutty::base::Base_Buffer<K, nutty::DeviceContent<K>, nutty::CudaAllocator<K>>
                >& keyBegin, 
        Iterator<
                K, nutty::base::Base_Buffer<K, nutty::DeviceContent<K>, nutty::CudaAllocator<K>>
                >& keyEnd,
        Iterator<
                T, nutty::base::Base_Buffer<T, nutty::DeviceContent<T>, nutty::CudaAllocator<T>>
                >& values, 
        uint elementsPerBlock, uint startStage, uint endStage, uint startStep, uint length, BinaryOperation op)
        {
            nutty::cuda::SortKeyPerGroup(keyBegin, keyEnd, values, elementsPerBlock, startStage, endStage, startStep, length, op);
        }*/

        template <
            typename K,
            typename T, 
            typename BinaryOperation
        >
        void SortKeyStep(
        Iterator<
                K, nutty::base::Base_Buffer<K, nutty::DeviceContent<K>, nutty::CudaAllocator<K>>
                >& keyBegin, 
        Iterator<
                T, nutty::base::Base_Buffer<T, nutty::DeviceContent<T>, nutty::CudaAllocator<T>>
                >& values, 
        uint grid, uint block, uint stage, uint step, BinaryOperation op, uint offset = 0)
        {
            nutty::cuda::SortKeyStep(keyBegin, values, grid, block, stage, step, op, offset);
        }

        template <
            typename T,
            typename C,
            typename K,
            typename KC,
            typename BinaryOperation
        >
        void Sort(Iterator<K, KC>& keyStart, Iterator<K, KC>& keyEnd, Iterator<T, C>& valuesBegin, BinaryOperation op)
        {
            uint length = (uint)Distance(keyStart, keyEnd);

            uint elemCount = length;

            const uint maxElemsBlock = 512;

            uint elemsPerBlock = maxElemsBlock;

            if(elemCount >= elemsPerBlock)
            {
                elemCount = elemsPerBlock;
            }
            else
            {
                elemsPerBlock = elemCount;
            }

            uint perGroupEndStage = elemsPerBlock;
            if(elemsPerBlock & (elemsPerBlock-1))
            {
                perGroupEndStage = 1 << (getmsb(elemsPerBlock) + 1);
            }

            nutty::cuda::SortKeyPerGroup(keyStart, keyEnd, valuesBegin, elemsPerBlock, 2, perGroupEndStage, 1, length, op);

            elemCount = length;
            elemsPerBlock = maxElemsBlock;

            if(elemCount <= elemsPerBlock)
            {
                return;
            }
            /*
            uint stageStart = elemsPerBlock << 1;
            uint grid = cuda::getCudaGrid(length, elemsPerBlock);

            uint endStage = length;
            if(length & (length-1))
            {
                endStage = 1 << (getmsb(length) + 1);
            }

            for(uint pow2stage = stageStart; pow2stage <= endStage; pow2stage <<= 1)
            {
                for(uint step = pow2stage >> 1; step > 0; step = step >> 1)
                {
                    if((step << 1) <= elemsPerBlock)
                    {
                        SortKeyPerGroup(start, end, elemsPerBlock, pow2stage, pow2stage, step, length, op);
                        break;
                    }
                    else
                    {
                        SortKeyStep(start, grid, elemsPerBlock, pow2stage, step, length, op);
                    }
                }
            }
            */
        }
    }
}