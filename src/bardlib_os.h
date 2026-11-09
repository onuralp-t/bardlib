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

// declare/initialize globals in implementation
RtlCopyMemory_ win32_memcpy;
RtlMoveMemory_ win32_memmove;
RtlFillMemory_ win32_memset;
RtlCompareMemory_ win32_memcmp;

RtlQueryPerformanceFrequency_ win32_hpt_frequency;
RtlQueryPerformanceCounter_ win32_hpt_counter;

HANDLE win32_stdout;

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
#endif // _WIN32

__attribute((force_align_arg_pointer, used, noreturn))
void entry_point(void)
{
    bard_os_init();

    int result = user_main();

    ExitProcess(result);
}
#endif // BARDLIB_OS_IMPLEMENTATION
