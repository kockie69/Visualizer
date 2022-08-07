if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    execute_process(COMMAND "${CMAKE_COMMAND}"
            -G "Visual Studio 17 2022"
            -A "X64"
            -S "$ENV{GITHUB_WORKSPACE}/dep/projectm"
            -B "$ENV{GITHUB_WORKSPACE}/dep/projectm/cmake-build"
            -DTARGET_TRIPLET=x64-windows
            -DCMAKE_VERBOSE_MAKEFILE=YES
            -DENABLE_OPENMP=OFF
            -DENABLE_THREADING=OFF 
            -DENABLE_SDL=OFF
            -DENABLE_OPENMP=OFF
            -DENABLE_SHARED_LIB=OFF
            "-DCMAKE_INSTALL_PREFIX=$ENV{GITHUB_WORKSPACE}/dep/projectm/cmake-install"
            "-DCMAKE_TOOLCHAIN_FILE=$ENV{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake"

            RESULT_VARIABLE result
            )
elseif("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
    execute_process(COMMAND "${CMAKE_COMMAND}"
            -G "Unix Makefiles"
            -S "$ENV{GITHUB_WORKSPACE}/dep/projectm"
            -B "$ENV{GITHUB_WORKSPACE}/dep/projectm/cmake-build"
            -DCMAKE_VERBOSE_MAKEFILE=YES
            -DENABLE_OPENMP=OFF
            -DENABLE_SDL=OFF
            -DCMAKE_BUILD_TYPE=$ENV{BUILD_TYPE}
            "-DCMAKE_INSTALL_PREFIX=$ENV{GITHUB_WORKSPACE}/dep/projectm/cmake-install"

            RESULT_VARIABLE result
            )
elseif("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Darwin")
    execute_process(COMMAND "${CMAKE_COMMAND}"
            -G "Unix Makefiles"
            -S "$ENV{GITHUB_WORKSPACE}/dep/projectm"
            -B "$ENV{GITHUB_WORKSPACE}/dep/projectm/cmake-build"
            -DCMAKE_VERBOSE_MAKEFILE=YES
            -DENABLE_OPENMP=OFF
            -DENABLE_SDL=OFF
            "-DCMAKE_INSTALL_PREFIX=$ENV{GITHUB_WORKSPACE}/dep/projectm/cmake-install"

            RESULT_VARIABLE result
            )
endif()

if(NOT result EQUAL 0)
    message(FATAL_ERROR "CMake returned bad exit status")
endif()
