#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>

// TODO: make this headeronly library
#include "bardlib_os.h"

// TODO: abstract win32 away
struct Window_State {
    union {
        struct { // win32
        HINSTANCE instance;
        WNDCLASS window_class;
        WNDPROC window_proc;
        HWND window;
        BITMAPINFO bitmap_info;
        } win32;
    };
    int window_should_close;
    uint64_t keyboard; // indices 0 - 26 is ASCII english alphabet, set bit means key is being pressed
    uint32_t* pixels;
    uint64_t window_width;
    uint64_t window_height;
} global_state;


// implement https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
bool idx_is_on_line(int64_t idx, int64_t x1, int64_t y1, int64_t x2, int64_t y2)
{
    if (idx >= (int64_t)(global_state.window_height * global_state.window_width)) {win32_printf("WARNING: idx_is_on_line: idx too big \n)"); return false;}

    int64_t idx_x = (int64_t)(idx % global_state.window_width);
    int64_t idx_y = (int64_t)(idx / global_state.window_width);

    bool satisfies_y = y1 <= y2 ? y1 <= idx_y && idx_y <= y2 : y2 <= idx_y && idx_y <= y1;
    bool satisfies_x = x1 <= x2 ? x1 <= idx_x && idx_x <= x2 : x2 <= idx_x && idx_x <= x1;

    if (!satisfies_x || !satisfies_y) return false;

    // naive line formula: https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line#Line_defined_by_two_points
    double tolerance = 0.8; // TODO take thickness/tolerance into account in the satisfies_x/y checks above

    int64_t dx = x1 - x2;
    int64_t dy = y1 - y2;

    if (dy*dy + dx*dx == 0) return idx_x == x1 && idx_y == y1;

    // int64_t distance_from_idx_to_line = ABSOLUTE_VALUE(dy*idx_x - dx*idx_y + x2*y1 - y2*x1) / SQRT(dy*dy + dx*dx);
    // int64_t distance_from_idx_to_line_squared = (dy*idx_x - dx*idx_y + x2*y1 - y2*x1)*(dy*idx_x - dx*idx_y + x2*y1 - y2*x1) / (dy*dy + dx*dx);
    int64_t distance_from_idx_to_line_squared_scaled = (dy*idx_x - dx*idx_y + x2*y1 - y2*x1)*(dy*idx_x - dx*idx_y + x2*y1 - y2*x1);
    int64_t tolerance_squared_scaled = (int64_t)(tolerance * tolerance * (dy*dy + dx*dx));

    // TODO: we could drop the "scaling" since we scale both sides by same amount, but it's not really important
    return distance_from_idx_to_line_squared_scaled < tolerance_squared_scaled;
}

void draw_line(uint64_t x1, uint64_t y1, uint64_t x2, uint64_t y2, uint32_t color)
{
    uint64_t idx1 = y1 * global_state.window_width + x1;
    uint64_t idx2 = y2 * global_state.window_width + x2;

    if (idx1 == idx2) {global_state.pixels[idx1] = color; win32_printf("WARNING: idx1 == idx2, coloring single pixel and exiting \n)"); return;}

    uint64_t biggeridx = idx1 > idx2 ? idx1 : idx2;
    uint64_t smalleridx = idx1 < idx2 ? idx1 : idx2;

    if (biggeridx >= (global_state.window_height * global_state.window_width)) {win32_printf("WARNING: biggeridx bigger than window :( \n)"); return;}

    for (uint64_t idx = smalleridx; idx <= biggeridx; ++idx)
    {
        if (idx_is_on_line(idx, x1, y1, x2, y2)) {
            global_state.pixels[idx] = color;
        }
    }
}

#define BARD_RGB(r, g, b) (uint32_t)((0x00 << 24) | (((uint8_t)(r)) << 16) | (((uint8_t)(g)) << 8) | ((uint8_t)(b)))

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE: { // user attempted to close the window
            // DestroyWindow(hwnd);
            PostQuitMessage(0);
        } break;

        case WM_DESTROY: { // the closed window's resources are getting released
            // thread is quiting (sends WM_QUIT, not handled by windproc, used to exit the inf loop in main)
            PostQuitMessage(0);
        } break;

        case WM_KILLFOCUS: global_state.keyboard = 0; break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            int windowWidth = clientRect.right - clientRect.left;
            int windowHeight = clientRect.bottom - clientRect.top;

            StretchDIBits(
                hdc,
                0, 0, windowWidth, windowHeight,
                0, 0, 800, 600,
                global_state.pixels,
                &global_state.win32.bitmap_info,
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

void bard_create_window_win32(uint64_t window_width, uint64_t window_height, const char* window_title)
{
    HINSTANCE hinstance = GetModuleHandleA(NULL);

    const char* CLASS_NAME = window_title; // this doesn't determine title name

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hinstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursorA(NULL, IDC_ARROW); // first param is the location of the cursor resource

    global_state.win32.window_proc = WindowProc;

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        window_title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hinstance, NULL
    );

    global_state.win32.instance = hinstance;
    global_state.win32.window_class = wc;
    global_state.win32.window = hwnd;
    global_state.win32.window_proc = WindowProc;

    global_state.pixels = (uint32_t*)VirtualAlloc(NULL, window_width * window_height * sizeof(uint32_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    global_state.win32.bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    global_state.win32.bitmap_info.bmiHeader.biWidth = window_width;
    global_state.win32.bitmap_info.bmiHeader.biHeight = -window_height; // neg -> making (0,0) top left
    global_state.win32.bitmap_info.bmiHeader.biPlanes = 1;
    global_state.win32.bitmap_info.bmiHeader.biBitCount = 32;
    global_state.win32.bitmap_info.bmiHeader.biCompression = BI_RGB;

    global_state.window_width = window_width;
    global_state.window_height = window_height;

    ShowWindow(global_state.win32.window, SW_NORMAL);
}

int user_main() {
    // TODO: abstract away this thing:
    bard_create_window_win32(800, 600, "this is a title");

    while (!global_state.window_should_close)
    {
        // TODO: abstract away event handling :scream:
        MSG msg = {0};

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {
                global_state.window_should_close = true;
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg); // calls WindowProc
        }

        const uint64_t windowsize = global_state.window_width * global_state.window_height;
        for (uint64_t idx = 0; idx < windowsize; ++idx)
        {
            global_state.pixels[idx] = BARD_RGB(255, 255, 255);
        }

        draw_line(0, 0, 799, 599, BARD_RGB(255, 0, 0));

        // TODO: need begindrawing/enddrawing delimeter functions
        RedrawWindow(global_state.win32.window, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    return 0;
}
