cmake_minimum_required(VERSION 4.0)

option(BUILD_TESTS "Build the testing suite." OFF)
option(BUILD_SAMPLE "Build the sample executable." ON)

if(BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "testing")
endif()

project(rfl_eigen_serdes VERSION 1.0.0 LANGUAGES C CXX)

# CTestを有効化
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  add_compile_options(/utf-8)
endif()



# 依存パッケージの検索
find_package(reflectcpp CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

# ==============================================================================
# ライブラリターゲットの定義 (ヘッダーオンリーなのでINTERFACE)
# ==============================================================================
add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        # インストールされたヘッダーへのパス
        $<INSTALL_INTERFACE:include>
        # ビルドツリー（ローカル）でのヘッダーへのパス
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# ライブラリが依存するライブラリを指定
target_link_libraries(${PROJECT_NAME}
    INTERFACE
        reflectcpp::reflectcpp
        Eigen3::Eigen
)


# ==============================================================================
# サンプルコードのビルド設定
# ==============================================================================
if(BUILD_SAMPLE)
    add_executable(sample sample/sample.cpp)
    # 作成したライブラリターゲットにリンクする
    target_link_libraries(sample PRIVATE ${PROJECT_NAME})
endif()


# ==============================================================================
# テストコードのビルド設定
# ==============================================================================
if(BUILD_TESTS)
    find_package(Boost CONFIG REQUIRED COMPONENTS unit_test_framework)

    add_executable(serdes_test test/test_serdes.cpp)
    target_compile_definitions(serdes_test PRIVATE NOMINMAX)
    # 作成したライブラリターゲットにリンクする
    target_link_libraries(serdes_test PRIVATE
        ${PROJECT_NAME}
        Boost::unit_test_framework
    )

    add_test(NAME EigenSerializationTests COMMAND serdes_test)
endif()


# ==============================================================================
# インストールとパッケージ設定
# ==============================================================================
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 1. ターゲットのエクスポート設定
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
)

# 2. ヘッダーファイルのインストール
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 3. <project>Targets.cmakeのインストール
#    find_package()で必要になるターゲット情報が含まれる
install(EXPORT ${PROJECT_NAME}Targets
    FILE      ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
)

# 4. <project>Config.cmake と <project>ConfigVersion.cmake の生成とインストール
#    find_package()が依存関係を解決するために使うヘルパーファイル
# ヘルパーテンプレートを作成
configure_package_config_file(
    "cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
)
# バージョンファイルを作成
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
# 生成したConfigファイルとVersionファイルをインストール
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "lib/cmake/${PROJECT_NAME}"
)