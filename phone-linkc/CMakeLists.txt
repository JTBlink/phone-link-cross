cmake_minimum_required(VERSION 3.19)
project(phone-linkc LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network LinguistTools)

# 查找 libimobiledevice（可选）
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(IMOBILEDEVICE libimobiledevice-1.0)
    pkg_check_modules(PLIST libplist-2.0)
    pkg_check_modules(USBMUXD libusbmuxd-2.0)
endif()

qt_standard_project_setup()

qt_add_executable(phone-linkc
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    devicemanager.cpp
    devicemanager.h
    deviceinfo.cpp  
    deviceinfo.h
)

qt_add_translations(
    TARGETS phone-linkc
    TS_FILES phone-linkc_zh_CN.ts
)

target_link_libraries(phone-linkc
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
)

# 如果找到 libimobiledevice，则启用相关功能
if(IMOBILEDEVICE_FOUND AND PLIST_FOUND)
    target_compile_definitions(phone-linkc PRIVATE HAVE_LIBIMOBILEDEVICE)
    target_link_libraries(phone-linkc
        PRIVATE
        ${IMOBILEDEVICE_LIBRARIES}
        ${PLIST_LIBRARIES}
    )
    target_include_directories(phone-linkc
        PRIVATE
        ${IMOBILEDEVICE_INCLUDE_DIRS}
        ${PLIST_INCLUDE_DIRS}
    )
    
    if(USBMUXD_FOUND)
        target_link_libraries(phone-linkc PRIVATE ${USBMUXD_LIBRARIES})
        target_include_directories(phone-linkc PRIVATE ${USBMUXD_INCLUDE_DIRS})
    endif()
    
    message(STATUS "libimobiledevice support enabled")
else()
    message(WARNING "libimobiledevice not found. iOS device features will be disabled.")
endif()

include(GNUInstallDirs)

install(TARGETS phone-linkc
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET phone-linkc
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
