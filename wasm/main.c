#include "common.h"
#include <math.h>
#include <string.h>

// enum {
//     SOUP_WIDTH = 200,
//     SOUP_HEIGHT = 200,
//     TAPE_N = SOUP_WIDTH*SOUP_HEIGHT,
//     SOUP_SIZE = TAPE_N*TAPE_LENGTH,
// };

// Define the global array of RegionData
RegionData global_regions[REGION_COUNT];

// Random number generator
uint64_t rand64() {
    uint64_t z = (rng_state[0] += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

// Implementation of get_tape_len
int get_tape_len() { 
    return TAPE_LENGTH; 
}

// Implementation of get_soup_width
int get_soup_width() { 
    return GRID_WIDTH; 
}

// Implementation of get_soup_height
int get_soup_height() { 
    return GRID_HEIGHT; 
}

void initialize_regions() {
    for (int i = 0; i < REGION_COUNT; ++i) {
        global_regions[i].mutation_multiplier = 1.0f;
        for (int j = 0; j < 4; ++j) {
            global_regions[i].directional_influence[j] = 1.0f;
        }
        global_regions[i].impassable = 0;
    }
    // Copy initialized data to exposed buffer
    memcpy(regions_data, global_regions, sizeof(RegionData) * REGION_COUNT);
}

void init(int seed) {
    rng_state[0] = seed;
    initialize_regions();
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT * TAPE_LENGTH; ++i) {
        soup[i] = rand64();
    }
}

void mutate(int n) {
    for (int i=0; i<n; ++i) {
        uint64_t rnd = rand64();
        uint8_t v = rnd&0xff; rnd >>= 8;
        soup[rnd % SOUP_SIZE] = v;
        
        // Region indexes
        int index = rnd % (GRID_WIDTH * GRID_HEIGHT * TAPE_LENGTH);
        int x = (index / TAPE_LENGTH) % GRID_WIDTH;
        int y = (index / TAPE_LENGTH) / GRID_WIDTH;
        int region_x = x / REGION_WIDTH;
        int region_y = y / REGION_HEIGHT;
        int region_index = region_y * 8 + region_x; // Adjusted for 8x8

        if (global_regions[region_index].impassable) continue; 
    }
}
// void mutate(int n) {
//     for (int i = 0; i < n; ++i) {
//         uint64_t rnd = rand64();
//         uint8_t v = rnd & 0xff;
//         rnd >>= 8;
//         int index = rnd % (GRID_WIDTH * GRID_HEIGHT * TAPE_LENGTH);
//         int x = (index / TAPE_LENGTH) % GRID_WIDTH;
//         int y = (index / TAPE_LENGTH) / GRID_WIDTH;
//         int region_x = x / REGION_WIDTH;
//         int region_y = y / REGION_HEIGHT;
//         int region_index = region_y * 4 + region_x;

//         if (global_regions[region_index].impassable) continue;

//         if (rand64() % (int)(10 / global_regions[region_index].mutation_multiplier) == 0) {
//             soup[index] = v;
//         }
//     }
// }

int prepare_batch() {
    int pair_n = 0, collision_count = 0, pos = 0;
    uint8_t mask[TAPE_N] = {0};
    while (pair_n < MAX_BATCH_PAIR_N && collision_count < 16) {
        uint64_t rnd = rand64();
        int dir = (rnd & 1) * 2 - 1; rnd >>= 1;
        int horizontal = rnd & 1; rnd >>= 1;
        int i = rnd % (TAPE_N), j = i;
        
        int x = i % GRID_WIDTH;
        int y = i / GRID_WIDTH;
        int region_x = x / REGION_WIDTH;
        int region_y = y / REGION_HEIGHT;
        int region_index = region_y * 8 + region_x; // adjusted for 8x8

        if (global_regions[region_index].impassable) {
            ++collision_count;
            continue;
        }

        if (mask[i]) {
            ++collision_count;
            continue;
        }

        // // Apply directional influence
        // float* influence = global_regions[region_index].directional_influence;
        // float total_influence = influence[0] + influence[1] + influence[2] + influence[3];
        // float r = (float)rand64() / (float)UINT64_MAX * total_influence;

        // if (r < influence[0] && y > 0) {  // Up
        //     j = i - GRID_WIDTH;
        // } else if (r < influence[0] + influence[1] && y < GRID_HEIGHT - 1) {  // Down
        //     j = i + GRID_WIDTH;
        // } else if (r < influence[0] + influence[1] + influence[2] && x > 0) {  // Left
        //     j = i - 1;
        // } else if (x < GRID_WIDTH - 1) {  // Right
        //     j = i + 1;
        // }

        // Freshly added, remove if it breaks stuff
        if (horizontal) {
            if (i % SOUP_WIDTH == 0)     { dir =  1; }
            if ((i+1) % SOUP_WIDTH == 0) { dir = -1; }
            j += dir;
        } else {
            if (i < SOUP_WIDTH)          { dir =  1; }
            if (TAPE_N-i-1 < SOUP_WIDTH) { dir = -1; }
            j += dir*SOUP_WIDTH;
        }

        if (mask[j]) { ++collision_count; continue;}
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

int absorb_batch() {
    const uint8_t *src = batch;
    const int pair_n = batch_pair_n[0];
    for (int i = 0; i < pair_n * 2; ++i) {
        const int tape_idx = batch_idx[i];
        uint8_t *dst = soup + tape_idx * TAPE_LENGTH;
        for (int k = 0; k < TAPE_LENGTH; ++k, ++src, ++dst) {
            *dst = *src;
        }
        write_count[tape_idx] = batch_write_count[i];
    }
    return pair_n;
}

void updateCounts() {
    for (int i = 0; i < 256; ++i) counts[i] = 0;
    for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT * TAPE_LENGTH; ++i) {
        counts[soup[i]]++;
    }
}

void set_region_mutation_multiplier(int region_x, int region_y, float multiplier) {
    if (region_x >= 0 && region_x < 8 && region_y >= 0 && region_y < 8) {
        int index = region_y * 8 + region_x; // adjusted for 8x8
        global_regions[index].mutation_multiplier = multiplier;
        regions_data[index].mutation_multiplier = multiplier;
    }
}

void set_region_directional_influence(int region_x, int region_y, float up, float down, float left, float right) {
    if (region_x >= 0 && region_x < 8 && region_y >= 0 && region_y < 8) {
        int index = region_y * 8 + region_x; // adjusted for 8x8
        global_regions[index].directional_influence[0] = up;
        global_regions[index].directional_influence[1] = down;
        global_regions[index].directional_influence[2] = left;
        global_regions[index].directional_influence[3] = right;
        memcpy(regions_data[index].directional_influence, global_regions[index].directional_influence, sizeof(float) * 4);
    }
}

void set_region_impassable(int region_x, int region_y, uint8_t impassable) {
    if (region_x >= 0 && region_x < 8 && region_y >= 0 && region_y < 8) {
        int index = region_y * 8 + region_x; // adjusted for 8x8
        global_regions[index].impassable = impassable;
        regions_data[index].impassable = impassable;
    }
}