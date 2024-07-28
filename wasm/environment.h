#pragma once

#include <stdint.h>

#ifdef WASM
    #define WASM_EXPORT(name) __attribute__((export_name(name)))
#else
    #define WASM_EXPORT(name)
#endif

/**
 * @brief Enum representing different region sizes.
 */
enum RegionSize {
    REGION_4x4 = 4,    /**< Divides the soup into 4x4 regions */
    REGION_8x8 = 8,    /**< Divides the soup into 8x8 regions */
    REGION_16x16 = 16  /**< Divides the soup into 16x16 regions */
};

/**
 * @brief Structure representing a region in the soup.
 */
typedef struct {
    int x;      /**< X-coordinate of the region */
    int y;      /**< Y-coordinate of the region */
    int width;  /**< Width of the region in tapes */
    int height; /**< Height of the region in tapes */
} Region;

/**
 * @brief Structure holding properties for a region.
 */
typedef struct {
    float mutation_rate; /**< Mutation rate for the region */
} RegionProperties;

/**
 * @brief Structure managing all regions in the soup.
 */
typedef struct {
    RegionProperties* properties; /**< Array of properties for each region */
    enum RegionSize size;         /**< Size of regions */
    int region_count;             /**< Total number of regions */
} RegionManager;

WASM_EXPORT("init_region_manager")
RegionManager* init_region_manager(enum RegionSize size);

WASM_EXPORT("get_region_properties")
RegionProperties* get_region_properties(RegionManager* manager, int region_x, int region_y);

WASM_EXPORT("set_region_mutation_rate")
void set_region_mutation_rate(RegionManager* manager, int region_x, int region_y, float rate);

WASM_EXPORT("mutate_with_regions")
void mutate_with_regions(RegionManager* manager, int n);

WASM_EXPORT("free_region_manager")
void free_region_manager(RegionManager* manager);

WASM_EXPORT("has_error")
int get_has_error();

WASM_EXPORT("get_last_error")
const char* get_last_error();

WASM_EXPORT("clear_error")
void clear_error();

// WASM_EXPORT("get_soup")
// uint8_t* get_soup(void);

// WASM_EXPORT("get_soup_size")
// int get_soup_size(void);

Region get_region(int tape_index, enum RegionSize size);
void get_tapes_in_region(enum RegionSize size, int region_x, int region_y, int* tape_indices, int* count);
void apply_to_region(enum RegionSize size, int region_x, int region_y, void (*func)(int));