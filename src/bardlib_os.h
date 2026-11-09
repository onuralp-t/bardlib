#ifndef BARDLIB_OS_H_
#define BARDLIB_OS_H_

#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>

int user_main();

size_t strlen(const char *s);
inline void* memcpy(void* dst, const void* src, size_t n);
inline void* memmove(void* dst, const void* src, size_t n);
inline void* memset(void* dst, int c, size_t n);
inline int memcmp(const void* p1, const void* p2, size_t n);

uint32_t puts(char* msg);
__attribute__((format(printf, 1, 2))) void printf(char* fmt, ...); // 1-indexed position of fmt string, and the variadic params

void bard_os_init();
void bard_os_exit(int64_t exitcode);

void bard_create_window(uint64_t window_width, uint64_t window_height, const char* window_title);
typedef struct Bard_Window {
    const char* title;
    uint64_t width;
    uint64_t height;
    uint32_t* pixels;
} Bard_Window;

void bard_poll_events();

#endif // BARDLIB_OS_H_

#define BARDLIB_OS_IMPLEMENTATION // TODO: reomve
#ifdef BARDLIB_OS_IMPLEMENTATION

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#pragma clang diagnostic ignored "-Wextra"
#pragma clang diagnostic ignored "-Wpedantic"
#define STB_SPRINTF_IMPLEMENTATION
#include "../thirdparty/stb_sprintf.h"
#pragma clang diagnostic pop

int _fltused = 0;
size_t strlen(const char *s) {
	const char *a = s;
	for (; *s; s++);
	return s-a;
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
typedef void* (__cdecl *RtlCopyMemory_)(void*, const void*, size_t);
typedef void* (__cdecl *RtlMoveMemory_)(void*, const void*, size_t);
typedef void* (__cdecl *RtlFillMemory_)(void*, size_t, int);
typedef size_t (__cdecl *RtlCompareMemory_)(const void*, const void*, size_t);

typedef int (__cdecl *RtlQueryPerformanceFrequency_)(uint64_t*);
typedef int (__cdecl *RtlQueryPerformanceCounter_)(uint64_t*);

typedef struct Win32_Window {
    HINSTANCE instance;
    WNDCLASS window_class;
    WNDPROC window_proc;
    HWND window;
    BITMAPINFO bitmap_info;
} Win32_Window;

// declare/initialize globals in implementation
RtlCopyMemory_ win32_memcpy;
RtlMoveMemory_ win32_memmove;
RtlFillMemory_ win32_memset;
RtlCompareMemory_ win32_memcmp;

RtlQueryPerformanceFrequency_ win32_hpt_frequency;
RtlQueryPerformanceCounter_ win32_hpt_counter;

HANDLE win32_stdout;

Win32_Window win32_window; // TODO: assumes single window? or is this just main window? idk
Bard_Window bard_window;

void bard_os_init() {
    HMODULE ntdll_handle = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll_handle) bard_os_exit(1);

    FARPROC win32_memcpy_fptr = GetProcAddress(ntdll_handle, "RtlCopyMemory");
    win32_memcpy = (RtlCopyMemory_)win32_memcpy_fptr;

    FARPROC win32_memmove_fptr = GetProcAddress(ntdll_handle, "RtlMoveMemory");
    win32_memmove = (RtlMoveMemory_)win32_memmove_fptr;

    FARPROC win32_memset_fptr = GetProcAddress(ntdll_handle, "RtlFillMemory");
    win32_memset = (RtlFillMemory_)win32_memset_fptr;

    FARPROC win32_memcmp_fptr = GetProcAddress(ntdll_handle, "RtlCompareMemory");
    win32_memcmp = (RtlCompareMemory_)win32_memcmp_fptr;

    FARPROC win32_hpt_frequency_fptr = GetProcAddress(ntdll_handle, "RtlQueryPerformanceFrequency");
    win32_hpt_frequency = (RtlQueryPerformanceFrequency_)win32_hpt_frequency_fptr;

    FARPROC win32_hpt_counter_fptr = GetProcAddress(ntdll_handle, "RtlQueryPerformanceCounter");
    win32_hpt_counter = (RtlQueryPerformanceCounter_)win32_hpt_counter_fptr;

    if (!win32_memcpy_fptr || !win32_memmove_fptr || !win32_memset_fptr || !win32_memcmp_fptr ||
        !win32_hpt_frequency_fptr || !win32_hpt_counter_fptr) {
            bard_os_exit(1);
        }

    win32_stdout = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleOutputCP(CP_UTF8);
}

void bard_os_exit(int64_t exitcode) {
    ExitProcess(exitcode);
}

inline void* memcpy(void* dst, const void* src, size_t n) {return win32_memcpy(dst, src, n);}
inline void* memmove(void* dst, const void* src, size_t n) {return win32_memmove(dst, src, n);}
inline void* memset(void* dst, int c, size_t n) {return win32_memset(dst, n, c);}
inline int memcmp(const void* p1, const void* p2, size_t n) {
    size_t same = win32_memcmp(p1, p2, n);
    if (same == n) return 0;
    const uint8_t *a = (const uint8_t*)p1 + same;
    const uint8_t *b = (const uint8_t*)p2 + same;
    return (*a > *b) - (*a < *b);
}

uint32_t puts(char* msg) {
    uint32_t bytes_written = 0;
    WriteFile(win32_stdout, msg, (DWORD)strlen(msg), (LPDWORD)&bytes_written, NULL);
    return bytes_written;
}

__attribute__((format(printf, 1, 2))) // 1-indexed position of fmt string, and the variadic params
void printf(char* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    stbsp_vsprintf(win32_stdout, fmt, args);
    puts(win32_stdout);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE: [[fallthrough]]; // user attempted to close the window
        case WM_DESTROY: // the closed window's resources are getting released, thread is quiting (sends WM_QUIT)
        {
            // DestroyWindow(hwnd);
            PostQuitMessage(0);
        } break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            int32_t windowWidth = clientRect.right - clientRect.left;
            int32_t windowHeight = clientRect.bottom - clientRect.top;

            StretchDIBits(
                hdc,
                0, 0, windowWidth, windowHeight,
                0, 0, bard_window.width, bard_window.height,
                bard_window.pixels,
                &win32_window.bitmap_info,
                DIB_RGB_COLORS,
                SRCCOPY
            );

            EndPaint(hwnd, &ps);
            return 0;
        } break;

        default: {
            return DefWindowProcA(hwnd, uMsg, wParam, lParam);
        } break;
    }
    return 0;
}

void bard_create_window(uint64_t window_width, uint64_t window_height, const char* window_title) {
    bard_window.title = window_title;
    bard_window.width = window_width;
    bard_window.height = window_height;

    HINSTANCE hinstance = GetModuleHandleA(NULL);

    const char* CLASS_NAME = window_title; // this doesn't determine title name

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hinstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursorA(NULL, IDC_ARROW); // first param is the location of the cursor resource

    win32_window.window_proc = WindowProc;

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        window_title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hinstance, &bard_window
    );

    win32_window.instance = hinstance;
    win32_window.window_class = wc;
    win32_window.window = hwnd;
    win32_window.window_proc = WindowProc;

    bard_window.pixels = (uint32_t*)VirtualAlloc(NULL, window_width * window_height * sizeof(uint32_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    win32_window.bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    win32_window.bitmap_info.bmiHeader.biWidth = window_width;
    win32_window.bitmap_info.bmiHeader.biHeight = -window_height; // neg -> making (0,0) top left
    win32_window.bitmap_info.bmiHeader.biPlanes = 1;
    win32_window.bitmap_info.bmiHeader.biBitCount = 32;
    win32_window.bitmap_info.bmiHeader.biCompression = BI_RGB;

    ShowWindow(win32_window.window, SW_NORMAL);
}

/* void bard_poll_events() {
    MSG msg = {0};

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT) {
            global_state.window_should_close = true;
        }

        TranslateMessage(&msg);
        DispatchMessageA(&msg); // calls WindowProc
    }
} */
#endif // _WIN32

__attribute((force_align_arg_pointer, used, noreturn))
void entry_point(void)
{
    bard_os_init();

    int result = user_main();

    ExitProcess(result);
}
#endif // BARDLIB_OS_IMPLEMENTATION
