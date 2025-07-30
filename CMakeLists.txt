cmake_minimum_required(VERSION 4.0)
project(rfl_test C CXX)

message(${CMAKE_SOURCE_DIR})
enable_testing()
#set(CMAKE_CXX_FLAGS "/std:c++20")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /std:c++20")
message(${CMAKE_CXX_FLAGS})
if(WIN32)
    message("this is windows")

   foreach(NameAndValue ${ConfigContents})
        # Strip leading spaces
        string(REGEX REPLACE "^[ ]+" "" NameAndValue ${NameAndValue})
        # Find variable name
        string(REGEX MATCH "^[^=]+" Name ${NameAndValue})
        # Find the value
        string(REPLACE "${Name}=" "" Value ${NameAndValue})
        # Set the variable
        set(${Name} "${Value}")
    endforeach()
    message(${CMAKE_TOOLCHAIN_FILE})

elseif(LINUX)
    message("this is LINUX")
else()
message("what is this...")
endif()

#find_package(metis REQUIRED)
find_package(spdlog REQUIRED)



find_package(reflectcpp CONFIG REQUIRED)
find_package(Eigen3  CONFIG REQUIRED)


# サブモジュールを追加 (テストとサンプルは不要なのでOFFにする)
# add_subdirectory を呼び出す前に、サブディレクトリのオプションを設定
set(BUILD_TESTS OFF CACHE BOOL "Disable tests for rfl_eigen_serdes" FORCE)
set(BUILD_SAMPLE OFF CACHE BOOL "Disable sample for rfl_eigen_serdes" FORCE)

add_subdirectory(external/rfl_eigen_serdes EXCLUDE_FROM_ALL)


# PkgConfigモジュールのインクルード


# cbc パッケージの発見

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()


add_executable(main ./old_command_pattern/main.cpp)
target_link_libraries(main PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen  )

add_executable(eigen_sd ./old_command_pattern/serial_deserial_test_eigen3.cpp)
target_link_libraries(eigen_sd PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen     rfl_eigen_serdes)
