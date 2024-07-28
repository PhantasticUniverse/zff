#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "environment.h"

#define MAX_ERROR_LENGTH 256

static char last_error_message[MAX_ERROR_LENGTH];
static int has_error = 0;

static void handle_error(const char* message) {
    // Use snprintf to prevent buffer overflow
    snprintf(last_error_message, sizeof(last_error_message), "%s", message);
    has_error = 1;
}

WASM_EXPORT("has_error")
int get_has_error() {
    return has_error;
}

WASM_EXPORT("get_last_error")
const char* get_last_error() {
    return last_error_message;
}

WASM_EXPORT("clear_error")
void clear_error() {
    has_error = 0;
    last_error_message[0] = '\0';
}

Region get_region(int tape_index, enum RegionSize size) {
    Region r = {-1, -1, -1, -1}; // Initialize with invalid values
    
    if (tape_index < 0 || tape_index >= SOUP_SIZE) {
        handle_error("Invalid tape index in get_region");
        return r;
    }
    
    if (size != REGION_4x4 && size != REGION_8x8 && size != REGION_16x16) {
        handle_error("Invalid region size in get_region");
        return r;
    }

    int tape_x = tape_index % SOUP_WIDTH;
    int tape_y = tape_index / SOUP_WIDTH;
    
    r.width = SOUP_WIDTH / size;
    r.height = SOUP_HEIGHT / size;
    r.x = tape_x / r.width;
    r.y = tape_y / r.height;
    
    return r;
}

void get_tapes_in_region(enum RegionSize size, int region_x, int region_y, int* tape_indices, int max_indices, int* count) {
    if (size != REGION_4x4 && size != REGION_8x8 && size != REGION_16x16) {
        handle_error("Invalid region size in get_tapes_in_region");
        *count = 0;
        return;
    }

    int regions_per_side = SOUP_WIDTH / (SOUP_WIDTH / size);
    if (region_x < 0 || region_x >= regions_per_side || region_y < 0 || region_y >= regions_per_side) {
        handle_error("Invalid region coordinates in get_tapes_in_region");
        *count = 0;
        return;
    }

    if (tape_indices == NULL || count == NULL) {
        handle_error("NULL pointer passed to get_tapes_in_region");
        return;
    }

    int tapes_per_region = (SOUP_WIDTH / size) * (SOUP_HEIGHT / size);
    if (max_indices < tapes_per_region) {
        handle_error("Insufficient buffer size in get_tapes_in_region");
        *count = 0;
        return;
    }

    int start_x = region_x * (SOUP_WIDTH / size);
    int start_y = region_y * (SOUP_HEIGHT / size);
    int end_x = start_x + (SOUP_WIDTH / size);
    int end_y = start_y + (SOUP_HEIGHT / size);
    
    *count = 0;
    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            tape_indices[(*count)++] = y * SOUP_WIDTH + x;
        }
    }

    assert(*count == tapes_per_region);
}

void apply_to_region(enum RegionSize size, int region_x, int region_y, void (*func)(int)) {
    if (func == NULL) {
        handle_error("NULL function pointer passed to apply_to_region");
        return;
    }

    int tape_indices[SOUP_WIDTH * SOUP_HEIGHT / (4*4)];  // Maximum possible tapes in a region
    int count;
    get_tapes_in_region(size, region_x, region_y, tape_indices, sizeof(tape_indices)/sizeof(tape_indices[0]), &count);
    
    if (has_error) return;  // Check if get_tapes_in_region encountered an error

    for (int i = 0; i < count; i++) {
        func(tape_indices[i]);
    }
}

WASM_EXPORT("init_region_manager")
RegionManager* init_region_manager(enum RegionSize size) {
    if (size != REGION_4x4 && size != REGION_8x8 && size != REGION_16x16) {
        handle_error("Invalid region size in init_region_manager");
        return NULL;
    }

    RegionManager* manager = (RegionManager*)malloc(sizeof(RegionManager));
    if (manager == NULL) {
        handle_error("Memory allocation failed in init_region_manager");
        return NULL;
    }

    manager->size = size;
    manager->region_count = (SOUP_WIDTH / (SOUP_WIDTH / size)) * (SOUP_HEIGHT / (SOUP_HEIGHT / size));
    manager->properties = (RegionProperties*)malloc(sizeof(RegionProperties) * manager->region_count);
    
    if (manager->properties == NULL) {
        handle_error("Memory allocation failed for properties in init_region_manager");
        free(manager);
        return NULL;
    }

    // Initialize default properties
    for (int i = 0; i < manager->region_count; i++) {
        manager->properties[i].mutation_rate = 1.0f;  // Default mutation rate
    }
    
    return manager;
}

WASM_EXPORT("get_region_properties")
RegionProperties* get_region_properties(RegionManager* manager, int region_x, int region_y) {
    if (manager == NULL) {
        handle_error("NULL RegionManager passed to get_region_properties");
        return NULL;
    }

    int regions_per_side = SOUP_WIDTH / (SOUP_WIDTH / manager->size);
    if (region_x < 0 || region_x >= regions_per_side || region_y < 0 || region_y >= regions_per_side) {
        handle_error("Invalid region coordinates in get_region_properties");
        return NULL;
    }

    int index = region_y * regions_per_side + region_x;
    return &manager->properties[index];
}

WASM_EXPORT("set_region_mutation_rate")
void set_region_mutation_rate(RegionManager* manager, int region_x, int region_y, float rate) {
    if (manager == NULL) {
        handle_error("NULL RegionManager passed to set_region_mutation_rate");
        return;
    }

    if (rate < 0.0f || rate > 1.0f) {
        handle_error("Invalid mutation rate in set_region_mutation_rate");
        return;
    }

    RegionProperties* props = get_region_properties(manager, region_x, region_y);
    if (props) {
        props->mutation_rate = rate;
    }
}

WASM_EXPORT("get_region_mutation_rate")
float get_region_mutation_rate(RegionManager* manager, int region_x, int region_y) {
    RegionProperties* props = get_region_properties(manager, region_x, region_y);
    if (props) {
        return props->mutation_rate;
    }
    return -1.0f; // Invalid value
}

WASM_EXPORT("mutate_with_regions")
void mutate_with_regions(RegionManager* manager, int n) {
    if (manager == NULL) {
        handle_error("NULL RegionManager passed to mutate_with_regions");
        return;
    }

    if (n < 0) {
        handle_error("Invalid number of mutations in mutate_with_regions");
        return;
    }

    for (int i = 0; i < n; ++i) {
        uint64_t rnd = rand64();
        int tape_index = rnd % SOUP_SIZE;
        
        // Get the region for this tape
        Region region = get_region(tape_index, manager->size);
        
        // Get the region's properties
        RegionProperties* props = get_region_properties(manager, region.x, region.y);
        
        if (props == NULL) continue;  // Skip if we couldn't get properties

        // Only mutate if a random number is less than the region's mutation rate
        if ((float)rand64() / (float)UINT64_MAX < props->mutation_rate) {
            uint8_t v = rnd & 0xff;
            soup[tape_index] = v;
        }
    }
}

WASM_EXPORT("free_region_manager")
void free_region_manager(RegionManager* manager) {
    if (manager == NULL) {
        return;  // It's okay to free a NULL pointer, so we just return
    }
    free(manager->properties);
    free(manager);
}