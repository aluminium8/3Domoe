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

find_package(libigl CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
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



add_library(ResultRepository source/ResultRepository.cpp)
target_link_libraries(ResultRepository PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen  )
target_include_directories(ResultRepository PRIVATE include)

add_library(CommandProcessor source/CommandProcessor.cpp)
target_link_libraries(CommandProcessor PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen ResultRepository )
target_include_directories(CommandProcessor PRIVATE include)

add_executable(main ./sample/main.cpp)
target_link_libraries(main PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen ResultRepository CommandProcessor )
target_include_directories(main PRIVATE include)





#TODO old files must be deleted
add_executable(_old_main ./old_command_pattern/_main.cpp)
target_link_libraries(_old_main PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen  )

add_executable(_old_eigen_sd ./old_command_pattern/_serial_deserial_test_eigen3.cpp)
target_link_libraries(_old_eigen_sd PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen     rfl_eigen_serdes)

add_executable(_old_accessor ./old_command_pattern/accessor_test.cpp)
target_link_libraries(_old_accessor PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen     rfl_eigen_serdes)
add_executable(_old_generic_access ./old_command_pattern/generic_access.cpp)
target_link_libraries(_old_generic_access PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen     rfl_eigen_serdes)

add_executable(_old_eigen3_rfl_generic_test ./old_command_pattern/eigen3_rfl_generic_test.cpp)
target_link_libraries(_old_eigen3_rfl_generic_test PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen     rfl_eigen_serdes)


add_executable(_old_glfw_eigen3_test ./old_command_pattern/glfw_eigen3_test.cpp)
target_link_libraries(_old_glfw_eigen3_test PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen  glfw glad::glad imgui::imgui rfl_eigen_serdes)
add_executable(_old_glfw_eigen3_draw_test ./old_command_pattern/glfw_eigen3_draw_test.cpp)
target_link_libraries(_old_glfw_eigen3_draw_test PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen  glfw glad::glad imgui::imgui rfl_eigen_serdes)
add_executable(_old_glfw_eigen3_draw_test_from_libigl ./old_command_pattern/glfw_eigen3_draw_test_from_libigl.cpp)
target_link_libraries(_old_glfw_eigen3_draw_test_from_libigl PRIVATE spdlog::spdlog reflectcpp::reflectcpp Eigen3::Eigen  glfw glad::glad imgui::imgui igl::igl_core  rfl_eigen_serdes)
