export AFL_LLVM_LAF_SPLIT_SWITCHES=1
export AFL_LLVM_LAF_TRANSFORM_COMPARES=1
export AFL_LLVM_LAF_SPLIT_COMPARES=1
export CC="afl-clang-fast"
export CXX="afl-clang-fast++"
export CFLAGS="-g -O3 -fsanitize=address -DFUZZING"
export CXXFLAGS="-g -O3 -fsanitize=address -DFUZZING"
export LDFLAGS="-fsanitize=address"

rm -rf build
mkdir build
cd build
cmake  -D CMAKE_BUILD_TYPE=Debug ..

