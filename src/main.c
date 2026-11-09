#define BARDLIB_OS_IMPLEMENTATION
#include "bardlib_os.h"

// TODO: abstract win32 away
struct Window_State {
    int window_should_close;
} global_state;


// implement https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
bool idx_is_on_line(int64_t idx, int64_t x1, int64_t y1, int64_t x2, int64_t y2)
{
    if (idx >= (int64_t)(bard_window.height * bard_window.width)) {printf("WARNING: idx_is_on_line: idx too big \n)"); return false;}

    int64_t idx_x = (int64_t)(idx % bard_window.width);
    int64_t idx_y = (int64_t)(idx / bard_window.width);

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
    uint64_t idx1 = y1 * bard_window.width + x1;
    uint64_t idx2 = y2 * bard_window.width + x2;

    if (idx1 == idx2) {bard_window.pixels[idx1] = color; printf("WARNING: idx1 == idx2, coloring single pixel and exiting \n)"); return;}

    uint64_t biggeridx = idx1 > idx2 ? idx1 : idx2;
    uint64_t smalleridx = idx1 < idx2 ? idx1 : idx2;

    if (biggeridx >= (bard_window.height * bard_window.width)) {printf("WARNING: biggeridx bigger than window :( \n)"); return;}

    for (uint64_t idx = smalleridx; idx <= biggeridx; ++idx)
    {
        if (idx_is_on_line(idx, x1, y1, x2, y2)) {
            bard_window.pixels[idx] = color;
        }
    }
}

#define BARD_RGB(r, g, b) (uint32_t)((0x00 << 24) | (((uint8_t)(r)) << 16) | (((uint8_t)(g)) << 8) | ((uint8_t)(b)))

int user_main() {
    // TODO: abstract away this thing:
    bard_create_window(800, 600, "this is a title");

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

        const uint64_t windowsize = bard_window.width * bard_window.height;
        for (uint64_t idx = 0; idx < windowsize; ++idx)
        {
            bard_window.pixels[idx] = BARD_RGB(255, 255, 255);
        }

        draw_line(0, 0, 799, 599, BARD_RGB(255, 0, 0));

        // TODO: need begindrawing/enddrawing delimeter functions
        RedrawWindow(win32_window.window, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    return 0;
}
