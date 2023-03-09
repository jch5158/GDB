#include "stdafx.h"


constexpr int MAP_WIDTH = 90;
constexpr int MAP_HEIGHT = 40;


void ReadMapFile(const WCHAR* pFildName, char(*nodeMap)[MAP_WIDTH]);

void TestPathFind(const WCHAR* pFildName, const WCHAR* pTag);

int wmain()
{
    CTLSPerformanceProfiler::SetPerformanceProfiler(L"JumpPointSearch", 1);

    TestPathFind(L"..\\..\\Map\\Map_1", L"Map_1 PathFind");

    TestPathFind(L"..\\..\\Map\\Map_2", L"Map_2 PathFind");

    TestPathFind(L"..\\..\\Map\\Map_3", L"Map_3 PathFind");

    CTLSPerformanceProfiler::PrintPerformanceProfile();
    CTLSPerformanceProfiler::FreePerformanceProfiler();

    return 1;
}


void ReadMapFile(const WCHAR* fileName, char(*nodeMap)[MAP_WIDTH])
{
    FILE* fp = nullptr;

    _wfopen_s(&fp, fileName, L"rb");
    if (fp == nullptr)
    {
        CCrashDump::Crash();

        return;
    }

    fread(nodeMap, 1, MAP_WIDTH * MAP_HEIGHT, fp);

    fclose(fp);

    return;
}


void TestPathFind(const WCHAR* pFildName, const WCHAR* pTag)
{
    int startX;
    int startY;

    int destX;
    int destY;

    CJumpPointSearch jpsObj(MAP_WIDTH, MAP_HEIGHT);

    std::vector<CJumpPointSearch::stRouteNode> route;

    char nodeMap[MAP_HEIGHT][MAP_WIDTH] = { 0, };

    ReadMapFile(pFildName, nodeMap);

    for (int indexY = 0; indexY < MAP_HEIGHT; ++indexY)
    {
        for (int indexX = 0; indexX < MAP_WIDTH; ++indexX)
        {
            if (nodeMap[indexY][indexX] == (char)CJumpPointSearch::eNodeAttribute::NODE_START_POINT)
            {
                startX = indexX;
                startY = indexY;
            }
            else if (nodeMap[indexY][indexX] == (char)CJumpPointSearch::eNodeAttribute::NODE_DEST_POINT)
            {
                destX = indexX;
                destY = indexY;
            }
            else if (nodeMap[indexY][indexX] == (char)CJumpPointSearch::eNodeAttribute::NODE_BLOCK)
            {
                if (jpsObj.SetMapAttribute(indexX, indexY, CJumpPointSearch::eNodeAttribute::NODE_BLOCK) == false)
                    CCrashDump::Crash();
            }
        }
    }


    for (int cnt = 0; cnt < 100; ++cnt)
    {
        CTLSPerformanceProfiler profiler(pTag);

        for (int index = 0; index < 100; ++index)
        {
            if (jpsObj.PathFind(startX, startY, destX, destY, route) == false)
                CCrashDump::Crash();
        }
    }


    return;
}