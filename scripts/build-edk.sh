cmake -S libs/EDK -B libs/EDK/build -G Ninja \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER="$PWD/scripts/edk-clang++" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TEST=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_TOOLS=OFF \
    -DBUILD_SWIG=OFF \
    -DBUILD_WRAPPERS=OFF
cmake --build libs/EDK/build