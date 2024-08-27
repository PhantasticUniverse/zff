#include "region_grid.h"
#include <stdlib.h>
#include <string.h>

#define MAX_GRID_SIZE 16

static Region grid[MAX_GRID_SIZE][MAX_GRID_SIZE];
static int grid_size = 0;

void init_region_grid(int size) {
    if (size > 0 && size <= MAX_GRID_SIZE) {
        grid_size = size;
        for (int y = 0; y < grid_size; y++) {
            for (int x = 0; x < grid_size; x++) {
                init_region(&grid[y][x]);
            }
        }
    }
}

int get_region_grid_size() {
    return grid_size;
}

Region* get_region(int x, int y) {
    if (x >= 0 && x < grid_size && y >= 0 && y < grid_size) {
        return &grid[y][x];
    }
    return NULL;
}

// Add more functions as needed for manipulating the grid