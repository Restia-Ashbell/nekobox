cmake_minimum_required(VERSION 3.16)

set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(nekoray VERSION 0.1 LANGUAGES CXX)

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

if (WIN32)
    set(LIBBOX_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libbox.dll)
elseif (APPLE)
    set(LIBBOX_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libbox.dylib)
else ()
    set(LIBBOX_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libbox.so)
endif ()
set(LIBBOX_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../sing-box)

if (NOT EXISTS ${LIBBOX_SRC_DIR})
    execute_process(
            COMMAND git clone https://github.com/Restia-Ashbell/sing-box.git ${LIBBOX_SRC_DIR}
    )
endif ()

execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${LIBBOX_SRC_DIR}
        OUTPUT_VARIABLE LIBBOX_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

file(GLOB_RECURSE LIBBOX_SOURCES "${LIBBOX_SRC_DIR}/*.go")
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

add_custom_target(BuildCore ALL DEPENDS ${LIBBOX_OUTPUT})

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
)
FetchContent_MakeAvailable(QJsonModel QHotkey yaml-cpp zxing-cpp)

set_target_properties(ZXing PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

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
        ${QV2RAY_RC}
)

# Qt exe
qt_add_executable(nekoray
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
)
add_dependencies(nekoray BuildCore)

# Target

set_property(TARGET nekoray PROPERTY AUTOUIC ON)
set_property(TARGET nekoray PROPERTY AUTOMOC ON)
set_property(TARGET nekoray PROPERTY AUTORCC ON)

if (APPLE)
    set(MACOSX_ICON ${CMAKE_SOURCE_DIR}/res/nekoray.icns)
    target_sources(nekoray PRIVATE ${MACOSX_ICON})
    set_target_properties(nekoray PROPERTIES
            MACOSX_BUNDLE_ICON_FILE "nekoray.icns"
            RESOURCE ${MACOSX_ICON}
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/res/MacOSXBundleInfo.plist
            MACOSX_BUNDLE_GUI_IDENTIFIER moe.nekoray.macosx
            MACOSX_BUNDLE TRUE
    )
endif ()

if (WIN32)
    set_target_properties(nekoray PROPERTIES WIN32_EXECUTABLE TRUE)

    qt_add_resources(nekoray "emoji_fonts"
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
qt_add_translations(nekoray TS_FILES ${TS_FILES} MERGE_QT_TRANSLATIONS)

# Target Link

target_include_directories(nekoray PRIVATE
        ${zxing-cpp_SOURCE_DIR}/example
)

target_link_libraries(nekoray PRIVATE
        Qt::Widgets
        Qt::Network
        Qt::Concurrent
        ${PLATFORM_LIBRARIES}
        QJsonModel
        QHotkey::QHotkey
        yaml-cpp::yaml-cpp
        ZXing::ZXing
        ${LIBBOX_OUTPUT}
)

qt_finalize_executable(nekoray)