#pragma once

#include <stdint.h>

#ifdef WASM
    #define WASM_EXPORT(name) __attribute__((export_name(name)))
#else
    #define WASM_EXPORT(name)
#endif

#define BUFFER(name, type, size) extern type name[size]; \
  WASM_EXPORT("_get_"#name) type* get_##name(); \
  WASM_EXPORT("_len_"#name"__"#type) int get_##name##_len();

enum {
    TAPE_LENGTH = 16, // must be 2 ** N
    MAX_BATCH_PAIR_N = 1024*8,
    SOUP_WIDTH = 200,
    SOUP_HEIGHT = 200
};

#define TAPE_N (SOUP_WIDTH * SOUP_HEIGHT)
#define SOUP_SIZE (TAPE_N * TAPE_LENGTH)

BUFFER(soup, uint8_t, SOUP_SIZE)

// Declaration for rand64 function
uint64_t rand64(void);

// If you need to access the soup directly in WebAssembly
WASM_EXPORT("get_soup")
uint8_t* get_soup(void);

// If you need to get the soup size in WebAssembly
WASM_EXPORT("get_soup_size")
int get_soup_size(void);