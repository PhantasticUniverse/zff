#ifndef REGION_GRID_H
#define REGION_GRID_H

#include "region.h"

void init_region_grid(int size);
int get_region_grid_size();
Region* get_region(int x, int y);
int get_region_for_cell(int x, int y);

#endif // REGION_GRID_H