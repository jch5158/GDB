#pragma once

//// Ÿ���� ���� ���� ����
//#define PERMETER_OF_SQUARE 20
//
//// ���� Ÿ�� ����
//#define MAX_HEIGHT 40
//
//// ���� Ÿ�� ����
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