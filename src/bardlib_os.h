#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>

int user_main();

#ifdef _WIN32

int _fltused = 0;

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

typedef void* (__cdecl *RtlCopyMemory_)(void*, const void*, size_t);
typedef void* (__cdecl *RtlMoveMemory_)(void*, const void*, size_t);
typedef void* (__cdecl *RtlFillMemory_)(void*, size_t, int);
typedef size_t (__cdecl *RtlCompareMemory_)(const void*, const void*, size_t);

typedef int (__cdecl *RtlQueryPerformanceFrequency_)(uint64_t*);
typedef int (__cdecl *RtlQueryPerformanceCounter_)(uint64_t*);

union {
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
} win32_membuiltin_functions = {0};

union {
    struct {
        RtlQueryPerformanceFrequency_ qpc_freq;
        RtlQueryPerformanceCounter_ qpc_count;
    };
    struct {
        FARPROC fqpc_freq;
        FARPROC fqpc_count;
    };
} win32_time_functions = {0};

struct {
    char buffer[1024];
    HANDLE stdout_handle;
    DWORD scratch_int;
} win32_stdout = {0};

struct {
    uint64_t freq;
    uint64_t previous_count;
    uint64_t current_count;
} win32_time_state = {0};

// TODO: name these to win32_memcpy etc. and then point memcpy to these if WIN32_ is defined
inline void* memcpy(void* dst, const void* src, size_t n) {return win32_membuiltin_functions.copy(dst, src, n);}
inline void* memmove(void* dst, const void* src, size_t n) {return win32_membuiltin_functions.move(dst, src, n);}
inline void* memset(void* dst, int c, size_t n) {return win32_membuiltin_functions.set(dst, n, c);}
inline int memcmp(const void* p1, const void* p2, size_t n) {
    size_t same = win32_membuiltin_functions.cmp(p1, p2, n);
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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#pragma clang diagnostic ignored "-Wextra"
#pragma clang diagnostic ignored "-Wpedantic"
#define STB_SPRINTF_IMPLEMENTATION
#include "../thirdparty/stb_sprintf.h"
#pragma clang diagnostic pop

// TODO: compiler may expect strlen too, on top of the mem* functions? https://nullprogram.com/blog/2024/02/05/
static size_t bard_strlen(const char *s) {
	const char *a = s;
	for (; *s; s++);
	return s-a;
}

void win32_puts(char* msg) {
    WriteFile(win32_stdout.stdout_handle, msg, (DWORD)bard_strlen(msg), &win32_stdout.scratch_int, NULL);
}

__attribute__((format(printf, 1, 2))) // 1-indexed position of fmt string, and the variadic params
void win32_printf(char* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    stbsp_vsprintf(win32_stdout.buffer, fmt, args);
    win32_puts(win32_stdout.buffer);
}

static void win32_init() {
    HMODULE ntdll_handle = GetModuleHandleW(L"ntdll.dll");
    // builtins
    win32_membuiltin_functions.fcopy = GetProcAddress(ntdll_handle, "RtlCopyMemory");
    win32_membuiltin_functions.fmove = GetProcAddress(ntdll_handle, "RtlMoveMemory");
    win32_membuiltin_functions.fset = GetProcAddress(ntdll_handle, "RtlFillMemory");
    win32_membuiltin_functions.fcmp = GetProcAddress(ntdll_handle, "RtlCompareMemory");
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

__attribute((force_align_arg_pointer, used, noreturn))
void entry_point(void)
{
    win32_init();

    int result = user_main();

    ExitProcess(result);
}

#endif // _WIN32
