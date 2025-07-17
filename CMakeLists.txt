cmake_minimum_required(VERSION 3.14)

set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(nekoray VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# Find Qt
if (NOT QT_VERSION_MAJOR)
    set(QT_VERSION_MAJOR 6)
endif ()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Svg LinguistTools)

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

find_package(Threads)

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
    GIT_TAG master
)
FetchContent_Declare(
    qrcodegen
    GIT_REPOSITORY https://github.com/nayuki/QR-Code-generator.git
    GIT_TAG master
)
FetchContent_MakeAvailable(QJsonModel QHotkey yaml-cpp zxing-cpp qrcodegen)

set_target_properties(ZXing PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

# Sources
set(PROJECT_SOURCES
        ${PLATFORM_SOURCES}

        main/main.cpp
        main/NekoGui.cpp
        main/NekoGui_Utils.cpp
        main/HTTPRequestHelper.cpp

        ${qrcodegen_SOURCE_DIR}/cpp/qrcodegen.cpp

        3rdparty/QtExtKeySequenceEdit.cpp

        3rdparty/qv2ray/v2/ui/LogHighlighter.cpp
        3rdparty/qv2ray/v2/components/proxy/QvProxyConfigurator.cpp
        3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.cpp
        3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.hpp
        3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.ui

        db/Database.cpp
        db/traffic/TrafficLooper.cpp
        db/ProfileFilter.cpp
        db/ConfigBuilder.cpp

        fmt/AbstractBean.cpp
        fmt/Bean2CoreObj_box.cpp
        fmt/Bean2External.cpp
        fmt/Bean2Link.cpp
        fmt/Link2Bean.cpp
        fmt/ChainBean.hpp # translate

        sub/GroupUpdater.cpp

        sys/ExternalProcess.cpp
        sys/AutoRun.cpp

        ui/ThemeManager.cpp
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
        res/theme/feiyangqingyun/qss.qrc
        ${QV2RAY_RC}
)

# Qt exe
qt_add_executable(nekoray
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
)

# Target

set_property(TARGET nekoray PROPERTY AUTOUIC ON)
set_property(TARGET nekoray PROPERTY AUTOMOC ON)
set_property(TARGET nekoray PROPERTY AUTORCC ON)

if(APPLE)
    set(MACOSX_ICON ${CMAKE_SOURCE_DIR}/res/nekoray.icns)
    target_sources(nekoray PRIVATE ${MACOSX_ICON})
    set_target_properties(nekoray PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "nekoray.icns"
        RESOURCE ${MACOSX_ICON}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/res/MacOSXBundleInfo.plist
        MACOSX_BUNDLE_GUI_IDENTIFIER moe.nekoray.macosx
        MACOSX_BUNDLE TRUE
    )
endif()

if(WIN32)
    set_target_properties(nekoray PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Target Source Translations

set(TS_FILES
        translations/zh_CN.ts
        translations/fa_IR.ts
        translations/ru_RU.ts
)
set(LUPDATE_OPTIONS
        -locations none -no-obsolete
)
qt_add_lupdate(nekoray TS_FILES ${TS_FILES} OPTIONS ${LUPDATE_OPTIONS})
qt_add_lrelease(nekoray TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE QM_FILES)

configure_file(translations/translations.qrc ${CMAKE_BINARY_DIR} COPYONLY)
target_sources(nekoray PRIVATE ${CMAKE_BINARY_DIR}/translations.qrc)

# Target Link

target_include_directories(nekoray PRIVATE
    ${zxing-cpp_SOURCE_DIR}/example
    ${qrcodegen_SOURCE_DIR}/cpp
)

target_link_directories(nekoray PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(nekoray PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets 
        Qt${QT_VERSION_MAJOR}::Network 
        Qt${QT_VERSION_MAJOR}::Svg
        Threads::Threads
        ${PLATFORM_LIBRARIES}
        QJsonModel
        QHotkey::QHotkey 
        yaml-cpp::yaml-cpp
        ZXing::ZXing
        libbox
)

qt_finalize_executable(nekoray)