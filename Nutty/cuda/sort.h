#pragma once
#include "Globals.cuh"
#include "cuda_helper.h"

#ifndef ___host
#define ___host __host__ __device__
#endif

namespace nutty
{
    namespace cuda
    {
        /************************************************************************/
        /* VALUE SORT                                                           */
        /************************************************************************/
        template <
            typename T, 
            typename BinaryOperation
        >
        __device__ char __cmp(T t0, T t1, BinaryOperation _op, uint stage)
        {
            char dir = (((2 * GlobalId ) / stage) & 1);
            char cmp = _op(t0, t1);
            return (!dir & cmp) | (dir & !cmp);
        }

        template <
            typename T, 
            typename BinaryOperation
        >
        __device__ void __bitonicMergeSortStep(T* v, uint stage, uint step, uint tid, uint length, BinaryOperation _cmp_func)
        {
            uint first = (step<<1) * (tid / step);
            uint second = first + step;
            
            uint bankOffset = (tid % step);
            char isBankOffset = bankOffset > 0;

            first += bankOffset * isBankOffset;
            second += bankOffset * isBankOffset;

            uint gsecond = (step<<1) * (GlobalId / step) + step;
            gsecond += bankOffset * isBankOffset;
            
            uint start = (step<<1) * (GlobalId/(step));

            if(start + (step<<1) > length)
            {
                char dir = (((2 * GlobalId ) / stage) & 1);
                if(!dir && stage == (step<<1))
                {
                    first += (step<<1) - (length%(step<<1));
                }
            }

            if(gsecond < length)
            {
                T n0 = v[first];

                T n1 = v[second];

                if(__cmp(n0, n1, _cmp_func, stage))
                {
                    v[second] = n0;
                    v[first] = n1;
                }
            }
        }

        template <
            typename T, 
            typename BinaryOperation
        >
        __global__ void bitonicMergeSortStep(T* v, uint stage, uint step, uint length, BinaryOperation _cmp_func)
        {
            __bitonicMergeSortStep(v, stage, step, GlobalId, length, _cmp_func);
        }

        template <
            typename T, 
            typename BinaryOperation
        >
        __global__ void bitonicMergeSortPerGroup(T* g_values, uint startStage, uint endStage, uint step, uint length, BinaryOperation _cmp_func)
        {
            volatile uint __i  = (2 * GlobalId);

            if(__i >= length)
            {
                return;
            }

            ShrdMemory<T> shrdMem;
            T* shrd = shrdMem.Ptr();

            shrd[2 * threadIdx.x + 0] = g_values[__i + 0];

            if(__i+1 < length)
            {
                shrd[2 * threadIdx.x + 1] = g_values[__i + 1];
            }

            for(uint stage = startStage; stage <= endStage;)
            {
                for(;step > 0; step = step >> 1)
                {
                    __syncthreads();
                    __bitonicMergeSortStep(shrd, stage, step, threadIdx.x, length, _cmp_func);
                }

                step = stage;
                stage <<= 1;
                //step = stage >> 1;
            }

            g_values[__i + 0] = shrd[2 * threadIdx.x + 0];

            if(__i+1 < length)
            {
                g_values[__i + 1] = shrd[2 * threadIdx.x + 1];
            }
            /*
            uint tId = threadIdx.x;
            uint i = GlobalId; //2*
            
            if(2*i >= length)
            {
                return;
            }

            ShrdMemory<T> shrdMem;
            T* shrd = shrdMem.Ptr();

            shrd[tId + 0] = g_values[i + 0];

            if(i + blockDim.x < length)
            {
                shrd[tId + blockDim.x] = g_values[i + blockDim.x];
            }

            uint step = startStep;

            for(uint stage = startStage; stage <= endStage;)
            {
                for(;step > 0; step = step >> 1)
                {
                    __syncthreads();
                    __bitonicMergeSortStep(shrd, stage, step, tId, length, _cmp_func);
                }

                stage <<= 1;
                step = stage >> 1;
            }

            __syncthreads();

            g_values[i + 0] = shrd[tId + 0];

            if(i + blockDim.x < length)
            {
                g_values[i + blockDim.x] = shrd[tId + blockDim.x];
            }*/
        }

        template <
            typename T,
            typename BinaryOperation
        >
        ___host void SortPerGroup(
        DevicePtr<T>& vals,
        uint elementsPerBlock, uint startStage, uint endStage, uint startStep, uint length, BinaryOperation op, cudaStream_t pStream = NULL)
        {
            cudaBitonicMergeSortPerGroup(vals(), elementsPerBlock, startStage, endStage, startStep, length, op, pStream);
        }

        template <
            typename T,
            typename BinaryOperation
        >
        ___host void SortPerGroup(
        Iterator<
            T, nutty::base::Base_Buffer<T, nutty::DeviceContent<T>, nutty::CudaAllocator<T>>
            >& vals,
        uint elementsPerBlock, uint startStage, uint endStage, uint startStep, uint length, BinaryOperation op, cudaStream_t pStream = NULL)
        {
            cudaBitonicMergeSortPerGroup(vals(), elementsPerBlock, startStage, endStage, startStep, length, op, pStream);
        }

        template <
            typename T,
            typename BinaryOperation
        >
        ___host void cudaBitonicMergeSortPerGroup(
        T* begin, uint elementsPerBlock, uint startStage, uint endStage, uint startStep, uint length, BinaryOperation op, cudaStream_t pStream = NULL)
        {
            dim3 block = (elementsPerBlock + elementsPerBlock%2)/2;
            dim3 grid = GetCudaGrid(length, elementsPerBlock);

            uint shrdMem = elementsPerBlock * sizeof(T);

            bitonicMergeSortPerGroup
                <<<grid, block, shrdMem, pStream>>>
                (
                begin, startStage, endStage, startStep, length, op
                );
        }

        template <
            typename Iterator_,
            typename BinaryOperation
        >
        ___host void SortStep(
            Iterator_& values,
        uint grid, uint block, uint stage, uint step, uint length, BinaryOperation op, cudaStream_t pStream = NULL)
        {
            bitonicMergeSortStep
                <<<grid, block, 0, pStream>>>
                (
                values(), stage, step, length, op
                );
        }

        /************************************************************************/
        /* KEY VALUE SORT                                                       */
        /************************************************************************/
                /*
        template <
        template <typename, typename> class KVPair, class K, class V,
        typename BinaryOperation
        >
        __device__ void __bitonicMergeSortKeyStep(KVPair<K, V>* kv, uint stage, uint step, uint id, BinaryOperation _cmp_func, uint offset = 0)
        {

        }
        */

        template <
            typename V,
            typename K,
            typename BinaryOperation
        >
        __device__ void __bitonicMergeSortKeyStep(V* v, K* k, uint stage, uint step, uint id, uint length, BinaryOperation _cmp_func, uint offset = 0)
        {
            uint first = (step << 1) * (id / step);
            uint second = first + step;

            uint bankOffset = (id % step);
            char isBankOffset = bankOffset > 0;

            first += bankOffset * isBankOffset;
            second += bankOffset * isBankOffset;

            uint gsecond = (step << 1) * (GlobalId / step) + step;
            gsecond += bankOffset * isBankOffset;

            uint start = (step<<1) * (GlobalId/(step));

            if(start + (step<<1) > length)
            {
                char dir = (((2 * GlobalId ) / stage) & 1);
                if(!dir && stage == (step<<1))
                {
                    first += (step<<1) - (length%(step<<1));
                }
            }

            if(offset + gsecond < length)
            {
                K k0 = k[offset + first];

                K k1 = k[offset + second];

                V v0 = v[k0];

                V v1 = v[k1];

                if(__cmp(v0, v1, _cmp_func, stage))
                {
                    k[offset + first] = k1;
                    k[offset + second] = k0;
                }
            }
        }

        template <
            typename T, 
            typename K,
            typename BinaryOperation
        >
        __global__ void bitonicMergeSortKeyStep(T* v, K* k, uint stage, uint step, uint length, BinaryOperation _cmp_func, uint offset = 0)
        {
            __bitonicMergeSortKeyStep(v, k, stage, step, GlobalId, length, _cmp_func, offset);
        }

        template <
            typename T,
            typename K,
            typename BinaryOperation
        >
        __global__ void bitonicMergeSortKeyPerGroup(T* v, K* k, uint startStage, uint endStage, uint startStep, uint length, BinaryOperation _cmp_func)
        {
            uint tId = threadIdx.x;
            uint i = 2 * GlobalId;

            if(i >= length)
            {
                return;
            }

            /*ShrdMemory<KVPair<K, T>> shrdMem;
            KVPair<K, T>* shrd = shrdMem.Ptr();

            K k0 = key[i + 0];
            T v0 = v[k0];

            shrd[2 * tId + 0] = KVPair<K, T>(k0, v0);
            if(i+1 < length)
            {
                K k1 = key[i + 1];
                T v1 = v[k1];
                shrd[2 * tId + 1] = KVPair<K, T>(k1, v1);
            }*/

            uint step = startStep;

            for(uint stage = startStage; stage <= endStage;)
            {
                for(;step > 0; step = step >> 1)
                {
                    __syncthreads();
                    __bitonicMergeSortKeyStep(v, k, stage, step, tId, length, _cmp_func);
                }
                stage <<= 1;
                step = stage >> 1;
            }

            /*key[i + 0] = shrd[2 * tId + 0].k;
            v[i + 0] = shrd[2 * tId + 0].v;
            if(i+1 < length)
            {
                key[i + 1] = shrd[2 * tId + 1].k;
                v[i + 1] = shrd[2 * tId + 1].v;
            }*/
        }

        template <
            typename IteratorKey,
            typename IteratorData,
            typename BinaryOperation
        >
        void SortKeyPerGroup(
        IteratorKey& keyBegin, 
        IteratorKey& keyEnd, 
        IteratorData& values, 
        uint elementsPerBlock, uint startStage, uint endstage, uint startStep, uint length, BinaryOperation op)
        {
            dim3 block = (elementsPerBlock + elementsPerBlock%2)/2;
            dim3 grid = GetCudaGrid(length, elementsPerBlock);

            uint shrdMem = 0; //todo

            bitonicMergeSortKeyPerGroup
                <<<grid, block, shrdMem>>>
                (
                values(), keyBegin(), startStage, endstage, startStep, length, op
                );
        }

        template <
            typename IteratorKey,
            typename IteratorData,
            typename BinaryOperation
        >
        void SortKeyStep(
        IteratorKey& keys, 
        IteratorData& values, 
        uint grid, uint block, uint stage, uint step, uint length, BinaryOperation op, uint offset = 0)
        {
            bitonicMergeSortKeyStep<<<grid, block>>>
                (
                values(), keys(), stage, step, length, op
                );
        }
    }
}