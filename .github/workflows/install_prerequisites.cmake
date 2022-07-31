message(STATUS "Using host CMake version: ${CMAKE_VERSION}")

if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    # On Windows, using vcpkg to install and build is the best practice.
    set(VCPKG "$ENV{VCPKG_INSTALLATION_ROOT}/vcpkg.exe")
    execute_process(COMMAND "${VCPKG}" --triplet=x64-windows install glew sdl2

            RESULT_VARIABLE result
            )
endif()

if(NOT result EQUAL 0)
    message(FATAL_ERROR "A command returned bad exit status")
endif()