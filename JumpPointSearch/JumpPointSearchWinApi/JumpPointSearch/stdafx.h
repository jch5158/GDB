#pragma once

//// 타일의 가로 세로 길이
//#define PERMETER_OF_SQUARE 20
//
//// 세로 타일 개수
//#define MAX_HEIGHT 40
//
//// 가로 타일 개수
//#define MAX_WIDTH 70


#include <iostream>
#include <Windows.h>
#include <time.h>
#include <tchar.h>
#include <locale>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

struct RouteNode
{
	bool mRouteFlag;
	int mPosY;
	int mPosX;
};