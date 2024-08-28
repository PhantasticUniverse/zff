# zff_enviro API

This document outlines the API for the zff_enviro simulation.

## WebAssembly Exported Functions

### Initialization and Setup

#### `init(seed: int) -> void`
Initializes the simulation with a given random seed.
- `seed`: Integer value used to seed the random number generator.

#### `init_exported_region_grid(size: int) -> void`
Initializes the region grid with the specified size.
- `size`: Integer value representing the grid size (4, 8, or 16).

### Simulation Control

#### `mutate(n: int) -> void`
Introduces random mutations into the simulation.
- `n`: Number of mutations to perform.

#### `prepare_batch() -> int`
Prepares a batch of cell pairs for interaction. Returns the number of pairs prepared.

#### `absorb_batch() -> int`
Processes the prepared batch of cell pairs. Returns the number of pairs processed.

#### `updateCounts() -> void`
Updates the count of each cell type in the simulation.

### Region Grid Operations

#### `get_exported_region_grid_size() -> int`
Returns the current size of the region grid.

#### `get_exported_region(x: int, y: int) -> Region*`
Retrieves the region at the specified coordinates.
- `x`, `y`: Coordinates of the region in the grid.

#### `set_exported_region(x: int, y: int, region: Region*) -> void`
Sets the properties of a region at the specified coordinates.
- `x`, `y`: Coordinates of the region in the grid.
- `region`: Pointer to a Region structure with the new properties.

#### `set_exported_region_obstacle(x: int, y: int, is_obstacle: bool) -> void`
Sets the obstacle property of a region.
- `x`, `y`: Coordinates of the region in the grid.
- `is_obstacle`: Boolean indicating if the region is an obstacle.

#### `set_exported_region_temperature(x: int, y: int, value: float) -> void`
Sets the temperature of a region.
- `x`, `y`: Coordinates of the region in the grid.
- `value`: Float value representing the temperature.

#### `set_exported_region_energy(x: int, y: int, value: float) -> void`
Sets the energy level of a region.
- `x`, `y`: Coordinates of the region in the grid.
- `value`: Float value representing the energy level.

#### `set_exported_region_directional_influence(x: int, y: int, direction: int, value: float) -> void`
Sets the directional influence of a region.
- `x`, `y`: Coordinates of the region in the grid.
- `direction`: Integer representing the direction (0-3 for N, E, S, W).
- `value`: Float value representing the influence strength.

#### `set_exported_region_randomness(x: int, y: int, value: float) -> void`
Sets the randomness factor of a region.
- `x`, `y`: Coordinates of the region in the grid.
- `value`: Float value representing the randomness factor.

### Region Property Getters

#### `get_exported_region_obstacle(x: int, y: int) -> bool`
#### `get_exported_region_temperature(x: int, y: int) -> float`
#### `get_exported_region_energy(x: int, y: int) -> float`
#### `get_exported_region_directional_influence(x: int, y: int, direction: int) -> float`
#### `get_exported_region_randomness(x: int, y: int) -> float`

These functions retrieve the respective properties of a region at the specified coordinates.

### Utility Functions

#### `get_tape_len() -> int`
Returns the length of a cell's tape.

#### `get_soup_width() -> int`
Returns the width of the simulation grid.

#### `get_soup_height() -> int`
Returns the height of the simulation grid.

#### `get_region_for_cell(x: int, y: int) -> int`
Returns the region index for a given cell coordinate.
- `x`, `y`: Coordinates of the cell in the simulation grid.

### Constants

#### `MIN_REGION_GRID_SIZE`
Minimum allowed size for the region grid (4).

#### `MAX_REGION_GRID_SIZE`
Maximum allowed size for the region grid (16).

#### `SOUP_WIDTH`
Width of the simulation grid (200).

#### `SOUP_HEIGHT`
Height of the simulation grid (200).

### Data Structures

#### `Region`
Represents a region in the simulation grid. Contains properties such as:
- `is_obstacle`: Boolean indicating if the region is an obstacle.
- `temperature`: Float value representing the temperature of the region.
- `energy_level`: Float value representing the energy level of the region.
- `randomness_factor`: Float value representing the randomness factor of the region.
- `directional_influence`: Array of 4 float values representing influence in N, E, S, W directions.

Note: The exact structure of `Region` may vary based on implementation details.