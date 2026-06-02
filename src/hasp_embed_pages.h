/* Lookup for jsonl page files embedded in the firmware via board_build.embed_files.
   Defined when HASP_USE_EMBED_PAGES is set; otherwise returns nullptr. */
#ifndef HASP_EMBED_PAGES_H
#define HASP_EMBED_PAGES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

const uint8_t* hasp_embed_find(const char* name, size_t* len);

#ifdef __cplusplus
}
#endif

#endif
