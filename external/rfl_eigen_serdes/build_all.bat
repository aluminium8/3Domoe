@echo off
REM 環境変数を使ってvcpkgのパスを指定する
REM cmake -S ./ -B build  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake  
REM テスト用コードもコンパイルする場合
cmake -S ./ -B build  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake  -DBUILD_TESTS=ON





cmake --build build --config Debug -j
cmake --build build --config Release -j