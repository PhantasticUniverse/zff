#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "environment.h"
#include "common.h"

// Mock rand64 function for testing
uint64_t rand64() {
    return 0x0123456789ABCDEF;
}

// Mock soup array
uint8_t soup[SOUP_SIZE];

void test_get_region() {
    Region r = get_region(0, REGION_4x4);
    assert(r.x == 0 && r.y == 0);
    assert(r.width == SOUP_WIDTH / 4 && r.height == SOUP_HEIGHT / 4);

    r = get_region(SOUP_SIZE - 1, REGION_4x4);
    assert(r.x == 3 && r.y == 3);

    r = get_region(SOUP_WIDTH * 100 + 100, REGION_8x8);
    assert(r.x == 4 && r.y == 4);
    assert(r.width == SOUP_WIDTH / 8 && r.height == SOUP_HEIGHT / 8);
}

void test_get_tapes_in_region() {
    int tape_indices[10000];
    int count;

    get_tapes_in_region(REGION_4x4, 0, 0, tape_indices, &count);
    assert(count == (SOUP_WIDTH / 4) * (SOUP_HEIGHT / 4));
    assert(tape_indices[0] == 0);
    assert(tape_indices[count - 1] == (SOUP_WIDTH / 4) * (SOUP_HEIGHT / 4) - 1);

    get_tapes_in_region(REGION_4x4, 3, 3, tape_indices, &count);
    assert(count == (SOUP_WIDTH / 4) * (SOUP_HEIGHT / 4));
    assert(tape_indices[0] == SOUP_WIDTH * (SOUP_HEIGHT - SOUP_HEIGHT / 4) + (SOUP_WIDTH - SOUP_WIDTH / 4));
    assert(tape_indices[count - 1] == SOUP_SIZE - 1);
}

void test_apply_to_region() {
    int call_count = 0;
    void test_func(int tape_index); {
        call_count++;
    }

    apply_to_region(REGION_4x4, 0, 0, test_func);
    assert(call_count == (SOUP_WIDTH / 4) * (SOUP_HEIGHT / 4));
}

void test_region_manager() {
    RegionManager* manager = init_region_manager(REGION_4x4);
    assert(manager != NULL);
    assert(manager->size == REGION_4x4);
    assert(manager->region_count == 16);

    RegionProperties* props = get_region_properties(manager, 0, 0);
    assert(props != NULL);
    assert(props->mutation_rate == 1.0f);

    set_region_mutation_rate(manager, 0, 0, 0.5f);
    props = get_region_properties(manager, 0, 0);
    assert(props->mutation_rate == 0.5f);

    free_region_manager(manager);
}

void test_mutate_with_regions() {
    RegionManager* manager = init_region_manager(REGION_4x4);
    set_region_mutation_rate(manager, 0, 0, 0.0f);  // No mutations in this region
    set_region_mutation_rate(manager, 1, 1, 1.0f);  // Always mutate in this region

    for (int i = 0; i < SOUP_SIZE; i++) {
        soup[i] = 0;
    }

    mutate_with_regions(manager, 1000);

    // Check that no mutations occurred in region (0,0)
    for (int i = 0; i < SOUP_WIDTH / 4; i++) {
        for (int j = 0; j < SOUP_HEIGHT / 4; j++) {
            assert(soup[j * SOUP_WIDTH + i] == 0);
        }
    }

    // Check that mutations occurred in region (1,1)
    int mutations = 0;
    for (int i = SOUP_WIDTH / 4; i < SOUP_WIDTH / 2; i++) {
        for (int j = SOUP_HEIGHT / 4; j < SOUP_HEIGHT / 2; j++) {
            if (soup[j * SOUP_WIDTH + i] != 0) {
                mutations++;
            }
        }
    }
    assert(mutations > 0);

    free_region_manager(manager);
}

int main() {
    test_get_region();
    test_get_tapes_in_region();
    test_apply_to_region();
    test_region_manager();
    test_mutate_with_regions();

    printf("All tests passed!\n");
    return 0;
}