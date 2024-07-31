import { prepareWASM } from "./util.js";

const GRID_WIDTH = 200;
const GRID_HEIGHT = 200;
const REGION_WIDTH = GRID_WIDTH / 8; // Adjusted for 8x8
const REGION_HEIGHT = GRID_HEIGHT / 8;

async function initWASM() {
    const wasm = await WebAssembly.instantiateStreaming(fetch('../wasm/z80worker.wasm'));
    return prepareWASM(wasm.instance);
}
let modulePromise = initWASM();

self.onmessage = async e => {
    const wasm = await modulePromise;
    const msg = e.data;
    wasm.batch.set(msg.batch);
    const totalOps = wasm.run(msg.pair_n, 128);

    const pair_n = msg.pair_n;
    const batchLength = msg.batch.length;

    // Use the copied regions data
    const regions_data = new Float32Array(msg.regions_data);

    // Get tape_len using the exported function
    const tape_len = wasm.get_tape_len();

    // Process each pair considering the environmental layer
    for (let i = 0; i < pair_n * 2; ++i) {
        const index = i * tape_len;
        const x = (msg.ofs * 2 + i) % GRID_WIDTH;
        const y = Math.floor((msg.ofs * 2 + i) / GRID_WIDTH);
        const regionX = Math.floor(x / REGION_WIDTH);
        const regionY = Math.floor(y / REGION_HEIGHT);

        const regionIndex = regionY * 8 + regionX; // adjusted for 8x8
        const regionStart = regionIndex * 8; // was already 8, may need to change
        const mutationMultiplier = regions_data[regionStart];
        const directionalInfluence = regions_data.slice(regionStart + 1, regionStart + 5);
        const impassable = regions_data[regionStart + 5] !== 0;

        // Skip cells in impassable regions
        if (impassable) continue;

        // // Apply mutation based on mutation multiplier
        // if (Math.random() < mutationMultiplier / 10) {
        //     for (let j = 0; j < tape_len; j++) {
        //         if (Math.random() < 0.1) {  // 10% chance to mutate each byte
        //             wasm.batch[index + j] = Math.floor(Math.random() * 256);
        //         }
        //     }
        // }

        // // Apply directional influence
        // const totalInfluence = directionalInfluence.reduce((a, b) => a + b, 0);
        // if (totalInfluence > 0) {
        //     const normalizedInfluence = directionalInfluence.map(inf => inf / totalInfluence);
        //     let r = Math.random();
        //     let targetIndex = -1;
        //     if (r < normalizedInfluence[0] && y > 0) { // Up
        //         targetIndex = ((y - 1) * GRID_WIDTH + x) * tape_len;
        //     } else if (r < normalizedInfluence[0] + normalizedInfluence[1] && y < GRID_HEIGHT - 1) { // Down
        //         targetIndex = ((y + 1) * GRID_WIDTH + x) * tape_len;
        //     } else if (r < normalizedInfluence[0] + normalizedInfluence[1] + normalizedInfluence[2] && x > 0) { // Left
        //         targetIndex = (y * GRID_WIDTH + (x - 1)) * tape_len;
        //     } else if (x < GRID_WIDTH - 1) { // Right
        //         targetIndex = (y * GRID_WIDTH + (x + 1)) * tape_len;
        //     }

        //     if (targetIndex !== -1 && targetIndex < batchLength) {
        //         // Only copy if the target is within our batch
        //         for (let j = 0; j < tape_len; j++) {
        //             wasm.batch[targetIndex + j] = wasm.batch[index + j];
        //         }
        //     }
        // }
    }

    self.postMessage({
        batch: wasm.batch.slice(0, batchLength),
        write_count: wasm.write_count.slice(0, pair_n * 2),
        pair_n, ofs: msg.ofs, totalOps
    });
};