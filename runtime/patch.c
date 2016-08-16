#include "pebble.h"
#include "patch.auto.h"
#include "text_shaper.h"
#include "rtl.h"
#include "platform.h"

void *memset(void* dest, int val, size_t size) {
    while (size--) {
        *((char*)dest++) = (char)val;
    }
    return NULL; // Whatever.
}

void graphics_draw_text_patch(GContext* ctx, char* text, GFont const font, const GRect box, const GTextOverflowMode overflow_mode, GTextAlignment alignment, GTextAttributes* text_attributes) {
    bool shaped_text = false;
    if (text >= (char*)SRAM_BASE) {
        shaped_text = shape_text(text);
    }
    PASSTHRU(graphics_draw_text_patch, ctx, text, font, box, overflow_mode, alignment, text_attributes);
    if (shaped_text) {
        unshape_text(text);
    }
}

typedef void (*RenderHandler)(void*, void*, void*);
void render_wrap(void* gcontext, char** layout, bool more_text, RenderHandler handler, char* callsite_sp) {
    // First, apply RTL transforms.
    char* line_start, *line_end, *line_end_1, *line_end_2;
    line_start = *layout;
    if (line_start >= (char*)SRAM_BASE) {
        line_end_1 = *(char**)(callsite_sp + LINEEND_SP_OFF);
        line_end_2 = *(char**)(callsite_sp + LINEEND_SP_OFF + 4);
        line_end = more_text ? line_end_1 : line_end_2;
        while (line_end > line_start && *(line_end - 1) == ' ') line_end--;
        while (*line_start == ' ') line_start++;
        rtl_apply(line_start, line_end);
    }

    // Call through to actual render handler.
    void* mystery_argument = *(void**)(callsite_sp + RENDERHDLR_ARG3_SP_OFF);
    handler(gcontext, layout, mystery_argument);

    // If we applied the RTL operations once, do them again to undo the changes.
    if (line_start >= (char*)SRAM_BASE) {
        rtl_apply(line_start, line_end);
    }
}
