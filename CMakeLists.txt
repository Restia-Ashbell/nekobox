cmake_minimum_required(VERSION 3.28)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(nekobox LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

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
if(WIN32)
    set(PLATFORM_SOURCES res/neko.rc)
    set(PLATFORM_LIBRARIES wininet rasapi32)
    add_compile_definitions(UNICODE _UNICODE NOMINMAX)
    if(MSVC)
        add_compile_options(/utf-8)
    endif()
endif()

# Dependencies
set(BUILD_SHARED_LIBS OFF)
set(QHOTKEY_INSTALL OFF)
set(YAML_CPP_BUILD_TOOLS OFF)
set(YAML_MSVC_SHARED_RT ON)
set(ZXING_EXAMPLES OFF)

include(FetchContent)

FetchContent_Declare(
    QJsonModel
    GIT_REPOSITORY https://github.com/dridk/QJsonModel.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_Declare(
    QHotkey
    GIT_REPOSITORY https://github.com/Skycoder42/QHotkey.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_Declare(
    zxing-cpp
    GIT_REPOSITORY https://github.com/zxing-cpp/zxing-cpp.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_Declare(
    sing-box
    GIT_REPOSITORY https://github.com/Restia-Ashbell/sing-box.git
    GIT_TAG nekobox
    GIT_SHALLOW TRUE
)

if(EXISTS ${CMAKE_SOURCE_DIR}/../sing-box)
    set(FETCHCONTENT_SOURCE_DIR_SING-BOX ${CMAKE_SOURCE_DIR}/../sing-box)
endif()
FetchContent_MakeAvailable(QJsonModel QHotkey yaml-cpp zxing-cpp sing-box)

# internal compiler error
if(MINGW)
    set_target_properties(ZXing PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
endif()

set(LIBBOX_SRC_DIR ${sing-box_SOURCE_DIR})
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${LIBBOX_SRC_DIR}
    OUTPUT_VARIABLE LIBBOX_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(LIBBOX_LDFLAGS "-w -s -X github.com/sagernet/sing-box/constant.Version=${LIBBOX_GIT_HASH} -checklinkname=0")
set(LIBBOX_SHARED_LIBRARY ${CMAKE_BINARY_DIR}/libbox${CMAKE_SHARED_LIBRARY_SUFFIX})
set(LIBBOX_LINK_LIBRARY ${LIBBOX_SHARED_LIBRARY})
set(LIBBOX_OUTPUTS ${LIBBOX_SHARED_LIBRARY})
if(MSVC)
    set(LIBBOX_IMPORT_LIBRARY ${CMAKE_BINARY_DIR}/libbox.lib)
    set(LIBBOX_LINK_LIBRARY ${LIBBOX_IMPORT_LIBRARY})
    list(APPEND LIBBOX_OUTPUTS ${LIBBOX_IMPORT_LIBRARY})
    string(APPEND LIBBOX_LDFLAGS " -extldflags '-Wl,--out-implib,${LIBBOX_IMPORT_LIBRARY}'")
endif()

file(GLOB_RECURSE LIBBOX_SOURCES CONFIGURE_DEPENDS "${LIBBOX_SRC_DIR}/*.go")
add_custom_command(
    OUTPUT ${LIBBOX_OUTPUTS}
    BYPRODUCTS ${CMAKE_BINARY_DIR}/libbox.h
    COMMAND go build -v -buildmode=c-shared -trimpath
            -ldflags "${LIBBOX_LDFLAGS}"
            -tags "with_clash_api,with_gvisor,with_quic,with_wireguard,with_utls,with_dhcp,badlinkname,tfogo_checklinkname0"
            -o ${LIBBOX_SHARED_LIBRARY} ./cmd/sing-box
    WORKING_DIRECTORY ${LIBBOX_SRC_DIR}
    DEPENDS ${LIBBOX_SOURCES} ${LIBBOX_SRC_DIR}/go.mod ${LIBBOX_SRC_DIR}/go.sum
    COMMENT "Building Go shared library"
)
add_custom_target(libbox DEPENDS ${LIBBOX_OUTPUTS})

# Sources
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS src/* 3rdparty/*)

qt_add_executable(nekobox
    WIN32 MACOSX_BUNDLE
    ${PROJECT_SOURCES}
    ${PLATFORM_SOURCES}
    res/neko.qrc
)
add_dependencies(nekobox libbox)

set_target_properties(nekobox PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

if(APPLE)
    set(MACOSX_ICON ${CMAKE_SOURCE_DIR}/res/nekobox.icns)
    target_sources(nekobox PRIVATE ${MACOSX_ICON})
    set_target_properties(nekobox PROPERTIES
        MACOSX_BUNDLE_ICON_FILE nekobox.icns
        RESOURCE ${MACOSX_ICON}
        MACOSX_BUNDLE_GUI_IDENTIFIER moe.nekobox.macosx
    )
endif()

if(WIN32)
    FetchContent_Declare(
        NotoFlagFont
        URL https://raw.githubusercontent.com/googlefonts/noto-emoji/main/2D/fonts/NotoColorEmoji-flagsonly.ttf
        DOWNLOAD_NO_EXTRACT TRUE
        TLS_VERIFY FALSE
    )
    FetchContent_MakeAvailable(NotoFlagFont)
    set(NOTO_FLAG_FONT_FILE ${notoflagfont_SOURCE_DIR}/NotoColorEmoji-flagsonly.ttf)
    set_source_files_properties(${NOTO_FLAG_FONT_FILE} PROPERTIES QT_RESOURCE_ALIAS fonts/NotoColorEmoji-flagsonly.ttf)

    qt_add_resources(nekobox emoji_fonts
        FILES
            ${NOTO_FLAG_FONT_FILE}
    )
endif()

# Translations
qt_add_translations(nekobox
    TS_FILES
        translations/zh_CN.ts
        translations/fa_IR.ts
        translations/ru_RU.ts
    LUPDATE_OPTIONS
        -locations none
        -no-obsolete
    MERGE_QT_TRANSLATIONS
)

target_include_directories(nekobox PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(nekobox
    PRIVATE
        Qt::Widgets
        Qt::Network
        Qt::Concurrent
        ${PLATFORM_LIBRARIES}
        QJsonModel
        QHotkey::QHotkey
        yaml-cpp::yaml-cpp
        ZXing::ZXing
        ${LIBBOX_LINK_LIBRARY}
)

install(TARGETS nekobox
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(APPLE)
    set(deploy_tool_options_arg -libpath=${CMAKE_BINARY_DIR})
elseif(WIN32)
    set(deploy_tool_options_arg --no-system-d3d-compiler --no-system-dxc-compiler --no-opengl-sw)
    install(FILES ${LIBBOX_SHARED_LIBRARY} DESTINATION ${CMAKE_INSTALL_BINDIR})
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
