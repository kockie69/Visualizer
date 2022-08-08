	if [ "$1" == 1 ]; then
        cd dep/projectm/build && cmake -G "Ninja" -DCMAKE_LIBRARY_PATH=dep/lib -DENABLE_OPENMP="OFF" -DCMAKE_BUILD_TYPE=Release -DENABLE_THREADING="OFF" -DENABLE_SDL="OFF" -DCMAKE_INSTALL_PREFIX=../../../dep/ ..
    else
        cd dep/projectm/build && cmake -DCMAKE_LIBRARY_PATH=dep/lib -DENABLE_OPENMP="OFF" -DCMAKE_BUILD_TYPE=Release -DENABLE_THREADING="OFF" -DENABLE_SDL="OFF" -DCMAKE_INSTALL_PREFIX=../../../dep/ ..
    fi
    