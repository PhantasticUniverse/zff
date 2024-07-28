clear
FLAGS="-target wasm32-freestanding-musl -DWASM -fno-entry -O ReleaseSmall -Wl,--export-all -Wl,--no-entry"
cd wasm
zig build-exe main.c environment.c -lc $FLAGS
zig build-exe test_environment.c environment.c -lc $FLAGS
zig build-exe z80worker.c ../external/z80.c -lc $FLAGS
ls -lh *.wasm