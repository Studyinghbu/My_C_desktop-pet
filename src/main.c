#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// AlphaBlend 需要
#pragma comment(lib, "Msimg32.lib")

// 菜单项ID
#define IDM_SMALL   100
#define IDM_MEDIUM  101
#define IDM_LARGE   102
#define IDM_EXIT    103

// 定时器ID
#define IDT_SPEEDUPDATE   200
#define IDT_EMOTIONUPDATE 201

// 全局变量
HINSTANCE g_hInstance;
HWND g_hWnd;
HDC g_hdcMem;
HBITMAP g_hBmp[9];   // 0-2: 开心(cinna_1~3)，3-5: 一般(cinna_4~6)，6-8: 红温(cinna_7~9)
HBITMAP g_hBmpOld;

// 键盘钩子
HHOOK g_hKeyboardHook;

// 窗口大小和位置
int g_nScreenWidth, g_nScreenHeight;
int g_nWindowWidth  = 200;
int g_nWindowHeight = 200;
int g_nXPos = 100, g_nYPos = 100;

// 图片大小
int g_imgWidth  = 200;
int g_imgHeight = 200;

// 拖动相关
POINT g_ptMouseDown;
BOOL g_bDragging   = FALSE;
BOOL g_bEscPressed = FALSE;

// 统计信息
int g_nKeyCount     = 0;
int g_nLastKeyCount = 0;
int g_nTypingSpeed  = 0;   // char/s
int g_totalChars    = 0;
int g_currentSpeed  = 0;
clock_t g_lastCheckTime = 0;

// 表情状态
int   g_currentEmotion  = 0;    // 0-8
float g_emotionProgress = 0.0f; // 0~1
int   g_targetEmotion   = 0;

// 键盘钩子回调函数
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

// 声明
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL LoadResources();
void UpdateTypingSpeed();
void UpdateEmotion();
void ShowContextMenu(HWND hWnd, LPARAM lParam);
void ResizeWindow(HWND hWnd, int width, int height);
void CleanupResources();

// WinMain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    BOOL bRet;

    srand((unsigned int)time(NULL));

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    if (!LoadResources())
    {
        MessageBox(NULL, TEXT("Failed to load resources!"), TEXT("Error"), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    g_lastCheckTime = clock();

    SetTimer(g_hWnd, IDT_SPEEDUPDATE,   1000, NULL);
    SetTimer(g_hWnd, IDT_EMOTIONUPDATE, 100,  NULL);

    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (bRet == -1)
            break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// 注册窗口类
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = NULL;
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = NULL;
    wcex.lpszClassName = TEXT("DeskPetFace");
    wcex.hIconSm       = NULL;
    return RegisterClassEx(&wcex);
}

// 初始化窗口
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    g_hInstance = hInstance;

    g_nScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
    g_nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    g_hWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        TEXT("DeskPetFace"),
        TEXT("Desk Pet Face"),
        WS_POPUP,
        g_nXPos, g_nYPos, g_nWindowWidth, g_nWindowHeight,
        NULL, NULL, hInstance, NULL);

    if (!g_hWnd)
        return FALSE;

    SetLayeredWindowAttributes(g_hWnd, RGB(255, 255, 255), 255, LWA_COLORKEY | LWA_ALPHA);

    g_hdcMem = CreateCompatibleDC(NULL);

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    return TRUE;
}

// 主窗口过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        RegisterHotKey(hWnd, 1, 0, VK_ESCAPE);
        // 安装全局键盘钩子
        g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, g_hInstance, 0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        if (g_hBmp[g_currentEmotion] && g_hBmp[g_targetEmotion])
        {
            BITMAP bm;
            GetObject(g_hBmp[g_currentEmotion], sizeof(bm), &bm);

            HDC hdcTemp = CreateCompatibleDC(hdc);
            HBITMAP hbmpTemp = CreateCompatibleBitmap(hdc, g_imgWidth, g_imgHeight);
            HBITMAP hbmpOldTemp = (HBITMAP)SelectObject(hdcTemp, hbmpTemp);

            // 先画当前表情
            g_hBmpOld = (HBITMAP)SelectObject(g_hdcMem, g_hBmp[g_currentEmotion]);
            BitBlt(hdcTemp, 0, 0, g_imgWidth, g_imgHeight, g_hdcMem, 0, 0, SRCCOPY);
            SelectObject(g_hdcMem, g_hBmpOld);

            if (g_currentEmotion != g_targetEmotion)
            {
                g_hBmpOld = (HBITMAP)SelectObject(g_hdcMem, g_hBmp[g_targetEmotion]);

                BLENDFUNCTION blend;
                blend.BlendOp             = AC_SRC_OVER;
                blend.BlendFlags          = 0;
                blend.SourceConstantAlpha = (BYTE)(g_emotionProgress * 255);
                blend.AlphaFormat         = 0;

                AlphaBlend(
                    hdcTemp,
                    0, 0, g_imgWidth, g_imgHeight,
                    g_hdcMem,
                    0, 0, g_imgWidth, g_imgHeight,
                    blend);

                SelectObject(g_hdcMem, g_hBmpOld);
            }

            BitBlt(hdc, 0, 0, g_imgWidth, g_imgHeight, hdcTemp, 0, 0, SRCCOPY);

            SelectObject(hdcTemp, hbmpOldTemp);
            DeleteObject(hbmpTemp);
            DeleteDC(hdcTemp);
        }

        // 画文字，在图片下方
        TCHAR buffer[128];
        
        // 先清除文字区域（用白色背景覆盖）
        RECT textRect;
        textRect.left = 0;
        textRect.top = g_imgHeight;
        textRect.right = g_nWindowWidth;
        textRect.bottom = g_nWindowHeight;
        FillRect(hdc, &textRect, (HBRUSH)GetStockObject(WHITE_BRUSH));
        
        // 设置文字颜色和背景模式
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, RGB(255, 255, 255));

        int textY = g_imgHeight + 5;

        wsprintf(buffer, TEXT("Total Chars: %d"), g_totalChars);
        TextOut(hdc, 10, textY, buffer, lstrlen(buffer));

        wsprintf(buffer, TEXT("Speed: %d char/s"), g_nTypingSpeed);
        TextOut(hdc, 10, textY + 18, buffer, lstrlen(buffer));

        EndPaint(hWnd, &ps);
        return 0;
    }



    case WM_LBUTTONDOWN:
        g_bDragging = TRUE;
        g_ptMouseDown.x = LOWORD(lParam);
        g_ptMouseDown.y = HIWORD(lParam);
        SetCapture(hWnd);
        return 0;

    case WM_MOUSEMOVE:
        if (g_bDragging)
        {
            POINT ptCursor;
            GetCursorPos(&ptCursor);
            g_nXPos = ptCursor.x - g_ptMouseDown.x;
            g_nYPos = ptCursor.y - g_ptMouseDown.y;
            SetWindowPos(hWnd, HWND_TOPMOST, g_nXPos, g_nYPos, 0, 0, SWP_NOSIZE);
        }
        return 0;

    case WM_LBUTTONUP:
        g_bDragging = FALSE;
        ReleaseCapture();
        return 0;

    case WM_HOTKEY:
        if (wParam == 1)
            PostMessage(hWnd, WM_CLOSE, 0, 0);
        return 0;

    case WM_RBUTTONDOWN:
        ShowContextMenu(hWnd, lParam);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_SMALL:
            ResizeWindow(hWnd, 150, 150);
            break;
        case IDM_MEDIUM:
            ResizeWindow(hWnd, 200, 200);
            break;
        case IDM_LARGE:
            ResizeWindow(hWnd, 250, 250);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        }
        return 0;

    case WM_TIMER:
        if (wParam == IDT_SPEEDUPDATE)
        {
            UpdateTypingSpeed();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (wParam == IDT_EMOTIONUPDATE)
        {
            UpdateEmotion();
        }
        return 0;

    case WM_DESTROY:
        CleanupResources();
        // 卸载键盘钩子
        if (g_hKeyboardHook)
            UnhookWindowsHookEx(g_hKeyboardHook);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// 加载 9 张 bmp，并根据尺寸调整窗口
BOOL LoadResources()
{
    WCHAR szExePath[MAX_PATH];
    GetModuleFileNameW(NULL, szExePath, MAX_PATH);

    WCHAR szExeDir[MAX_PATH];
    WCHAR* pSlash = wcsrchr(szExePath, L'\\');
    if (pSlash)
    {
        *pSlash = L'\0';
        wcsncpy(szExeDir, szExePath, MAX_PATH - 1);
        szExeDir[MAX_PATH - 1] = L'\0';
    }
    else
    {
        MessageBoxW(NULL, L"Failed to get executable directory!", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // exe 在 bin 下，res 在上一层的 res 目录
    WCHAR szResDir[MAX_PATH];
    wsprintfW(szResDir, L"%s\\..\\res", szExeDir);

    DWORD dwAttr = GetFileAttributesW(szResDir);
    if (dwAttr == INVALID_FILE_ATTRIBUTES || !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        WCHAR szError[MAX_PATH + 64];
        wsprintfW(szError, L"res directory not found at: %s", szResDir);
        MessageBoxW(NULL, szError, L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    const WCHAR* imageNames[9] = {
        L"cinna_1.bmp",
        L"cinna_2.bmp",
        L"cinna_3.bmp",
        L"cinna_4.bmp",
        L"cinna_5.bmp",
        L"cinna_6.bmp",
        L"cinna_7.bmp",
        L"cinna_8.bmp",
        L"cinna_9.bmp"
    };

    WCHAR szImagePath[MAX_PATH];

    for (int i = 0; i < 9; ++i)
    {
        wsprintfW(szImagePath, L"%s\\%s", szResDir, imageNames[i]);

        g_hBmp[i] = (HBITMAP)LoadImageW(
            NULL,
            szImagePath,
            IMAGE_BITMAP,
            0, 0,
            LR_LOADFROMFILE);

        if (!g_hBmp[i])
        {
            WCHAR szError[512];
            wsprintfW(szError, L"Failed to load image: %s", szImagePath);
            MessageBoxW(NULL, szError, L"Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
    }

    // 用第一张图的尺寸作为基准，调整窗口大小
    BITMAP bm;
    GetObject(g_hBmp[0], sizeof(bm), &bm);
    g_imgWidth  = bm.bmWidth;
    g_imgHeight = bm.bmHeight;

    g_nWindowWidth  = g_imgWidth;
    g_nWindowHeight = g_imgHeight + 40; // 下面 40 像素放文字

    SetWindowPos(
        g_hWnd,
        HWND_TOPMOST,
        g_nXPos, g_nYPos,
        g_nWindowWidth, g_nWindowHeight,
        SWP_SHOWWINDOW
    );

    return TRUE;
}

// 每 5 秒更新一次打字速度，并随机选表情
void UpdateTypingSpeed()
{
    clock_t currentTime = clock();
    double elapsedTime = (double)(currentTime - g_lastCheckTime) / CLOCKS_PER_SEC;

    if (elapsedTime >= 5.0)
    {
        int charCount = g_nKeyCount - g_nLastKeyCount;
        g_nTypingSpeed = (int)(charCount / elapsedTime);
        g_nLastKeyCount = g_nKeyCount;
        g_lastCheckTime = currentTime;

        g_currentSpeed = g_nTypingSpeed;

        int baseIndex = 0;

        if (g_nTypingSpeed >= 9)
            baseIndex = 6;          // cinna_7~9
        else if (g_nTypingSpeed >= 4)
            baseIndex = 3;          // cinna_4~6
        else
            baseIndex = 0;          // cinna_1~3

        int newTarget = baseIndex + (rand() % 3);
        g_targetEmotion   = newTarget;
        g_emotionProgress = 0.0f;
    }
}

// 每 100ms 做表情平滑过渡
void UpdateEmotion()
{
    if (g_currentEmotion != g_targetEmotion)
    {
        g_emotionProgress += 0.05f;

        if (g_emotionProgress >= 1.0f)
        {
            g_currentEmotion  = g_targetEmotion;
            g_emotionProgress = 0.0f;
        }

        InvalidateRect(g_hWnd, NULL, FALSE);
    }
}

// 右键菜单
void ShowContextMenu(HWND hWnd, LPARAM lParam)
{
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppendMenu(hMenu, MF_STRING, IDM_SMALL,  TEXT("Small"));
    AppendMenu(hMenu, MF_STRING, IDM_MEDIUM, TEXT("Medium"));
    AppendMenu(hMenu, MF_STRING, IDM_LARGE,  TEXT("Large"));
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, IDM_EXIT,   TEXT("Exit"));

    POINT pt;
    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);
    ClientToScreen(hWnd, &pt);

    TrackPopupMenu(
        hMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN,
        pt.x, pt.y,
        0, hWnd, NULL);

    DestroyMenu(hMenu);
}

// 改变窗口尺寸（目前只简单缩放）
void ResizeWindow(HWND hWnd, int width, int height)
{
    g_nWindowWidth  = width;
    g_nWindowHeight = height;

    SetWindowPos(
        hWnd,
        HWND_TOPMOST,
        g_nXPos, g_nYPos,
        width, height,
        SWP_SHOWWINDOW);

    InvalidateRect(hWnd, NULL, FALSE);
}

// 键盘钩子回调函数
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT* pKbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN)
        {
            // 只统计可打印字符（排除功能键）
            if (pKbdStruct->vkCode >= 0x20 && pKbdStruct->vkCode <= 0x7E)
            {
                g_nKeyCount++;
                g_totalChars++;
            }
        }
    }
    // 调用下一个钩子
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

// 清理资源
void CleanupResources()
{
    for (int i = 0; i < 9; ++i)
    {
        if (g_hBmp[i])
        {
            DeleteObject(g_hBmp[i]);
            g_hBmp[i] = NULL;
        }
    }

    if (g_hdcMem)
    {
        DeleteDC(g_hdcMem);
        g_hdcMem = NULL;
    }

    if (g_hWnd)
    {
        KillTimer(g_hWnd, IDT_SPEEDUPDATE);
        KillTimer(g_hWnd, IDT_EMOTIONUPDATE);
        UnregisterHotKey(g_hWnd, 1);
    }
}
