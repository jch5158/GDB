#pragma once

// 타일의 가로 세로 길이
#define PERMETER_OF_SQUARE 15

// 세로 타일 개수
#define MAX_HEIGHT 40

// 가로 타일 개수
#define MAX_WIDTH 90


extern HBRUSH       oldBrush;

// 오픈리스트
extern HBRUSH       blueBrush;

// 클로즈 리스트
extern HBRUSH       yellowBrush;

// 출발지 체크브러쉬
extern HBRUSH       greenBrush;

// 목적지 체크브러쉬
extern HBRUSH       redBrush;

// 장애물 체크브러쉬
extern HBRUSH       grayBrush;


extern HBRUSH		routeBrush;


extern HBRUSH brushBlockList[MAX_HEIGHT][MAX_WIDTH];
