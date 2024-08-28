#ifndef REGION_H
#define REGION_H

#include <stdbool.h>

// Constants for grid sizes
#define MAX_REGION_GRID_SIZE 16
#define MIN_REGION_GRID_SIZE 4

// Enum for directions
typedef enum {
    NORTH = 0,
    EAST,
    SOUTH,
    WEST,
    NUM_DIRECTIONS
} Direction;

// Region structure
typedef struct {
    bool is_obstacle;
    float directional_influence[NUM_DIRECTIONS];
    float randomness_factor;
    float temperature;
    float energy_level;
    // Add more parameters here as needed
} Region;

// Function prototypes
void init_region(Region* region);
void set_region_obstacle(Region* region, bool is_obstacle);
void set_region_directional_influence(Region* region, Direction dir, float value);
void set_region_randomness(Region* region, float value);
void set_region_temperature(Region* region, float value);
void set_region_energy(Region* region, float value);

// Getter function prototypes (if needed)
bool get_region_obstacle(const Region* region);
float get_region_directional_influence(const Region* region, Direction dir);
float get_region_randomness(const Region* region);
float get_region_temperature(const Region* region);
float get_region_energy(const Region* region);

#endif // REGION_H