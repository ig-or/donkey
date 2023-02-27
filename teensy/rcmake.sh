cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain.cmake ../.

mkdir -p build1
cd build1
cmake -G "Ninja"  -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake  ../. 
cd ..
