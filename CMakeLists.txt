cmake_minimum_required(VERSION 3.19)

set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(nekobox LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE MATCHES "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif ()

# Version string from git
execute_process(
        COMMAND git describe --tags --always --dirty
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_compile_definitions(NKR_VERSION="${GIT_VERSION}")

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets Network LinguistTools Concurrent)

# Platform-specific sources, libraries and options
if (WIN32)
    set(PLATFORM_SOURCES res/neko.rc)
    set(PLATFORM_LIBRARIES wininet rasapi32)
    add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)
    if (MSVC)
        add_compile_options("/utf-8")
    endif ()
endif ()

# Package builds store config in the OS appdata dir
if (NKR_PACKAGE)
    add_compile_definitions(NKR_USE_APPDATA)
endif ()

set(BUILD_SHARED_LIBS OFF)
set(QHOTKEY_INSTALL OFF)
set(YAML_CPP_BUILD_TOOLS OFF)
set(ZXING_EXAMPLES OFF)

include(FetchContent)
FetchContent_Declare(
        QJsonModel
        GIT_REPOSITORY https://github.com/dridk/QJsonModel.git
        GIT_TAG master
)
FetchContent_Declare(
        QHotkey
        GIT_REPOSITORY https://github.com/Skycoder42/QHotkey.git
        GIT_TAG master
)
FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG master
)
FetchContent_Declare(
        zxing-cpp
        GIT_REPOSITORY https://github.com/zxing-cpp/zxing-cpp.git
        GIT_TAG v2.3.0
        EXCLUDE_FROM_ALL
)
if (EXISTS ${CMAKE_SOURCE_DIR}/../sing-box)
    FetchContent_Declare(
        sing-box
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/../sing-box
    )
else()
    FetchContent_Declare(
        sing-box
        GIT_REPOSITORY https://github.com/Restia-Ashbell/sing-box.git
        GIT_TAG nekobox
    )
endif()
FetchContent_MakeAvailable(QJsonModel QHotkey yaml-cpp zxing-cpp sing-box)

set_target_properties(ZXing PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

set(LIBBOX_SRC_DIR ${sing-box_SOURCE_DIR})
set(LIBBOX_OUTPUT "${CMAKE_BINARY_DIR}/libbox${CMAKE_SHARED_LIBRARY_SUFFIX}")

execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${LIBBOX_SRC_DIR}
        OUTPUT_VARIABLE LIBBOX_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

file(GLOB_RECURSE LIBBOX_SOURCES "${LIBBOX_SRC_DIR}/*.go")
if(MSVC)
    set(LIBBOX_LIB "${CMAKE_BINARY_DIR}/libbox.lib")
    add_custom_command(
        OUTPUT ${LIBBOX_OUTPUT} ${LIBBOX_LIB}
        COMMAND go build -v -buildmode=c-shared -trimpath
                -ldflags "-w -s -X github.com/sagernet/sing-box/constant.Version=${LIBBOX_GIT_HASH} -checklinkname=0 -extldflags '-Wl,--out-implib,${LIBBOX_LIB}'"
                -tags "with_clash_api,with_gvisor,with_quic,with_wireguard,with_utls,with_dhcp,badlinkname,tfogo_checklinkname0"
                -o ${LIBBOX_OUTPUT} ./cmd/sing-box
        WORKING_DIRECTORY ${LIBBOX_SRC_DIR}
        DEPENDS ${LIBBOX_SOURCES}
        COMMENT "Building Go shared library and converting to MSVC .lib"
    )
    set(LIBBOX_LINK_TARGET ${LIBBOX_LIB})
else()
    add_custom_command(
        OUTPUT ${LIBBOX_OUTPUT}
        COMMAND go build -v -buildmode=c-shared -trimpath
                -ldflags "-w -s -X github.com/sagernet/sing-box/constant.Version=${LIBBOX_GIT_HASH} -checklinkname=0"
                -tags "with_clash_api,with_gvisor,with_quic,with_wireguard,with_utls,with_dhcp,badlinkname,tfogo_checklinkname0"
                -o ${LIBBOX_OUTPUT} ./cmd/sing-box
        WORKING_DIRECTORY ${LIBBOX_SRC_DIR}
        DEPENDS ${LIBBOX_SOURCES}
        COMMENT "Building Go shared library"
    )
    set(LIBBOX_LINK_TARGET ${LIBBOX_OUTPUT})
endif()

add_custom_target(BuildCore ALL DEPENDS ${LIBBOX_OUTPUT})

# Sources
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
        src/*
        3rdparty/*
)

list(APPEND PROJECT_SOURCES
        ${PLATFORM_SOURCES}
        res/neko.qrc
)

# Qt exe
qt_add_executable(nekobox
        WIN32 MACOSX_BUNDLE
        ${PROJECT_SOURCES}
)
add_dependencies(nekobox BuildCore)

# Target

set_property(TARGET nekobox PROPERTY AUTOUIC ON)
set_property(TARGET nekobox PROPERTY AUTOMOC ON)
set_property(TARGET nekobox PROPERTY AUTORCC ON)

if (APPLE)
    set(MACOSX_ICON ${CMAKE_SOURCE_DIR}/res/nekobox.icns)
    target_sources(nekobox PRIVATE ${MACOSX_ICON})
    set_target_properties(nekobox PROPERTIES
            MACOSX_BUNDLE_ICON_FILE "nekobox.icns"
            RESOURCE ${MACOSX_ICON}
            MACOSX_BUNDLE_GUI_IDENTIFIER moe.nekobox.macosx
    )
endif ()

if (WIN32)
    qt_add_resources(nekobox "emoji_fonts"
        BASE "res"
        FILES
            res/fonts/NotoColorEmoji-flagsonly.ttf
    )
endif ()

# Target Source Translations

set(TS_FILES
        translations/zh_CN.ts
        translations/fa_IR.ts
        translations/ru_RU.ts
)
qt_add_translations(nekobox TS_FILES ${TS_FILES} MERGE_QT_TRANSLATIONS)

# Target Link

target_include_directories(nekobox PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${zxing-cpp_SOURCE_DIR}/example
)

target_link_libraries(nekobox PRIVATE
        Qt::Widgets
        Qt::Network
        Qt::Concurrent
        ${PLATFORM_LIBRARIES}
        QJsonModel
        QHotkey::QHotkey
        yaml-cpp::yaml-cpp
        ZXing::ZXing
        ${LIBBOX_LINK_TARGET}
)

install(TARGETS nekobox
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(deploy_tool_options_arg "")
set(DEPLOY_EXTRA_ARGS "")
if(APPLE)
    set(deploy_tool_options_arg -libpath=${CMAKE_BINARY_DIR})
elseif(WIN32)
    set(deploy_tool_options_arg --no-system-d3d-compiler --no-system-dxc-compiler --no-opengl-sw)
    install(FILES ${LIBBOX_OUTPUT} DESTINATION ${CMAKE_INSTALL_BINDIR})
elseif(LINUX)
    set_target_properties(nekobox PROPERTIES
        INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
    )
    set(DEPLOY_EXTRA_ARGS
        INCLUDE_PLUGINS qwayland
    )
endif()

qt_generate_deploy_app_script(
    TARGET nekobox
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS ${deploy_tool_options_arg}
    ${DEPLOY_EXTRA_ARGS}
)
install(SCRIPT ${deploy_script})