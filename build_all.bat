cmake -S ./ -B build  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
REM cmake --build build --config Debug -j
cmake --build build --config Release -j