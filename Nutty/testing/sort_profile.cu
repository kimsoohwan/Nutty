#include <windows.h>
#include "../Nutty.h"
#include "../Fill.h"
#include "../Sort.h"
#include <sstream>
#include <fstream>
#include "../Inc.h"
#include "../ForEach.h"
#include "../Functions.h"
#include "../cuTimer.h"

#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/generate.h>
#include <thrust/sort.h>

struct sortinfo
{
    uint pos;
    int a;
    int b;
};

template < typename IT>
bool checkSort(IT& b, uint size, sortinfo* si)
{
    nutty::HostBuffer<int> cpy(b.Size());
    nutty::Copy(cpy.Begin(), b.Begin(), size);
    auto it = cpy.Begin();
    int i = 0;
    int _cc = 0;
    while(it != cpy.End())
    {
        int cc = *it;
        if(cc < i)
        {
            return false;
        }
        i = cc;
        it++;
        _cc++;
    }
    return true;
}

int main(void)
{
    #define _CRTDBG_MAP_ALLOC

#if 1
    _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif

    nutty::Init();

    DEVICE_SYNC_CHECK();

    cudaDeviceProp props;

    cudaGetDeviceProperties(&props, 0);
    
    std::ofstream profile("sorting_profile.txt");

    profile << "maxgridx=" << props.maxGridSize[0] << "\n";

    profile << "Elems\nTime\n\n";

    std::vector<double> times;

    nutty::cuTimer timer;

    int runs = 1;
    uint startBit = 8;
    uint maxBit = 25;
    uint ivalStep = 1e6;
    srand(0);

    profile << "runs: " << runs << "\n\n";

    for(uint i = startBit; i < maxBit; ++i)
    {
        times.push_back(0);
        profile << (1 << i) << "\n";
    }

    bool error = false;
    for(int k = 0; k < runs; k++)
    {
        if(error)
        {
            break;
        }

        for(int i = startBit; i < maxBit; ++i)
        {
            uint elems = 1 << i;

            nutty::DeviceBuffer<uint> a(elems);

            nutty::Fill(a.Begin(), a.End(), rand);

            timer.Start();

            nutty::Sort(a.Begin(), a.End(), nutty::BinaryDescending<int>());

            cudaError_t err = cudaDeviceSynchronize();

            timer.Stop();

            if(err != cudaSuccess)
            {
                profile << elems << " -> " << cudaGetErrorString(err) << "\n\n";
                error = true;
                break;
            }
 
            times[i - startBit] += timer.GetMillis() / 1000.0; 
        }
    }

    for(int i = 0; i < maxBit - startBit; ++i)
    {
        times[i] = times[i] / (double) runs;
    }

    profile << "\n\n";

    for(auto& it : times)
    {
        profile << it << "\n";
    }

    profile.close();

    nutty::Release();

    return 0;
}