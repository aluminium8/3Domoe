cd build
ctest -C Debug --rerun-failed --output-on-failure
ctest -C Release --rerun-failed --output-on-failure
cd ../
