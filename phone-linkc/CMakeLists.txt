cmake_minimum_required(VERSION 3.19)
project(phone-linkc LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Widgets Network)

# 查找 libimobiledevice（可选）
find_package(PkgConfig QUIET)
set(LIBIMOBILEDEVICE_AVAILABLE FALSE)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(IMOBILEDEVICE QUIET libimobiledevice-1.0)
    pkg_check_modules(PLIST QUIET libplist-2.0)
    pkg_check_modules(USBMUXD QUIET libusbmuxd-2.0)
    
    if(IMOBILEDEVICE_FOUND AND PLIST_FOUND)
        set(LIBIMOBILEDEVICE_AVAILABLE TRUE)
    endif()
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

# 翻译支持暂时禁用以避免构建问题
# TODO: 在未来版本中重新启用翻译支持
message(STATUS "Translation support disabled (to avoid build issues)")

target_link_libraries(phone-linkc
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
)

# 如果找到 libimobiledevice，则启用相关功能
if(LIBIMOBILEDEVICE_AVAILABLE)
    target_compile_definitions(phone-linkc PRIVATE HAVE_LIBIMOBILEDEVICE)
    
    # 链接库
    target_link_libraries(phone-linkc
        PRIVATE
        ${IMOBILEDEVICE_LIBRARIES}
        ${PLIST_LIBRARIES}
    )
    
    # 添加包含目录
    if(IMOBILEDEVICE_INCLUDE_DIRS)
        target_include_directories(phone-linkc PRIVATE ${IMOBILEDEVICE_INCLUDE_DIRS})
    endif()
    if(PLIST_INCLUDE_DIRS)
        target_include_directories(phone-linkc PRIVATE ${PLIST_INCLUDE_DIRS})
    endif()
    
    # 添加编译标志
    if(IMOBILEDEVICE_CFLAGS_OTHER)
        target_compile_options(phone-linkc PRIVATE ${IMOBILEDEVICE_CFLAGS_OTHER})
    endif()
    if(PLIST_CFLAGS_OTHER)
        target_compile_options(phone-linkc PRIVATE ${PLIST_CFLAGS_OTHER})
    endif()
    
    if(USBMUXD_FOUND)
        target_link_libraries(phone-linkc PRIVATE ${USBMUXD_LIBRARIES})
        if(USBMUXD_INCLUDE_DIRS)
            target_include_directories(phone-linkc PRIVATE ${USBMUXD_INCLUDE_DIRS})
        endif()
        if(USBMUXD_CFLAGS_OTHER)
            target_compile_options(phone-linkc PRIVATE ${USBMUXD_CFLAGS_OTHER})
        endif()
    endif()
    
    message(STATUS "libimobiledevice support enabled")
    message(STATUS "  - libimobiledevice include dirs: ${IMOBILEDEVICE_INCLUDE_DIRS}")
    message(STATUS "  - libplist include dirs: ${PLIST_INCLUDE_DIRS}")
else()
    message(WARNING "libimobiledevice not found. iOS device features will be disabled.")
    if(PKG_CONFIG_FOUND)
        message(STATUS "pkg-config was found but libimobiledevice/libplist are missing")
    else()
        message(STATUS "pkg-config not available")
    endif()
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
