cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug || exit 1
cmake --build build -j || exit 1
./build/tinycraft