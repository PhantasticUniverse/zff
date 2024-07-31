import { prepareWASM } from "./util.js";
import { byte2asm } from "./z80disasm.js";

const $ = q => document.querySelector(q);

const stat = $('#stat');
const seedInput = $('#seed');
const glsl = SwissGL($('#c'));

const noiseCoef0 = 1/16;
const thread_n = 4;

let main, z80, tape_len;
const colormap = new Uint8Array(256*4);

const workers = [];
let needReset = true;
let noiseCoef = noiseCoef0;
let pending = 0;
let batch_i = 0;
let opsEMA=0, startTime, batchOps;
let running=true;
let inspectIdx = 0;
let mouseXY = [0,0];

function createRegionGrid() {
    const regionGrid = document.getElementById('regionGrid');
    regionGrid.innerHTML = ''; // Clear existing grid
    for (let y = 0; y < 4; y++) {
        for (let x = 0; x < 4; x++) {
            const region = document.createElement('div');
            region.classList.add('region');
            region.id = `region-${x}-${y}`;
            const tooltip = document.createElement('div');
            tooltip.classList.add('region-tooltip');
            region.appendChild(tooltip);
            regionGrid.appendChild(region);
        }
    }
}

function updateRegionGrid() {
    if (!main) return; // Exit if main is not initialized
    for (let y = 0; y < 4; y++) {
        for (let x = 0; x < 4; x++) {
            const region = document.getElementById(`region-${x}-${y}`);
            if (!region) continue; // Skip if element doesn't exist
            const regionData = main.getRegionData(x, y);
            const tooltip = region.querySelector('.region-tooltip');
            if (!tooltip) continue; // Skip if tooltip doesn't exist
            
            // Update tooltip content
            tooltip.innerHTML = `
                Mutation Multiplier: ${regionData.mutation_multiplier.toFixed(2)}<br>
                Directional Influence:<br>
                - Up: ${regionData.directional_influence[0].toFixed(2)}<br>
                - Down: ${regionData.directional_influence[1].toFixed(2)}<br>
                - Left: ${regionData.directional_influence[2].toFixed(2)}<br>
                - Right: ${regionData.directional_influence[3].toFixed(2)}<br>
                Impassable: ${regionData.impassable ? 'Yes' : 'No'}
            `;

            // // Update region color based on properties
            // const r = Math.floor(regionData.mutation_multiplier * 128);
            // const g = Math.floor((regionData.directional_influence[0] + regionData.directional_influence[1]) * 64);
            // const b = Math.floor((regionData.directional_influence[2] + regionData.directional_influence[3]) * 64);
            // region.style.backgroundColor = `rgb(${r}, ${g}, ${b})`;

            // if (regionData.impassable) {
            //     region.style.backgroundImage = 'repeating-linear-gradient(45deg, #000, #000 10px, #333 10px, #333 20px)';
            // } else {
            //     region.style.backgroundImage = 'none';
            // }
        }
    }
}

function setRegionEnvironments() {
    // Ensure the WASM module is initialized
    if (!main) {
        console.error("WASM module not initialized");
        return;
    }

    // Loop through all 16 regions (4x4 grid)
    for (let y = 0; y < 4; y++) {
        for (let x = 0; x < 4; x++) {
            // Calculate a unique value for each region based on its position
            const regionValue = (x + 1) * (y + 1) / 16;

            // // Set mutation multiplier
            // // Varies from 0.5 to 2.0 across the grid
            // const mutationMultiplier = 0.5 + regionValue * 1.5;
            // main.setRegionMutationMultiplier(x, y, mutationMultiplier);

            // // Set directional influence
            // // Creates a circular flow pattern
            // const up = Math.sin((x / 3) * Math.PI) * 0.5 + 0.5;
            // const down = Math.sin(((x + 2) / 3) * Math.PI) * 0.5 + 0.5;
            // const left = Math.sin((y / 3) * Math.PI) * 0.5 + 0.5;
            // const right = Math.sin(((y + 2) / 3) * Math.PI) * 0.5 + 0.5;
            // main.setRegionDirectionalInfluence(x, y, up, down, left, right);

            // Set impassable

            // // Makes the corners impassable
            // const impassable = (x === 0 || x === 3) && (y === 0 || y === 3);
            const impassable = (x === 1 && (y != 1 && y != 3)) || (x === 2 && (y != 1 && y != 3));

            main.setRegionImpassable(x, y, impassable);
        }
    }
    updateRegionGrid();
}

function requestReset() {
    needReset = true;
    console.log('reset');
    
    const seed = parseInt(seedInput.value);
    main.init(seed);
    
    // batch_i = 0;
    // opsEMA = 0;
    
    // scheduleBatch();

    // setRegionEnvironments();
    // updateRegionGrid();
}
$('#reset').onclick = requestReset;

function playPause() {
    running = !running;
    $('#playPause').innerText = running ? "pause" : "play";
    if (running) {
        scheduleBatch();
    }    
}

$('#playPause').onclick = playPause;

document.addEventListener('keydown', function(event) {
    if (event.key === ' ' || event.code === 'Space') { 
        playPause();
    }
});

const clip = (v,a,b) => Math.max(a, Math.min(v, b));

$('#c').addEventListener('mousemove', e => {
    const {width, height} = $('#c');
    const soup_w = main.get_soup_width();
    const soup_h = main.get_soup_height();
    mouseXY = [e.offsetX/width, e.offsetY/height];
    const x = clip(Math.floor(mouseXY[0]*soup_w), 0, soup_w-1);
    const y = clip(Math.floor(mouseXY[1]*soup_h), 0, soup_h-1);
    inspectIdx = y*soup_w+x;
    const p = inspectIdx*tape_len;
    z80.batch.set(main.soup.slice(p, p+tape_len*2));
    z80.z80_trace(128);
});

function updateNoise() {
    const v = 2**$('#noise').value;
    $('#noiseLabel').innerText = `1/${v}`;
    noiseCoef = 1.0/v;
}
updateNoise();
$('#noise').addEventListener("input", updateNoise);

function scheduleBatch() {
    if (!running || pending) {
        return;
    }
    if (needReset) {
        const seed = parseInt(seedInput.value);
        main.init(seed);
        setRegionEnvironments(); // Reapply region environments after init
        batch_i = 0;
        needReset = false;
    }
    startTime = Date.now();
    batchOps = 0;
    const pair_n = main.prepare_batch();
    const job_n = pending = workers.length;
    const chunks = Array(job_n).fill(0).map((_,i) => Math.floor(i*pair_n/job_n));
    chunks.push(pair_n);

    // Create a copy of the regions data
    const regionsDataCopy = main.regions_data.slice();

    for (let i=0; i<job_n; ++i) {
        const start=chunks[i], end=chunks[i+1];
        workers[i].postMessage({
            batch: main.batch.slice(start*tape_len*2, end*tape_len*2),
            ofs: start, 
            pair_n: end-start,
            regions_data: regionsDataCopy
        });
    }
}

function onmessage(e) {
    const msg = e.data;
    main.batch.set(msg.batch, msg.ofs*tape_len*2);
    main.batch_write_count.set(msg.write_count, msg.ofs);
    batchOps += msg.totalOps;
    --pending;
    if (pending) {
        return;
    }
    main.absorb_batch();
    const pair_n = main.batch_pair_n[0];
    main.mutate(Math.floor(pair_n*noiseCoef));
    ++batch_i;
    const op_per_sec = batchOps / ((Date.now()-startTime)/1000.0);
    opsEMA = opsEMA*0.99 + 0.01*op_per_sec;
    scheduleBatch();
}

const hex = byte => byte.toString(16).padStart(2, '0');
const hexColor = (r,g,b) => `#${hex(r)}${hex(g)}${hex(b)}`;
const colorMatrix = [];

function createMatrix() {
    const matrix = document.getElementById('matrix');
    for (let i = 0; i < 16; i++) {
        for (let j = 0; j < 16; j++) {
            const square = document.createElement('div');
            square.classList.add('square');
            colorMatrix.push(square);
            const tooltip = document.createElement('span');
            tooltip.classList.add('tooltip');
            const code = i*16+j;
            tooltip.textContent = `0x${hex(code)}: ${byte2asm[code]}`;
            square.appendChild(tooltip);
            matrix.appendChild(square);
            square.addEventListener('click', (event) => openColorPicker(event, code));
        }
    }
}

function openColorPicker(event, code) {
    const colorPicker = document.getElementById('colorPicker');
    const [r,g,b] = colormap.slice(code*4,code*4+3);
    colorPicker.value = hexColor(r,g,b);
    
    const rect = event.target.getBoundingClientRect();
    const scrollX = window.scrollX;
    const scrollY = window.scrollY;
    
    colorPicker.style.left = (rect.left + scrollX) + 'px';
    colorPicker.style.top = (rect.bottom + scrollY + 5) + 'px';
    colorPicker.style.display = 'block';
    colorPicker.oninput = e => {
        const s = colorPicker.value;
        colorMatrix[code].style.backgroundColor = s;
        const rgb = [1,3,5].map(i => parseInt(s.slice(i,i+2), 16));
        colormap.set(rgb, code*4);
    }
    
    event.stopPropagation();

    setTimeout(() => colorPicker.click(), 0);
}

function closeColorPicker() {
    document.getElementById('colorPicker').style.display = 'none';
}

document.addEventListener('click', event => {
    if (event.target.closest('#colorPicker') === null) {
        closeColorPicker();
    }
});

function updateColormap() {
    for (let i=0; i<256; ++i) {
        const [r,g,b] = colormap.slice(i*4,i*4+3);
        colorMatrix[i].style.backgroundColor = hexColor(r,g,b);
    }
}

createMatrix();

function frame() {
    if (!main) {
        requestAnimationFrame(frame);
        return;
    }
    const w = main.get_soup_width();
    const h = main.get_soup_height();
    const soup = glsl({}, {data:main.soup, size:[w*tape_len, h], format:'r8', tag:'soup'});
    const cmap = glsl({}, {data:colormap, size:[256, 1], tag:'cmap'});

    // //Render environment
    // const soup2d = glsl({
    //     soup, 
    //     cmap, 
    //     regions_data: main.regions_data,
    //     FP:`
    //     uniform float regions_data[16];
    
    //     void fragment() {
    //             const int TapeLen=${tape_len}, Tile=4;
    //             ivec2 tapeXY=I/Tile, xy=I%Tile;
    //             int ofs = xy.y*Tile+xy.x;
    //             float v = soup(ivec2(tapeXY.x*TapeLen+ofs, tapeXY.y)).r;
                
    //             // Calculate the region coordinates
    //             int regionX = int(float(tapeXY.x) / float(${main.get_soup_width()} / 4));
    //             int regionY = int(float(tapeXY.y) / float(${main.get_soup_height()} / 4));
    //             int regionIndex = regionY * 4 + regionX;
                
    //             // Check if the region is impassable
    //             float isImpassable = regions_data[regionIndex];
                
    //             // If the region is impassable, set the color to black, otherwise use the color from the colormap
    //             vec4 color = cmap(vec2(v,0));
    //             color = mix(color, vec4(0.0, 0.0, 0.0, 1.0), step(0.5, isImpassable));
                
    //             // Debug: Visualize region boundaries and impassable status
    //             if (mod(float(tapeXY.x), float(${main.get_soup_width()} / 4)) < 1.0 || mod(float(tapeXY.y), float(${main.get_soup_height()} / 4)) < 1.0) {
    //                 color = vec4(1.0, 0.0, 0.0, 1.0);  // Red lines for region boundaries
    //             }
    //             color = mix(color, vec4(0.0, 1.0, 0.0, 1.0), step(0.9, isImpassable) * 0.5);  // Green tint for impassable regions

    //             FOut = color;
    //         }
    // `}, {size:[w*4, h*4], tag:'soup2d'});

    const soup2d = glsl({soup, cmap, FP:`
        const int TapeLen=${tape_len}, Tile=4;
        ivec2 tapeXY=I/Tile, xy=I%Tile;
        int ofs = xy.y*Tile+xy.x;
        float v = soup(ivec2(tapeXY.x*TapeLen+ofs, tapeXY.y)).r;
        FOut = cmap(vec2(v,0));
    `}, {size:[w*4, h*4], tag:'soup2d'});
    const soup2dAvg = glsl({soup2d, FP:`
        const int Tile=4;
        vec4 acc = vec4(0);
        for (int y=0; y<Tile; ++y)
        for (int x=0; x<Tile; ++x) {
            vec4 c = soup2d(I*Tile+ivec2(x, y));
            acc += c*c;
        }
        FOut = sqrt(acc / float(Tile*Tile));
    `}, {size:[w, h], tag:'soup2dAvg'});    
    glsl({tex:soup2dAvg, FP:`tex(vec2(UV.x,1.0-UV.y))`});
    const trace = glsl({}, {data:z80.trace_vis, size:[tape_len*2, 128], tag:'trace'});
    glsl({trace, Blend:'d*(1-sa)+s*sa',
        VP:`XY*vec2(1./8.,-0.5)-vec2(0.8, 0.4),0,1`,
        FP:`trace(UV).rgb,0.7`});
    
    main.updateCounts();
    const writes = main.write_count.reduce((a,b) => a+b, 0);
    const lines = [`batch_i: ${batch_i}\nwrites: ${writes}\nop/s: ${(opsEMA/1e6).toFixed(2)}M\n`]
    lines.push('Top codes (count, byte, asm):')
    const count_byte = Array.from(main.counts).map((v,i) => [v,i]).sort((a,b) => b[0]-a[0]);
    for (const [count, byte] of count_byte.slice(0,20)) {
        lines.push(`${count.toString().padStart(8)}  ${hex(byte)}  ${byte2asm[byte]}`)
    }
    lines.push('\nselected cell:');
    for (let i=0; i<tape_len; ++i) {
        const byte = main.soup[inspectIdx*tape_len+i];
        lines.push(`  ${hex(i)}  ${hex(byte)}  ${byte2asm[byte]}`)
    }
    stat.innerText = lines.join('\n');
    requestAnimationFrame(frame);
}
requestAnimationFrame(frame);

for (let i=0; i<thread_n; ++i) {
    const worker = new Worker("js/worker.js", { type: "module" });
    worker.onmessage = onmessage;
    workers.push(worker);
}

function set_color(i, r, g, b) {
    colormap.set([r,g,b,255], i*4);
}

function setColorsFromJyrkiMap() {
    for (let i = 0; i < 256; i++) {
        const color = cmap_jyrki[i];
        const r = (color >> 16) & 0xFF;
        const g = (color >> 8) & 0xFF;
        const b = color & 0xFF;
        set_color(i, r, g, b);
    }
}

async function run() {
    const mainWasm = await WebAssembly.instantiateStreaming(fetch('wasm/main.wasm'));
    const z80Wasm = await WebAssembly.instantiateStreaming(fetch('wasm/z80worker.wasm'));
    self.main = main = prepareWASM(mainWasm.instance);
    self.z80 = z80 = prepareWASM(z80Wasm.instance);

    tape_len = main.get_tape_len();

    setColorsFromJyrkiMap();
    
    updateColormap();

    createRegionGrid();
    
    scheduleBatch();
    
    setRegionEnvironments();
    
    // updateRegionGrid(); // Initial update of the region grid

    // scheduleBatch();
}
run();

const cmap_jyrki = [
    0x000000, 0xFF4080, 0xFF8000, 0xE00000, 0xD00000, 0xD00000, 0xFF2040, 0xC000C0, 0xFFFF40, 0x4040FF, 0xFFFF80, 0xE00000, 0xD00000, 0xD00000, 0xFF2040, 0xC000C0,
    0x800080, 0x40FF80, 0x80FF00, 0x00E000, 0x00D000, 0x00D000, 0x20FF40, 0xC000C0, 0x404080, 0x4040FF, 0xFFFF80, 0x00E000, 0x00D000, 0x00D000, 0x20FF40, 0xC000C0,
    0x4040C0, 0x4080FF, 0x80C0FF, 0x0000E0, 0x0000D0, 0x0000D0, 0x2040FF, 0xC0C000, 0x4040C0, 0x0080FF, 0xFFFFFF, 0x0000E0, 0x0000D0, 0x0000D0, 0x2040FF, 0xC0C000,
    0x4040C0, 0x804080, 0xFFC080, 0x600060, 0x0040E0, 0x0040E0, 0x4080FF, 0xC000C0, 0x4040C0, 0x4040FF, 0xFFFF80, 0x600060, 0xE0E000, 0xE0E000, 0xFFFF40, 0xC000C0,
    0xFF0000, 0xFF0000, 0xFF8000, 0xFF8000, 0xFF0080, 0xFF0080, 0xFF4080, 0xFF8080, 0xFF0000, 0xFF0000, 0xFF8000, 0xFF8000, 0xFF0080, 0xFF0080, 0xFF4080, 0xFF8080,
    0x80FF00, 0x80FF00, 0x00FF00, 0x00FF00, 0x40FF80, 0x40FF80, 0x80FF80, 0x80FF80, 0x80FF00, 0x80FF00, 0x00FF00, 0x00FF00, 0x40FF80, 0x40FF80, 0x80FF80, 0x80FF80,
    0x8000FF, 0x8000FF, 0x80C0FF, 0x80C0FF, 0x0000FF, 0x0000FF, 0x4080FF, 0x8080FF, 0x8000FF, 0x8000FF, 0x80C0FF, 0x80C0FF, 0x0000FF, 0x0000FF, 0x4080FF, 0x8080FF,
    0x4080FF, 0x4080FF, 0x80C0FF, 0x80C0FF, 0x0080FF, 0x0080FF, 0x404040, 0x80C0FF, 0xFFFF00, 0xFFFF00, 0xFFFF80, 0xFFFF80, 0xFFFF80, 0xFFFF80, 0xFFC080, 0xFFFF00,
    0xE0E000, 0xE0E000, 0xE0F000, 0xE0F000, 0xE0E080, 0xE0E080, 0xE0C080, 0xE0E000, 0xE0E000, 0xE0E000, 0xE0F000, 0xE0F000, 0xE0E080, 0xE0E080, 0xE0C080, 0xE0E000,
    0xC0C000, 0xC0C000, 0xC0D000, 0xC0D000, 0xC0C080, 0xC0C080, 0xC0A080, 0xC0C000, 0xC0C000, 0xC0C000, 0xC0D000, 0xC0D000, 0xC0C080, 0xC0C080, 0xC0A080, 0xC0C000,
    0xA0A000, 0xA0A000, 0xA0B000, 0xA0B000, 0xA0A080, 0xA0A080, 0xA08080, 0xA0A000, 0xA0A000, 0xA0A000, 0xA0B000, 0xA0B000, 0xA0A080, 0xA0A080, 0xA08080, 0xA0A000,
    0xA0A000, 0xA0A000, 0xA0B000, 0xA0B000, 0xA0A080, 0xA0A080, 0xA08080, 0xA0A000, 0x808000, 0x808000, 0x809000, 0x809000, 0x808080, 0x808080, 0x806080, 0x808000,
    0x4040C0, 0xFF4040, 0x4040A0, 0x404080, 0x4040A0, 0xFF4040, 0xE0E040, 0x404080, 0x4040C0, 0x404080, 0x4040A0, 0x808080, 0x4040A0, 0x404080, 0xE0E040, 0x404080,
    0x4040C0, 0x40FF40, 0x4040A0, 0xFFC040, 0x4040A0, 0x40FF40, 0xC0C040, 0x404080, 0x4040C0, 0xC0C0C0, 0x4040A0, 0xFFC040, 0x4040A0, 0x808080, 0xC0C040, 0x404080,
    0x4040C0, 0x4080FF, 0x4040A0, 0x8080FF, 0x4040A0, 0x4080FF, 0xA0A040, 0x404080, 0x4040C0, 0x4040FF, 0x4040A0, 0x80FF80, 0x4040A0, 0x808080, 0xA0A040, 0x404080,
    0x4040C0, 0xFFFF40, 0x4040A0, 0xC0C0C0, 0x4040A0, 0xFFFF40, 0xA0A040, 0x404080, 0x4040C0, 0x8080FF, 0x4040A0, 0xC0C0C0, 0x4040A0, 0x808080, 0x808040, 0x404080
    ];