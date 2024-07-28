#include <stddef.h>
#include <assert.h>  // Added for static assertions
#include "common.h"
#include "math.h"
#include "environment.h"

// Constants to replace magic numbers
#define RNG_CONSTANT_A 0x9e3779b97f4a7c15
#define RNG_CONSTANT_B 0xbf58476d1ce4e5b9
#define RNG_CONSTANT_C 0x94d049bb133111eb
#define BYTE_MASK 0xFF
#define BITS_IN_BYTE 8
#define MAX_COLLISION_ATTEMPTS 16
#define DIRECTION_BITS 1
#define ORIENTATION_BITS 1

// Static assertions to ensure compile-time checks
static_assert(TAPE_LENGTH > 0, "TAPE_LENGTH must be positive");
static_assert((TAPE_LENGTH & (TAPE_LENGTH - 1)) == 0, "TAPE_LENGTH must be a power of 2");
static_assert(SOUP_SIZE > 0, "SOUP_SIZE must be positive");

BUFFER(counts, int, 256)
BUFFER(write_count, int, TAPE_N)
BUFFER(batch_pair_n, int, 1)
BUFFER(batch_idx, int, MAX_BATCH_PAIR_N*2)
BUFFER(batch, uint8_t, MAX_BATCH_PAIR_N*2*TAPE_LENGTH)
BUFFER(batch_write_count, int, MAX_BATCH_PAIR_N*2)
BUFFER(rng_state, uint64_t, 1)

// Global RegionManager pointer
static RegionManager* g_region_manager = NULL;

WASM_EXPORT("get_tape_len")
int get_tape_len() {
    return TAPE_LENGTH;
}

WASM_EXPORT("get_soup_width")
int get_soup_width() {
    return SOUP_WIDTH;
}

WASM_EXPORT("get_soup_height")
int get_soup_height() {
    return SOUP_HEIGHT;
}

// Kept unchanged as per instruction
uint64_t rand64() {
    uint64_t z = (rng_state[0] += RNG_CONSTANT_A);
    z = (z ^ (z >> 30)) * RNG_CONSTANT_B;
    z = (z ^ (z >> 27)) * RNG_CONSTANT_C;
    return z ^ (z >> 31);
}

WASM_EXPORT("init")
void init(int seed) {
    rng_state[0] = seed;
    for (int i = 0; i < SOUP_SIZE; ++i) {
        soup[i] = rand64() & BYTE_MASK;
    }
}

WASM_EXPORT("mutate")
void mutate(int n) {
    if (n < 0) {
        // Added error handling for negative mutation count
        handle_error("Negative mutation count in mutate function");
        return;
    }
    
    if (g_region_manager) {
        mutate_with_regions(g_region_manager, n);
    } else {
        for (int i = 0; i < n; ++i) {
            uint64_t rnd = rand64();
            uint8_t v = rnd & BYTE_MASK;
            rnd >>= BITS_IN_BYTE;
            // Use bitwise AND with (SOUP_SIZE - 1) instead of modulo
            // This is safe because SOUP_SIZE is guaranteed to be a power of 2
            soup[rnd & (SOUP_SIZE - 1)] = v;
        }
    }
}

WASM_EXPORT("prepare_batch")
int prepare_batch() {
    int pair_n = 0, collision_count = 0, pos = 0;
    uint8_t mask[TAPE_N] = {0};
    while (pair_n < MAX_BATCH_PAIR_N && collision_count < MAX_COLLISION_ATTEMPTS) {
        uint64_t rnd = rand64();
        int dir = ((rnd & 1) << 1) - 1;  // Convert 0/1 to -1/1
        rnd >>= DIRECTION_BITS;
        int horizontal = rnd & 1;
        rnd >>= ORIENTATION_BITS;
        int i = rnd % TAPE_N, j = i;
        if (mask[i]) {
            ++collision_count;
            continue;
        }
        if (horizontal) {
            if (i % SOUP_WIDTH == 0) {
                dir = 1;
            }
            if ((i + 1) % SOUP_WIDTH == 0) {
                dir = -1;
            }
            j += dir;
        } else {
            if (i < SOUP_WIDTH) {
                dir = 1;
            }
            if (TAPE_N - i - 1 < SOUP_WIDTH) {
                dir = -1;
            }
            j += dir * SOUP_WIDTH;
        }
        if (j < 0 || j >= TAPE_N || mask[j]) {
            // Added bounds check for j
            ++collision_count;
            continue;
        }
        mask[i] = mask[j] = 1;
        batch_idx[2 * pair_n] = i;
        batch_idx[2 * pair_n + 1] = j;
        for (int k = 0; k < TAPE_LENGTH; ++k) {
            batch[pos + k] = soup[i * TAPE_LENGTH + k];
            batch[pos + k + TAPE_LENGTH] = soup[j * TAPE_LENGTH + k];
        }
        pos += TAPE_LENGTH * 2;
        pair_n++;
        collision_count = 0;
    }
    batch_pair_n[0] = pair_n;
    return pair_n;
}

WASM_EXPORT("absorb_batch")
void absorb_batch() {
    const uint8_t* src = batch;
    const int pair_n = batch_pair_n[0];
    for (int i = 0; i < pair_n * 2; ++i) {
        const int tape_idx = batch_idx[i];
        if (tape_idx < 0 || tape_idx >= TAPE_N) {
            // Added bounds check for tape_idx
            handle_error("Invalid tape index in absorb_batch");
            return;
        }
        uint8_t* dst = soup + tape_idx * TAPE_LENGTH;
        for (int k = 0; k < TAPE_LENGTH; ++k, ++src, ++dst) {
            *dst = *src;
        }
        write_count[tape_idx] = batch_write_count[i];
    }
}

WASM_EXPORT("updateCounts")
void updateCounts() {
    for (int i = 0; i < 256; ++i) counts[i] = 0;
    for (int i = 0; i < SOUP_SIZE; ++i) {
        counts[soup[i]]++;
    }
}

// Initialize the environment
WASM_EXPORT("init_environment")
int init_environment(enum RegionSize size) {
    if (g_region_manager) {
        free_region_manager(g_region_manager);
    }
    g_region_manager = init_region_manager(size);
    return g_region_manager != NULL;
}

// Clean up the environment
WASM_EXPORT("cleanup_environment")
void cleanup_environment() {
    if (g_region_manager) {
        free_region_manager(g_region_manager);
        g_region_manager = NULL;
    }
}