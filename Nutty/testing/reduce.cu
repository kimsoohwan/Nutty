#include "test_help.cuh"
#include "../Reduce.h"
#include <sstream>
#include <fstream>
#define _CRTDBG_MAP_ALLOC

#include "../Inc.h"
#include "../ForEach.h"
#include "../Functions.h"


int main(void)
{
#if 1
    _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif 

    //create nutty
    nutty::Init();

    //create device memory (1.048.576 elements)
    nutty::DeviceBuffer<int> data(10);

    nutty::Fill(data.Begin(), data.End(), nutty::unary::Sequence<int>());

    nutty::Reduce(data.Begin(), data.End(), nutty::binary::Max<int>(), 0);

    print(data[0]);
    
    //release nutty
    nutty::Release();

    return 0;
}