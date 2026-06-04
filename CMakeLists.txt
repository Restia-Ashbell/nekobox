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

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets Network LinguistTools Concurrent)

if (NKR_CROSS)
    set_property(TARGET Qt6::moc PROPERTY IMPORTED_LOCATION /usr/bin/moc)
    set_property(TARGET Qt6::uic PROPERTY IMPORTED_LOCATION /usr/bin/uic)
    set_property(TARGET Qt6::rcc PROPERTY IMPORTED_LOCATION /usr/bin/rcc)
    set_property(TARGET Qt6::lrelease PROPERTY IMPORTED_LOCATION /usr/bin/lrelease)
    set_property(TARGET Qt6::lupdate PROPERTY IMPORTED_LOCATION /usr/bin/lupdate)
endif ()

#### Platform Variables ####
if (APPLE)
    include("cmake/macos/macos.cmake")
elseif (WIN32)
    include("cmake/windows/windows.cmake")
else ()
    include("cmake/linux/linux.cmake")
endif ()

#### default prefix path ####

if (NOT NKR_DISABLE_LIBS)
    list(APPEND CMAKE_PREFIX_PATH ${NKR_LIBS})
endif ()

message("[CMAKE_PREFIX_PATH] ${CMAKE_PREFIX_PATH}")

# for some cross toolchain
list(APPEND CMAKE_FIND_ROOT_PATH ${CMAKE_PREFIX_PATH})
message("[CMAKE_FIND_ROOT_PATH] ${CMAKE_FIND_ROOT_PATH}")

#### NKR ####

include("cmake/print.cmake")
include("cmake/nkr.cmake")

if (NKR_PACKAGE OR NKR_PACKAGE_MACOS)
    nkr_add_compile_definitions(NKR_CPP_USE_APPDATA)
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
set(PROJECT_SOURCES
        ${PLATFORM_SOURCES}

        main/main.cpp
        main/NekoGui_DataStore.cpp
        main/NekoGui_ConfigItem.cpp
        main/HTTPRequestHelper.cpp

        3rdparty/qv2ray/v2/ui/LogHighlighter.cpp
        3rdparty/qv2ray/v2/components/proxy/QvProxyConfigurator.cpp
        3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.cpp
        3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.hpp
        3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.ui

        db/ProfileManager.cpp
        db/traffic/TrafficLooper.cpp
        db/ProfileFilter.cpp
        db/ConfigBuilder.cpp

        fmt/Bean2CoreObj_box.cpp
        fmt/Bean2External.cpp
        fmt/Bean2Link.cpp
        fmt/Link2Bean.cpp

        sub/GroupUpdater.cpp

        sys/ExternalProcess.cpp
        sys/AdminHelper.cpp
        sys/AutoRun.cpp

        ui/Icon.cpp

        ui/mainwindow_grpc.cpp
        ui/mainwindow.cpp
        ui/mainwindow.h
        ui/mainwindow.ui

        ui/edit/dialog_edit_profile.h
        ui/edit/dialog_edit_profile.cpp
        ui/edit/dialog_edit_profile.ui
        ui/edit/dialog_edit_group.h
        ui/edit/dialog_edit_group.cpp
        ui/edit/dialog_edit_group.ui

        ui/edit/edit_chain.h
        ui/edit/edit_chain.cpp
        ui/edit/edit_chain.ui
        ui/edit/edit_socks_http.h
        ui/edit/edit_socks_http.cpp
        ui/edit/edit_socks_http.ui
        ui/edit/edit_shadowsocks.h
        ui/edit/edit_shadowsocks.cpp
        ui/edit/edit_shadowsocks.ui
        ui/edit/edit_shadowsocksr.h
        ui/edit/edit_shadowsocksr.cpp
        ui/edit/edit_shadowsocksr.ui
        ui/edit/edit_vmess.h
        ui/edit/edit_vmess.cpp
        ui/edit/edit_vmess.ui
        ui/edit/edit_trojan_vless.h
        ui/edit/edit_trojan_vless.cpp
        ui/edit/edit_trojan_vless.ui
        ui/edit/edit_anytls.h
        ui/edit/edit_anytls.cpp
        ui/edit/edit_anytls.ui

        ui/edit/edit_naive.h
        ui/edit/edit_naive.cpp
        ui/edit/edit_naive.ui

        ui/edit/edit_quic.h
        ui/edit/edit_quic.cpp
        ui/edit/edit_quic.ui

        ui/edit/edit_ssh.h
        ui/edit/edit_ssh.cpp
        ui/edit/edit_ssh.ui

        ui/edit/edit_wireguard.h
        ui/edit/edit_wireguard.cpp
        ui/edit/edit_wireguard.ui

        ui/edit/edit_custom.h
        ui/edit/edit_custom.cpp
        ui/edit/edit_custom.ui

        ui/dialog_basic_settings.cpp
        ui/dialog_basic_settings.h
        ui/dialog_basic_settings.ui

        ui/dialog_manage_groups.cpp
        ui/dialog_manage_groups.h
        ui/dialog_manage_groups.ui

        ui/dialog_manage_routes.cpp
        ui/dialog_manage_routes.h
        ui/dialog_manage_routes.ui

        ui/dialog_hotkey.cpp
        ui/dialog_hotkey.h
        ui/dialog_hotkey.ui

        ui/widget/ProxyItem.cpp
        ui/widget/ProxyItem.h
        ui/widget/ProxyItem.ui
        ui/widget/GroupItem.cpp
        ui/widget/GroupItem.h
        ui/widget/GroupItem.ui

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