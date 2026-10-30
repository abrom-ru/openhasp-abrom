/* Lookup for binary LVGL fonts embedded in the firmware via board_build.embed_files.
   Defined when HASP_USE_EMBED_FONTS is set; otherwise the count is 0. */
#ifndef HASP_EMBED_FONTS_H
#define HASP_EMBED_FONTS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t hasp_embed_font_count(void);
const uint8_t* hasp_embed_font_get(size_t i, const char** name, size_t* len);

#ifdef __cplusplus
}
#endif

#endif
