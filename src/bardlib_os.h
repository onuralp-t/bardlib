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

void bard_os_init();
void bard_os_exit();

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

typedef void* (__cdecl *RtlCopyMemory_)(void*, const void*, size_t);
typedef void* (__cdecl *RtlMoveMemory_)(void*, const void*, size_t);
typedef void* (__cdecl *RtlFillMemory_)(void*, size_t, int);
typedef size_t (__cdecl *RtlCompareMemory_)(const void*, const void*, size_t);

typedef int (__cdecl *RtlQueryPerformanceFrequency_)(uint64_t*);
typedef int (__cdecl *RtlQueryPerformanceCounter_)(uint64_t*);

typedef union Win32_Builtins {
    struct {
        RtlCopyMemory_ copy;
        RtlMoveMemory_ move;
        RtlFillMemory_ set;
        RtlCompareMemory_ cmp;
    };
    struct {
        FARPROC fcopy;
        FARPROC fmove;
        FARPROC fset;
        FARPROC fcmp;
    };
} Win32_Builtins;

typedef union Win32_Time_Functions {
    struct {
        RtlQueryPerformanceFrequency_ qpc_freq;
        RtlQueryPerformanceCounter_ qpc_count;
    };
    struct {
        FARPROC fqpc_freq;
        FARPROC fqpc_count;
    };
} Win32_Time_Functions;

typedef struct Win32_Time_State {
    uint64_t freq;
    uint64_t previous_count;
    uint64_t current_count;
} Win32_Time_State;

typedef struct Win32_Stdout {
    char buffer[1024];
    HANDLE stdout_handle;
    DWORD scratch_int;
} Win32_Stdout;

double win32_time_s(void);

void win32_puts(char* msg);

__attribute__((format(printf, 1, 2))) // NOTE: 1-indexed position of fmt string, and the variadic params
void win32_printf(char* fmt, ...);

__attribute((force_align_arg_pointer, used, noreturn))
void entry_point(void);
#endif // _WIN32
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
Win32_Builtins win32_builtins = {0};
Win32_Time_Functions win32_time_functions = {0};
Win32_Time_State win32_time_state = {0};
Win32_Stdout win32_stdout = {0};

inline void* memcpy(void* dst, const void* src, size_t n) {return win32_builtins.copy(dst, src, n);}
inline void* memmove(void* dst, const void* src, size_t n) {return win32_builtins.move(dst, src, n);}
inline void* memset(void* dst, int c, size_t n) {return win32_builtins.set(dst, n, c);}
inline int memcmp(const void* p1, const void* p2, size_t n) {
    size_t same = win32_builtins.cmp(p1, p2, n);
    if (same == n) return 0;
    const uint8_t *a = (const uint8_t*)p1 + same;
    const uint8_t *b = (const uint8_t*)p2 + same;
    return (*a > *b) - (*a < *b);
}

double win32_time_s(void) {
    win32_time_functions.qpc_count((void*)&win32_time_state.current_count);
    double ret = (double)((win32_time_state.current_count - win32_time_state.previous_count) / (double)win32_time_state.freq);
    win32_time_state.previous_count = win32_time_state.current_count;
    return ret;
}

void win32_puts(char* msg) {
    WriteFile(win32_stdout.stdout_handle, msg, (DWORD)strlen(msg), &win32_stdout.scratch_int, NULL);
}

__attribute__((format(printf, 1, 2))) // 1-indexed position of fmt string, and the variadic params
void win32_printf(char* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    stbsp_vsprintf(win32_stdout.buffer, fmt, args);
    win32_puts(win32_stdout.buffer);
}

void bard_os_init() {
    HMODULE ntdll_handle = GetModuleHandleW(L"ntdll.dll");
    // builtins
    win32_builtins.fcopy = GetProcAddress(ntdll_handle, "RtlCopyMemory");
    win32_builtins.fmove = GetProcAddress(ntdll_handle, "RtlMoveMemory");
    win32_builtins.fset = GetProcAddress(ntdll_handle, "RtlFillMemory");
    win32_builtins.fcmp = GetProcAddress(ntdll_handle, "RtlCompareMemory");
    // time
    win32_time_functions.fqpc_freq = GetProcAddress(ntdll_handle, "RtlQueryPerformanceFrequency");
    win32_time_functions.fqpc_count = GetProcAddress(ntdll_handle, "RtlQueryPerformanceCounter");
    // time state init
    win32_time_functions.qpc_count((void*)&win32_time_state.previous_count);
    win32_time_functions.qpc_freq((void*)&win32_time_state.freq);
    // stdout handle
    win32_stdout.stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleOutputCP(CP_UTF8);
}
#endif // _WIN32

__attribute((force_align_arg_pointer, used, noreturn))
void entry_point(void)
{
    bard_os_init();

    int result = user_main();

    ExitProcess(result);
}
#endif // BARDLIB_OS_IMPLEMENTATION
