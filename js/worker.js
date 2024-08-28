import { prepareWASM } from "./util.js";

async function initWASM() {
    const wasm = await WebAssembly.instantiateStreaming(fetch('../wasm/z80worker.wasm'));
    return prepareWASM(wasm.instance);
}
let module = initWASM();

self.onmessage = async e => {
    const wasm = await module;
    const msg = e.data;
    wasm.batch.set(msg.batch);
    
    // Use global effects if enabled and the functions are available
    if (msg.useGlobalEffects) {
        if (typeof wasm.set_global_temperature === 'function') {
            wasm.set_global_temperature(msg.globalTemperature);
        }
        if (typeof wasm.set_global_energy === 'function') {
            wasm.set_global_energy(msg.globalEnergy);
        }
        if (typeof wasm.set_global_randomness === 'function') {
            wasm.set_global_randomness(msg.globalRandomness);
        }
    }
    
    const totalOps = wasm.run(msg.pair_n, 128);
    const pair_n = msg.pair_n;
    self.postMessage({
        batch: wasm.batch.slice(0, msg.batch.length),
        write_count: wasm.write_count.slice(0, pair_n*2),
        pair_n, ofs:msg.ofs, totalOps
    });
}
