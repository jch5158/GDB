// JumpPointSearchAlgorithm.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//


#include "stdafx.h"
#include "resource.h"
#include "framework.h"
#include "CJumpPointSearch.h"
#include "BresenhamLine.h"
#include "JumpPointSearchAlgorithm.h"


#define ID_COMBOBOX 100
#define MAX_LOADSTRING 100

enum class eMapAttribute
{
    NODE_UNBLOCK,
    NODE_BLOCK,
    NODE_START_POINT,
    NODE_DESTINATION_POINT
};


// 전역 변수:
HINSTANCE           hInst;                                // 현재 인스턴스입니다.
WCHAR               szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR               szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

HWND         hWnd;
HDC          hdc;


HBRUSH       oldBrush;

// 오픈리스트
HBRUSH       blueBrush;

// 클로즈 리스트
HBRUSH       yellowBrush;

// 출발지 체크브러쉬
HBRUSH       greenBrush;

// 목적지 체크브러쉬
HBRUSH       redBrush;

// 장애물 체크브러쉬
HBRUSH       grayBrush;

// 브레즌헴 라인
HBRUSH       routeBrush;

// red 색상이 칠해진 타일
int                redX = -1;
int                redY = -1;

// green 색상이 칠해진 타일
int                greenX = -1;
int                greenY = -1;

bool               returnFlag = false;

bool               pathFineFlag = true;

CJumpPointSearch    jspObject(MAX_WIDTH, MAX_HEIGHT);


RouteNode          routeNodeArray[100];
RouteNode          optimizeNodeArray[100];


//CList<JumpPointSearch::NODE*> routeList;
//
//CList<JumpPointSearch::NODE*> optimizeRouteList;

HBRUSH brushBlockList[MAX_HEIGHT][MAX_WIDTH];

char mapBuffer[MAX_HEIGHT][MAX_WIDTH];

WCHAR Items[][MAX_PATH] = { L"Map_1",L"Map_2",L"Map_3" };
WCHAR str[MAX_PATH] = { 0, };
HWND hCombo = NULL;

LRESULT gComboBoxNumber = -1;

void SaveMap(void);

void SaveToFile(std::wstring path, std::wstring fileName);

void LoadMap(void);

void ReadMapFile(const WCHAR* fileName);

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void CALLBACK       TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    _wsetlocale(LC_ALL, L"");
    
    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_JUMPPOINTSEARCHALGORITHM, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_JUMPPOINTSEARCHALGORITHM));

    MSG msg;

    srand((unsigned)time(NULL));

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_JUMPPOINTSEARCHALGORITHM));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = nullptr;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 1415, 800, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HANDLE       hTimer; 

    static HPEN         oldPen;
    static HPEN         redPen;
    static HPEN         pinkPen;

    // 마우스 좌표
    static DWORD        mouseX;
    static DWORD        mouseY;

    
    static bool         wallFlag = false;
    static bool         wallClearFlag = false;
    static bool         loopFlag = false;
    

    switch (message)
    {
    case WM_CREATE:

        // 차일드 윈도우이기 때문에 WS_CHILD는 무조건 넣어주어야 한다.  
        // ShowWindow 호출하지 않고 보여야하기 때문에 WS_VISIBLE 넣어줘야 한다.

        CreateWindow(L"button", L"맵 저장", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 500, 630, 100, 80, hWnd, (HMENU)0, hInst, NULL);

        CreateWindow(L"button", L"맵 불러오기", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 800, 630, 100, 80, hWnd, (HMENU)1, hInst, NULL);


        hCombo = CreateWindow(L"combobox", L"map", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            650, 630, 100, 200, hWnd, (HMENU)ID_COMBOBOX, hInst, NULL);
        for (int i = 0; i < 3; i++)
        {
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)Items[i]);
        }

        hdc = GetDC(hWnd);

        greenBrush = CreateSolidBrush(RGB(0, 200, 0));
        
        redBrush = CreateSolidBrush(RGB(255, 0, 0));

        grayBrush = CreateSolidBrush(RGB(105, 105, 105));

        blueBrush = CreateSolidBrush(RGB(0, 0, 255));

        yellowBrush = CreateSolidBrush(RGB(255, 255, 0));

        routeBrush = CreateSolidBrush(RGB(50, 50, 50));

        redPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));

        pinkPen = CreatePen(PS_SOLID, 2, RGB(255, 105, 180));

        hTimer = (HANDLE)SetTimer(hWnd, 1, 50, (TIMERPROC)TimerProc);

        oldBrush = (HBRUSH)SelectObject(hdc, greenBrush);

        for (int indexY = 0; indexY < MAX_HEIGHT; indexY++)
        {
            for (int indexX = 0; indexX < MAX_WIDTH; indexX++)
            {
                brushBlockList[indexY][indexX] = oldBrush;
            }
        }

        SelectObject(hdc, oldBrush);

        ReleaseDC(hWnd, hdc);

        break;
    case WM_COMMAND:
    {
        
        
        // 메뉴 선택을 구문 분석합니다:
        switch (LOWORD(wParam))
        {
        case 0:
            
            if (greenX == -1 || greenY == -1 || redX == -1 || redY == -1)
            {
                MessageBox(hWnd, L"출발지와 목적지를 선택하세요.", L"Button", MB_OK);
            }
            else if (gComboBoxNumber == -1)
            {
                MessageBox(hWnd, L"맵을 선택하세요.", L"Button", MB_OK);
            }
            else
            {
                SaveMap();
            }
            
            SetFocus(hWnd);

            break;

        case 1:
            
            jspObject.ResetAll(routeNodeArray, 100, optimizeNodeArray, 100);

            LoadMap();

            SetFocus(hWnd);

            InvalidateRect(hWnd, nullptr, false);

            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;

        case ID_COMBOBOX:
            switch (HIWORD(wParam)) 
            {

            case CBN_SELCHANGE:
           
                gComboBoxNumber = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
            
                
                SetFocus(hWnd);
                
                break;
            }

            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }        
    }
    break;
    case WM_KEYDOWN:

        if (wParam == VK_RETURN)
        {            
            if (greenX == -1 || greenY == -1 || redX == -1 || redY == -1)
            {
                MessageBox(hWnd, L"출발지와 목적지를 선택하세요", L"Button", MB_OK);
                break;
            }

            jspObject.ReStart(routeNodeArray, 100, optimizeNodeArray, 100);

            for (int iCntY = 0; iCntY < MAX_HEIGHT; ++iCntY)
            {
                for (int iCntX = 0; iCntX < MAX_WIDTH; ++iCntX)
                {
                    if (brushBlockList[iCntY][iCntX] == grayBrush)
                    {
                        jspObject.SettingMapAttribute(iCntX, iCntY);
                    }
                }
            }

            pathFineFlag = true;
        }

        if (wParam == VK_SPACE)
        {
            pathFineFlag = true;

            jspObject.ResetAll(routeNodeArray, 100, optimizeNodeArray, 100);

            greenX = -1;
            greenY = -1;

            redX = -1;
            redY = -1;
        }

        InvalidateRect(hWnd, nullptr, false);

        break;
    case WM_MOUSEMOVE:

        if (wallFlag == true)
        {
            mouseY = HIWORD(lParam);
            mouseX = LOWORD(lParam);

            // 드래그 하여 장애물 그리기
            brushBlockList[mouseY / PERMETER_OF_SQUARE][mouseX / PERMETER_OF_SQUARE] = grayBrush;

            InvalidateRect(hWnd, nullptr, false);            
        }
        // 드래그 하여 장애물 없애기
        else if (wallClearFlag == true)
        {
            mouseY = HIWORD(lParam);
            mouseX = LOWORD(lParam);
            
            brushBlockList[mouseY / PERMETER_OF_SQUARE][mouseX / PERMETER_OF_SQUARE] = oldBrush;

            InvalidateRect(hWnd, nullptr, false);            
        }
       

        break;
    case WM_LBUTTONDOWN:

        mouseY = HIWORD(lParam);
        mouseX = LOWORD(lParam);


        if (MK_CONTROL & wParam)
        {
            // 장애물 클리어
            
            brushBlockList[mouseY / PERMETER_OF_SQUARE][mouseX / PERMETER_OF_SQUARE] = oldBrush;
            
            wallClearFlag = true;
        }
        else if (MK_SHIFT & wParam)
        {
            // 장애물 만들기
            
            brushBlockList[mouseY / PERMETER_OF_SQUARE][mouseX / PERMETER_OF_SQUARE] = grayBrush;
            

            wallFlag = true;
        }
        else
        {  
            
            brushBlockList[greenY][greenX] = oldBrush;

            greenX = mouseX / PERMETER_OF_SQUARE;
            greenY = mouseY / PERMETER_OF_SQUARE;

            brushBlockList[greenY][greenX] = greenBrush;
        }


        InvalidateRect(hWnd, nullptr, false);

        break;

    case WM_RBUTTONDOWN:

        mouseY = HIWORD(lParam);
        mouseX = LOWORD(lParam);

        brushBlockList[redY][redX] = oldBrush;

        redX = mouseX / PERMETER_OF_SQUARE;
        redY = mouseY / PERMETER_OF_SQUARE;

        brushBlockList[redY][redX] = redBrush;

        InvalidateRect(hWnd, nullptr, false);


        break;
    case WM_LBUTTONUP:

        wallFlag = false;
        wallClearFlag = false;

        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        hdc = BeginPaint(hWnd, &ps);

        oldPen = (HPEN)SelectObject(hdc, redPen);

        for (int index = 0; index < 99; ++index)
        {
            if (optimizeNodeArray[index].mRouteFlag == false)
            {
                break;
            }              
            

            if (optimizeNodeArray[index + 1].mRouteFlag == true)
            {
                BresenhamLine::CatchLine(optimizeNodeArray[index].mPosX, optimizeNodeArray[index].mPosY, optimizeNodeArray[index+1].mPosX, optimizeNodeArray[index + 1].mPosY);
            }
            else
            {
                break;
            }
        }

        SelectObject(hdc, oldPen);
        
        for (int iCntY = 0; iCntY < MAX_HEIGHT; iCntY++)
        {
            for (int iCntX = 0; iCntX < MAX_WIDTH; iCntX++)
            {
                oldBrush = (HBRUSH)SelectObject(hdc, brushBlockList[iCntY][iCntX]);

                Rectangle(hdc, PERMETER_OF_SQUARE * iCntX, PERMETER_OF_SQUARE * iCntY, PERMETER_OF_SQUARE * (iCntX + 1), PERMETER_OF_SQUARE * (iCntY + 1));

                SelectObject(hdc, oldBrush);
            }
        }


        oldPen = (HPEN)SelectObject(hdc, redPen);

        for (int index = 0; index < 99; ++index)
        {
            if (routeNodeArray[index].mRouteFlag == true)
            {
                MoveToEx(hdc, 10 + (routeNodeArray[index].mPosX * PERMETER_OF_SQUARE), 10 + (routeNodeArray[index].mPosY * PERMETER_OF_SQUARE), nullptr);
            }
            else
            {
                break;
            }


            if (routeNodeArray[index + 1].mRouteFlag == true)
            {
                LineTo(hdc, 10 + (routeNodeArray[index + 1].mPosX * PERMETER_OF_SQUARE), 10 + (routeNodeArray[index + 1].mPosY * PERMETER_OF_SQUARE));
            }
            else
            {
                break;
            }
        }

        oldPen = (HPEN)SelectObject(hdc, pinkPen);

        for (int index = 0; index < 99; ++index)
        {
            if (optimizeNodeArray[index].mRouteFlag == true)
            {
                MoveToEx(hdc, 10 + (optimizeNodeArray[index].mPosX * PERMETER_OF_SQUARE), 10 + (optimizeNodeArray[index].mPosY * PERMETER_OF_SQUARE), nullptr);
            }
            else
            {
                break;
            }


            if (optimizeNodeArray[index + 1].mRouteFlag == true)
            {
                LineTo(hdc, 10 + (optimizeNodeArray[index + 1].mPosX * PERMETER_OF_SQUARE), 10 + (optimizeNodeArray[index + 1].mPosY * PERMETER_OF_SQUARE));
            }
            else
            {
                break;
            }
        }

        SelectObject(hdc, oldPen);

        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:

        KillTimer(hWnd, 1);


        // PostQuitMessage 함수를 호출하여 WM_QUIT 메시지를 보낸다. WM_QUIT 메시지가 입력되면 
        // 메시지 루프의 GetMessage 함수 리턴값이 False가 되어 프로그램이 종료된다.
        PostQuitMessage(0);
        break;
    default:


        // 윈도우의 이동이나 크기변경 따위의 처리는 직접 해 줄 필요없이 DefWindowProc으로 넘겨주기만 하면 된다.
        // 또한 DefWindowProc 함수가 메시지를 처리했을 경우 이 함수가 리턴한 값을 WndProc 함수가 다시 리턴해 주어야 한다.
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    if (pathFineFlag)
    {
        returnFlag = jspObject.PathFind(greenX, greenY, redX, redY, routeNodeArray, 100, optimizeNodeArray, 100);
        InvalidateRect(hWnd, nullptr, false);
        if (returnFlag)
        {
            pathFineFlag = false;
        }
    }
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


void SaveMap(void)
{

    switch (gComboBoxNumber)
    {
    case 0:

        //SaveToFile(L"..\\..\\Map\\Map_1");
        SaveToFile(L".\\Map", L"Map_1");

    break;

    case 1:
        
        SaveToFile(L".\\Map", L"Map_2");

    break;

    case 2:

        SaveToFile(L".\\Map", L"Map_3");

    break; 
    
    default:

        break;
    }

}

void SaveToFile(std::wstring path, std::wstring fileName)
{
    ZeroMemory(mapBuffer, sizeof(mapBuffer));

    for (int indexY = 0; indexY < MAX_HEIGHT; ++indexY)
    {
        for (int indexX = 0; indexX < MAX_WIDTH; ++indexX)
        {
            
            if (brushBlockList[indexY][indexX] == grayBrush)
            {
                mapBuffer[indexY][indexX] = (char)eMapAttribute::NODE_BLOCK;
            }
            else if (brushBlockList[indexY][indexX] == greenBrush)
            {
                mapBuffer[indexY][indexX] = (char)eMapAttribute::NODE_START_POINT;
            }
            else if (brushBlockList[indexY][indexX] == redBrush)
            {
                mapBuffer[indexY][indexX] = (char)eMapAttribute::NODE_DESTINATION_POINT;
            }
            else
            {
                mapBuffer[indexY][indexX] = (char)eMapAttribute::NODE_UNBLOCK;
            }
        }
    }


    if (!PathFileExists(path.c_str())) {
        bool result = CreateDirectory(path.c_str(), nullptr);
        if (!result) {
            MessageBox(hWnd, L"맵 저장에 실패하였습니다.", L"Button", MB_OK);
            return;
        }
    }

    FILE* fp = nullptr;

    _wfopen_s(&fp, (path + L"\\" + fileName).c_str(), L"wb");

    if (fp == nullptr)
    {
        MessageBox(hWnd, L"맵 저장에 실패하였습니다.", L"Button", MB_OK);
        return;
    }

    fwrite(mapBuffer, sizeof(mapBuffer), 1, fp);

    fclose(fp);

    MessageBox(hWnd, L"맵 저장에 성공하였습니다.", L"Button", MB_OK);

    return;
}

void LoadMap(void)
{
    switch (gComboBoxNumber)
    {
    case 0:

        //ReadMapFile(L"..\\..\\Map\\Map_1");
        ReadMapFile(L".\\Map\\Map_1");

        break;

    case 1:

        //ReadMapFile(L"..\\..\\Map\\Map_2");
        ReadMapFile(L".\\Map\\Map_2");

        break;

    case 2:

        //ReadMapFile(L"..\\..\\Map\\Map_3");
        ReadMapFile(L".\\Map\\Map_3");

        break;

    default:
        break;
    }
}

void ReadMapFile(const WCHAR* fileName)
{
    //ZeroMemory(brushBlockList, sizeof(brushBlockList));
    ZeroMemory(mapBuffer, sizeof(mapBuffer));

    FILE* fp = nullptr;

    _wfopen_s(&fp, fileName, L"rb");

    if (fp == nullptr)
    {
        MessageBox(hWnd, L"맵 파일을 찾지 못하였습니다.", L"Button", MB_OK);
        return;
    }

    fread(mapBuffer, 1, sizeof(mapBuffer), fp);

    fclose(fp);

    for (int indexY = 0; indexY < MAX_HEIGHT; ++indexY)
    {
        for (int indexX = 0; indexX < MAX_WIDTH; ++indexX)
        {
            if (mapBuffer[indexY][indexX] == (char)eMapAttribute::NODE_BLOCK)
            {
                brushBlockList[indexY][indexX] = grayBrush;
            }
            else if (mapBuffer[indexY][indexX] == (char)eMapAttribute::NODE_START_POINT)
            {
                brushBlockList[indexY][indexX] = greenBrush;
                greenY = indexY;
                greenX = indexX;
            }
            else if (mapBuffer[indexY][indexX] == (char)eMapAttribute::NODE_DESTINATION_POINT)
            {
                brushBlockList[indexY][indexX] = redBrush;
                redY = indexY;
                redX = indexX;
            }
        }
    }

    MessageBox(hWnd, L"맵을 불러왔습니다.", L"Button", MB_OK);

    return;
}
