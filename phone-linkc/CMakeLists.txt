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

# macOS 特定处理：如果 pkg-config 没找到，尝试直接使用 Homebrew 路径
if(APPLE AND NOT LIBIMOBILEDEVICE_AVAILABLE)
    message(STATUS "尝试使用 Homebrew 路径查找 libimobiledevice...")
    
    # 首先尝试使用环境变量中的路径
    if(DEFINED ENV{HOMEBREW_PREFIX})
        set(HOMEBREW_PREFIX $ENV{HOMEBREW_PREFIX})
        message(STATUS "使用环境变量 HOMEBREW_PREFIX: ${HOMEBREW_PREFIX}")
    else()
        # 尝试查找 Homebrew 安装的库
        execute_process(
            COMMAND brew --prefix
            OUTPUT_VARIABLE HOMEBREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE BREW_RESULT
        )
        if(BREW_RESULT EQUAL 0)
            message(STATUS "使用 brew --prefix 找到路径: ${HOMEBREW_PREFIX}")
        else()
            set(HOMEBREW_PREFIX "/usr/local")
            message(STATUS "brew 命令失败，使用默认路径: ${HOMEBREW_PREFIX}")
        endif()
    endif()
    
    message(STATUS "Homebrew prefix: ${HOMEBREW_PREFIX}")
    
    # 检查头文件和库文件是否存在
    set(IDEVICE_HEADER_FOUND FALSE)
    set(PLIST_HEADER_FOUND FALSE)
    
    if(EXISTS "${HOMEBREW_PREFIX}/include/libimobiledevice/libimobiledevice.h")
        set(IDEVICE_HEADER_FOUND TRUE)
        message(STATUS "✓ libimobiledevice.h found at ${HOMEBREW_PREFIX}/include/libimobiledevice/")
    else()
        message(STATUS "✗ libimobiledevice.h not found at ${HOMEBREW_PREFIX}/include/libimobiledevice/")
    endif()
    
    if(EXISTS "${HOMEBREW_PREFIX}/include/plist/plist.h")
        set(PLIST_HEADER_FOUND TRUE)
        message(STATUS "✓ plist.h found at ${HOMEBREW_PREFIX}/include/plist/")
    else()
        message(STATUS "✗ plist.h not found at ${HOMEBREW_PREFIX}/include/plist/")
    endif()
    
    if(IDEVICE_HEADER_FOUND AND PLIST_HEADER_FOUND)
        # 设置包含目录
        set(IMOBILEDEVICE_INCLUDE_DIRS "${HOMEBREW_PREFIX}/include")
        set(PLIST_INCLUDE_DIRS "${HOMEBREW_PREFIX}/include")
        set(USBMUXD_INCLUDE_DIRS "${HOMEBREW_PREFIX}/include")
        
        # 设置库路径
        set(IMOBILEDEVICE_LIBRARIES "-L${HOMEBREW_PREFIX}/lib -limobiledevice")
        set(PLIST_LIBRARIES "-L${HOMEBREW_PREFIX}/lib -lplist")
        set(USBMUXD_LIBRARIES "-L${HOMEBREW_PREFIX}/lib -lusbmuxd")
        
        # 设置编译标志
        set(IMOBILEDEVICE_CFLAGS_OTHER "-I${HOMEBREW_PREFIX}/include")
        set(PLIST_CFLAGS_OTHER "-I${HOMEBREW_PREFIX}/include")
        set(USBMUXD_CFLAGS_OTHER "-I${HOMEBREW_PREFIX}/include")
        
        # 设置找到标志
        set(IMOBILEDEVICE_FOUND TRUE)
        set(PLIST_FOUND TRUE)
        set(USBMUXD_FOUND TRUE)
        set(LIBIMOBILEDEVICE_AVAILABLE TRUE)
        
        message(STATUS "通过 Homebrew 路径找到 libimobiledevice")
        message(STATUS "  - Include dirs: ${IMOBILEDEVICE_INCLUDE_DIRS}")
        message(STATUS "  - Libraries: ${IMOBILEDEVICE_LIBRARIES}")
    else()
        message(WARNING "Homebrew 路径下未找到完整的 libimobiledevice 库")
        message(STATUS "  - libimobiledevice.h: ${IDEVICE_HEADER_FOUND}")
        message(STATUS "  - plist.h: ${PLIST_HEADER_FOUND}")
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
    # 验证头文件是否真的可以找到 - 使用简单的文件存在检查
    set(HAVE_IDEVICE_HEADER FALSE)
    set(HAVE_PLIST_HEADER FALSE)
    
    # 检查 libimobiledevice 头文件
    foreach(dir ${IMOBILEDEVICE_INCLUDE_DIRS})
        if(EXISTS "${dir}/libimobiledevice/libimobiledevice.h")
            set(HAVE_IDEVICE_HEADER TRUE)
            message(STATUS "Found libimobiledevice header at: ${dir}/libimobiledevice/libimobiledevice.h")
            break()
        endif()
    endforeach()
    
    # 检查 plist 头文件
    foreach(dir ${PLIST_INCLUDE_DIRS})
        if(EXISTS "${dir}/plist/plist.h")
            set(HAVE_PLIST_HEADER TRUE)
            message(STATUS "Found plist header at: ${dir}/plist/plist.h")
            break()
        endif()
    endforeach()
    
    # 输出调试信息
    message(STATUS "Header check results:")
    message(STATUS "  - HAVE_IDEVICE_HEADER: ${HAVE_IDEVICE_HEADER}")
    message(STATUS "  - HAVE_PLIST_HEADER: ${HAVE_PLIST_HEADER}")
    
    if(HAVE_IDEVICE_HEADER AND HAVE_PLIST_HEADER)
        target_compile_definitions(phone-linkc PRIVATE HAVE_LIBIMOBILEDEVICE)
        
        # 链接库 - 直接查找库文件
        find_library(IMOBILEDEVICE_LIB
            NAMES imobiledevice-1.0 libimobiledevice-1.0
            PATHS ${HOMEBREW_PREFIX}/lib ${HOMEBREW_PREFIX}/Cellar/libimobiledevice/*/lib
            NO_DEFAULT_PATH
        )
        
        find_library(PLIST_LIB
            NAMES plist-2.0 libplist-2.0
            PATHS ${HOMEBREW_PREFIX}/lib ${HOMEBREW_PREFIX}/Cellar/libplist/*/lib
            NO_DEFAULT_PATH
        )
        
        find_library(USBMUXD_LIB
            NAMES usbmuxd-2.0 libusbmuxd-2.0
            PATHS ${HOMEBREW_PREFIX}/lib ${HOMEBREW_PREFIX}/Cellar/libusbmuxd/*/lib
            NO_DEFAULT_PATH
        )
        
        message(STATUS "Found libraries:")
        message(STATUS "  - IMOBILEDEVICE_LIB: ${IMOBILEDEVICE_LIB}")
        message(STATUS "  - PLIST_LIB: ${PLIST_LIB}")
        message(STATUS "  - USBMUXD_LIB: ${USBMUXD_LIB}")
        
        if(IMOBILEDEVICE_LIB AND PLIST_LIB)
            target_link_libraries(phone-linkc PRIVATE ${IMOBILEDEVICE_LIB} ${PLIST_LIB})
            if(USBMUXD_LIB)
                target_link_libraries(phone-linkc PRIVATE ${USBMUXD_LIB})
            endif()
        else()
            message(WARNING "Could not find required libraries for libimobiledevice")
        endif()
        
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
        
        # USBMUXD 库已通过上面的 find_library 处理，不需要额外配置
        
        message(STATUS "libimobiledevice support enabled")
        message(STATUS "  - libimobiledevice include dirs: ${IMOBILEDEVICE_INCLUDE_DIRS}")
        message(STATUS "  - libplist include dirs: ${PLIST_INCLUDE_DIRS}")
    else()
        message(WARNING "libimobiledevice libraries found but headers not accessible. iOS device features will be disabled.")
        message(STATUS "  - HAVE_IDEVICE_HEADER: ${HAVE_IDEVICE_HEADER}")
        message(STATUS "  - HAVE_PLIST_HEADER: ${HAVE_PLIST_HEADER}")
        message(STATUS "  - IMOBILEDEVICE_INCLUDE_DIRS: ${IMOBILEDEVICE_INCLUDE_DIRS}")
        message(STATUS "  - PLIST_INCLUDE_DIRS: ${PLIST_INCLUDE_DIRS}")
    endif()
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
