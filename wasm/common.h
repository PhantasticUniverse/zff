#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#ifdef WASM
    #define WASM_EXPORT(name) __attribute__((export_name(name)))
#else
    #define WASM_EXPORT(name)
#endif

#define BUFFER(name, type, size) type name[size]; \
  WASM_EXPORT("_get_"#name) type* get_##name() {return name;} \
  WASM_EXPORT("_len_"#name"__"#type) int get_##name##_len() {return (size);}

#define GRID_WIDTH 200
#define GRID_HEIGHT 200
#define REGION_WIDTH (GRID_WIDTH / 8)
#define REGION_HEIGHT (GRID_HEIGHT / 8)
#define REGION_COUNT 64  // 8x8 grid of regions
// #define REGION_WIDTH (GRID_WIDTH / 4)
// #define REGION_HEIGHT (GRID_HEIGHT / 4)
// #define REGION_COUNT 16  // 4x4 grid of regions

enum {
    TAPE_LENGTH = 16, // must be 2 ** N
    MAX_BATCH_PAIR_N = 1024 * 8,
    SOUP_WIDTH = 200,
    SOUP_HEIGHT = 200,
    TAPE_N = SOUP_WIDTH*SOUP_HEIGHT,
    SOUP_SIZE = TAPE_N*TAPE_LENGTH,
};

// Define a struct for region data that matches our desired memory layout
typedef struct {
    float mutation_multiplier;
    float directional_influence[4];  // [up, down, left, right]
    uint8_t impassable;
    uint8_t padding[3];  // Ensure 4-byte alignment
} RegionData;

// Declare the global array of RegionData
extern RegionData global_regions[REGION_COUNT];

// Declare the buffer for exposing region data to JavaScript
BUFFER(regions_data, RegionData, REGION_COUNT)

// Declare other buffers only if we're in the main module
#ifndef Z80WORKER
BUFFER(soup, uint8_t, GRID_WIDTH * GRID_HEIGHT * TAPE_LENGTH)
BUFFER(counts, int, 256)
BUFFER(write_count, int, GRID_WIDTH * GRID_HEIGHT)
BUFFER(batch_pair_n, int, 1)
BUFFER(batch_idx, int, MAX_BATCH_PAIR_N * 2)
BUFFER(batch, uint8_t, MAX_BATCH_PAIR_N * 2 * TAPE_LENGTH)
BUFFER(batch_write_count, int, MAX_BATCH_PAIR_N * 2)
BUFFER(rng_state, uint64_t, 1)
#endif

// Function declarations
WASM_EXPORT("get_tape_len") int get_tape_len();
WASM_EXPORT("get_soup_width") int get_soup_width();
WASM_EXPORT("get_soup_height") int get_soup_height();
WASM_EXPORT("init") void init(int seed);
WASM_EXPORT("mutate") void mutate(int n);
WASM_EXPORT("prepare_batch") int prepare_batch();
WASM_EXPORT("absorb_batch") int absorb_batch();
WASM_EXPORT("updateCounts") void updateCounts();
WASM_EXPORT("set_region_mutation_multiplier") void set_region_mutation_multiplier(int region_x, int region_y, float multiplier);
WASM_EXPORT("set_region_directional_influence") void set_region_directional_influence(int region_x, int region_y, float up, float down, float left, float right);
WASM_EXPORT("set_region_impassable") void set_region_impassable(int region_x, int region_y, uint8_t impassable);

#endif // COMMON_H