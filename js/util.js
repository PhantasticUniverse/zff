export function prepareWASM(instance) {
    const type2class = {
        uint8_t: Uint8Array, 
        int: Int32Array, 
        uint64_t: BigUint64Array, 
        float: Float32Array,
        RegionData: Float32Array  // New type for our region data
    };
    const objects = {};
    const prefix = '_len_';
    const exports = instance.exports;

    for (const key in exports) {
        if (!key.startsWith(prefix)) {
            if (!key.startsWith('_')) {
                objects[key] = exports[key];
            }
            continue;
        }
        const [name, type] = key.slice(prefix.length).split('__');
        const ofs = exports['_get_'+name]();
        const len = exports[key]();
        const buf = new (type2class[type])(exports.memory.buffer, ofs, len * (type === 'RegionData' ? 8 : 1));
        objects[name] = buf;
    }

    // Add region-related functions
    objects.setRegionMutationMultiplier = (x, y, value) => {
        exports.set_region_mutation_multiplier(x, y, value);
    };

    objects.setRegionDirectionalInfluence = (x, y, up, down, left, right) => {
        exports.set_region_directional_influence(x, y, up, down, left, right);
    };

    objects.setRegionImpassable = (x, y, value) => {
        exports.set_region_impassable(x, y, value ? 1 : 0);
    };

    // The regions_data is now directly accessible as a Float32Array
    objects.regions_data = objects.regions_data;

    // Helper function to get region data
    objects.getRegionData = (x, y) => {
        if (x >= 0 && x < 4 && y >= 0 && y < 4) {
            const index = y * 4 + x;
            const start = index * 8;
            return {
                mutation_multiplier: objects.regions_data[start],
                directional_influence: [
                    objects.regions_data[start + 1],
                    objects.regions_data[start + 2],
                    objects.regions_data[start + 3],
                    objects.regions_data[start + 4]
                ],
                impassable: objects.regions_data[start + 5] !== 0
            };
        }
        return null;
    };

    return objects;
}