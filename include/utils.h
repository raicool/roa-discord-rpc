#pragma once

#define INCR_PTR(x, idx) ((char*)x + idx)

// memory pattern search macros
#define DYNAMIC_OFFSET_8 '?'
#define DYNAMIC_OFFSET_16 '?', '?'
#define DYNAMIC_OFFSET_32 '?', '?', '?', '?'
#define DYNAMIC_PTR_FIELD DYNAMIC_OFFSET_32
