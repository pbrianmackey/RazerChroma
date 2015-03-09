//! \example RazerChromaPeakMeter.cpp
// RazerChromaPeakMeter.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include "RazerChromaPeakMeter.h"
#include "RzErrors.h"
#include "RzChromaSDKTypes.h"
#include <iostream>

using namespace ChromaSDK::Keyboard;
using namespace ChromaSDK::Mouse;

typedef RZRESULT (*INIT)(void);
typedef RZRESULT (*UNINIT)(void);
typedef RZRESULT (*CREATEKEYBOARDCUSTOMGRIDEFFECTS)(ChromaSDK::Keyboard::CUSTOM_GRID_EFFECT_TYPE CustomEffects, RZEFFECTID *pEffectId);
typedef RZRESULT (*CREATEMOUSECUSTOMEFFECTS)(ChromaSDK::Mouse::CUSTOM_EFFECT_TYPE CustomEffect, RZEFFECTID *pEffectId);
typedef RZRESULT (*DELETEEFFECT)(RZEFFECTID EffectId);
typedef RZRESULT (*SETEFFECT)(RZEFFECTID EffectId);

#ifdef _WIN64
#define CHROMASDKDLL        _T("RzChromaSDK64.dll")
#else
#define CHROMASDKDLL        _T("RzChromaSDK.dll")
#endif

#define MAX_LOADSTRING  100

const COLORREF BLACK = RGB(0,0,0);
const COLORREF WHITE = RGB(255,255,255);
const COLORREF RED = RGB(255,0,0);
const COLORREF GREEN = RGB(0,255,0);
const COLORREF BLUE = RGB(0,0,255);
const COLORREF YELLOW = RGB(255,255,0);
const COLORREF PURPLE = RGB(128,0,128);
const COLORREF CYAN = RGB(00,255,255);
const COLORREF ORANGE = RGB(255,165,00);
const COLORREF PINK = RGB(255,192,203);

// Global Variables:
HINSTANCE hInst = NULL;                         // Current instance.
HWND hWnd = NULL;                               // Main window handle.
HMODULE hModule = NULL;                         // Chroma SDK module handle.
TCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // The main window class name
IAudioMeterInformation *pMeterInfo = NULL;
RZEFFECTID EffectIdClear = GUID_NULL;
COLORREF PresetColor = RED;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
void                ExitInstance(void);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                ShowPeakMeter(float peak);
void				CustomLogic();

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

     // TODO: Place code here.
    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_RAZERCHROMAPEAKMETER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RAZERCHROMAPEAKMETER));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    ExitInstance();

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RAZERCHROMAPEAKMETER));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = MAKEINTRESOURCE(IDC_RAZERCHROMAPEAKMETER);
    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Dynamically loads the Chroma SDK library.
    hModule = LoadLibrary(CHROMASDKDLL);
    if(hModule)
    {
        INIT Init = (INIT)GetProcAddress(hModule, "Init");
        if(Init)
        {
            Init();
        }
    }

    CoInitialize(NULL);

    // Get enumerator for audio endpoint devices.
    IMMDeviceEnumerator *pEnumerator = NULL;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
                                  NULL, CLSCTX_INPROC_SERVER,
                                  __uuidof(IMMDeviceEnumerator),
                                  (void**)&pEnumerator);

    if(FAILED(hr)) goto Exit;

    // Get peak meter for default audio-rendering device.
    IMMDevice *pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);

    if(FAILED(hr)) goto Exit;

    hr = pDevice->Activate(__uuidof(IAudioMeterInformation),
                           CLSCTX_ALL, NULL, (void**)&pMeterInfo);

    if(FAILED(hr)) goto Exit;

    if(pMeterInfo)
    {
        //SetTimer(hWnd, 1, 100, NULL);
    }

Exit:

    if(pDevice)
    {
        pDevice->Release();
        pDevice = NULL;
    }

    if(pEnumerator)
    {
        pEnumerator->Release();
        pEnumerator = NULL;
    }

    return TRUE;
}

void ExitInstance()
{
    if(!IsEqualGUID(EffectIdClear, GUID_NULL))
    {
        DELETEEFFECT DeleteEffect = (DELETEEFFECT)GetProcAddress(hModule, "DeleteEffect");
        if(DeleteEffect)
        {
            DeleteEffect(EffectIdClear);
        }
    }

    if(hModule)
    {
        UNINIT UnInit = (UNINIT)GetProcAddress(hModule, "UnInit");
        if(UnInit)
        {
            UnInit();
        }
    }

    KillTimer(hWnd, 1);

    if(pMeterInfo)
    {
        pMeterInfo->Release();
        pMeterInfo = NULL;
    }

    CoUninitialize();
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    float Peak = 0;

    switch (message)
    {
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_TIMER:
        if(SUCCEEDED(pMeterInfo->GetPeakValue(&Peak)))
        {
            ShowPeakMeter(Peak);
        }
        break;
    case WM_KEYUP:
        // Change the color of the meter.
        switch(wParam)
        {
        case VK_NUMPAD1: PresetColor = RED; break;
        case VK_NUMPAD2: PresetColor = GREEN; break;
        case VK_NUMPAD3: PresetColor = BLUE; break;
        case VK_NUMPAD4: PresetColor = YELLOW; break;
        case VK_NUMPAD5: PresetColor = CYAN; break;
        case VK_NUMPAD6: PresetColor = PINK; break;
        case VK_NUMPAD7: PresetColor = WHITE; break;
        case VK_NUMPAD8: PresetColor = ORANGE; break;
        case VK_NUMPAD9: PresetColor = PURPLE; break;
		case 0x49: 
			CustomLogic();
			break;
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void CustomLogic()
{
	std::cout << "Hello World!";
	CREATEKEYBOARDCUSTOMGRIDEFFECTS CreateKeyboardCustomGridEffects = (CREATEKEYBOARDCUSTOMGRIDEFFECTS)GetProcAddress(hModule, "CreateKeyboardCustomGridEffects");

	CUSTOM_GRID_EFFECT_TYPE Grid = {};

    // Presets
    Grid.Key[4][18] = RED;
	int r = rand() % 255;
	int g = rand() % 255;
	int b = rand() % 255;
	Grid.Key[4][18] = RGB(r,g,b);

	int ROWS = 6;
	int COLUMNS = 22;
	for(int i = 0; i < ROWS; i++)
	{
		int r = rand() % 255;
		int g = rand() % 255;
		int b = rand() % 255;
		

		for(int j = 0; j < COLUMNS; j++)
		{
			Grid.Key[i][j] = RGB(r,g,b);
		}
		
	}
	CreateKeyboardCustomGridEffects(Grid, nullptr);
}

// Message handler for about box.
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

void ShowPeakMeter(float Peak)
{
    CREATEKEYBOARDCUSTOMGRIDEFFECTS CreateKeyboardCustomGridEffects = (CREATEKEYBOARDCUSTOMGRIDEFFECTS)GetProcAddress(hModule, "CreateKeyboardCustomGridEffects");
    CREATEMOUSECUSTOMEFFECTS CreateMouseCustomEffects = (CREATEMOUSECUSTOMEFFECTS)GetProcAddress(hModule, "CreateMouseCustomEffects");

    if((CreateKeyboardCustomGridEffects == nullptr) ||
        (CreateMouseCustomEffects == nullptr))
    {
        return;
    }

    CUSTOM_GRID_EFFECT_TYPE Grid = {};

    // Presets
    Grid.Key[4][18] = RED;
    Grid.Key[4][19] = GREEN;
    Grid.Key[4][20] = BLUE;
    Grid.Key[3][18] = YELLOW;
    Grid.Key[3][19] = CYAN;
    Grid.Key[3][20] = PINK;
    Grid.Key[2][18] = WHITE;
    Grid.Key[2][19] = ORANGE;
    Grid.Key[2][20] = PURPLE;

    UINT NumKeys = 0;
    FLOAT Meter = Peak*100;
    FLOAT Step = FLOAT(100.00/12.00);

    // Count the number of steps for the peak meter.
    for(UINT i=0; i<12; i++)
    {
        if(Meter > (Step*i))
        {
            NumKeys++;
        }
    }

    // Fill the keys with colors.
    if(NumKeys > 0)
    {
        // Fill the meter.
        UINT i=0;
        for(i=0; i<NumKeys; i++)
        {
            Grid.Key[0][3+i] = PresetColor;
        }

        // Fill the rest with black color.
        for(UINT j=i; j<12; j++)
        {
            Grid.Key[0][3+j] = BLACK;
        }
    }
    else if(NumKeys == 0)
    {
        for(UINT i=0; i<12; i++)
        {
            Grid.Key[0][i] = BLACK;
        }
    }

    // Simple visualizer
    // Change the level of brightness according to the level.
    for(UINT Row=1; Row<6; Row++)
    {
        for(UINT Col=1; Col<15; Col++)
        {
            Grid.Key[Row][Col] = RGB((255.00*((FLOAT)NumKeys/12.00)), (255.00*((FLOAT)NumKeys/12.00)), (255.00*((FLOAT)NumKeys/12.00)));
        }
    }

    // Horizontal running lights.
    static UINT Row = 1;
    static UINT Col = 1;
    static INT ColDiff = 1;
    static INT RowDiff = 1;

    static COLORREF RunningLights = YELLOW;
    if(Col >= 14)
    {
        ColDiff = -1;
        RunningLights = RED;
    }
    else if(Col <= 1)
    {
        ColDiff = 1;
        RunningLights = PURPLE;
    }

    Col += ColDiff;

    Grid.Key[1][Col] = RunningLights;
    if((Col + ColDiff < 15) && (Col + ColDiff > 0))
    {
        Grid.Key[1][Col+ColDiff] = RunningLights;
    }

    Grid.Key[2][Col] = RunningLights;
    if((Col + ColDiff < 15) && (Col + ColDiff > 0))
    {
        Grid.Key[2][Col+ColDiff] = RunningLights;
    }

    // Set the keyboard effects.
    CreateKeyboardCustomGridEffects(Grid, nullptr);

    // Set the mouse logo color.
    ChromaSDK::Mouse::CUSTOM_EFFECT_TYPE MouseEffect = {};
    MouseEffect.LED = RZLED_LOGO;
    MouseEffect.Color = RGB((GetRValue(PresetColor)*((FLOAT)NumKeys/12.00)), (GetGValue(PresetColor)*((FLOAT)NumKeys/12.00)), (GetBValue(PresetColor)*((FLOAT)NumKeys/12.00)));;

    CreateMouseCustomEffects(MouseEffect, nullptr);

    // Set the mouse scroll wheel effect.
    ChromaSDK::Mouse::CUSTOM_EFFECT_TYPE MouseScrollWheel = {};
    MouseScrollWheel.LED = RZLED_SCROLLWHEEL;
    MouseScrollWheel.Color = RunningLights;

    CreateMouseCustomEffects(MouseScrollWheel, nullptr);
}
