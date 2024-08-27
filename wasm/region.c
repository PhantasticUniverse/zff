#include "region.h"
#include <string.h>

void init_region(Region* region) {
    memset(region, 0, sizeof(Region));
    region->temperature = 1.0f;  // Default temperature (neutral)
    region->energy_level = 1.0f; // Default energy level (neutral)
    region->randomness_factor = 0.0f; // No randomness by default
    for (int i = 0; i < NUM_DIRECTIONS; ++i) {
        region->directional_influence[i] = 0.0f; // No directional influence by default
    }
}

void set_region_obstacle(Region* region, bool is_obstacle) {
    region->is_obstacle = is_obstacle;
}

void set_region_directional_influence(Region* region, Direction dir, float value) {
    if (dir >= 0 && dir < NUM_DIRECTIONS) {
        region->directional_influence[dir] = value;
    }
}

void set_region_randomness(Region* region, float value) {
    region->randomness_factor = value;
}

void set_region_temperature(Region* region, float value) {
    region->temperature = value;
}

void set_region_energy(Region* region, float value) {
    region->energy_level = value;
}

// Implement getter functions if needed
bool get_region_obstacle(const Region* region) {
    return region->is_obstacle;
}

float get_region_directional_influence(const Region* region, Direction dir) {
    if (dir >= 0 && dir < NUM_DIRECTIONS) {
        return region->directional_influence[dir];
    }
    return 0.0f;
}

float get_region_randomness(const Region* region) {
    return region->randomness_factor;
}

float get_region_temperature(const Region* region) {
    return region->temperature;
}

float get_region_energy(const Region* region) {
    return region->energy_level;
}