cmake_minimum_required(VERSION 3.19)

# 禁用 Qt Creator 的 package manager auto-setup
# 这些设置必须在 project() 之前
set(QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP TRUE CACHE BOOL "" FORCE)
set(QT_NO_PACKAGE_MANAGER_AUTO_SETUP ON CACHE BOOL "" FORCE)

# Windows Visual Studio 版本兼容性处理
if(WIN32 AND CMAKE_GENERATOR MATCHES "Visual Studio")
    # 检测实际安装的Visual Studio版本
    if(CMAKE_GENERATOR MATCHES "Visual Studio 17 2022")
        # 如果指定VS 2022但未找到，尝试使用VS 2019
        if(NOT EXISTS "C:/Program Files/Microsoft Visual Studio/2022")
            message(STATUS "Visual Studio 2022 未找到，尝试使用 Visual Studio 2019")
            set(CMAKE_GENERATOR "Visual Studio 16 2019" CACHE STRING "Visual Studio Generator" FORCE)
        endif()
    endif()
endif()

# 如果存在 auto-setup.cmake 路径但文件不存在，则创建一个空的
if(DEFINED CMAKE_PROJECT_INCLUDE_BEFORE)
    if(NOT EXISTS "${CMAKE_PROJECT_INCLUDE_BEFORE}")
        get_filename_component(_setup_dir "${CMAKE_PROJECT_INCLUDE_BEFORE}" DIRECTORY)
        if(NOT EXISTS "${_setup_dir}")
            file(MAKE_DIRECTORY "${_setup_dir}")
        endif()
        file(WRITE "${CMAKE_PROJECT_INCLUDE_BEFORE}" "# Auto-generated empty file\n")
    endif()
endif()

project(phone-linkc LANGUAGES C CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# MSVC 编译器配置（全局设置）
# ============================================================================
if(MSVC)
    # Debug 模式编译选项 - 确保生成完整的调试符号 (PDB 文件)
    # /Zi - 生成完整的调试信息 (PDB 文件)
    # /Od - 禁用优化，便于调试
    # /Ob0 - 禁用内联展开
    # /RTC1 - 启用运行时错误检查
    # /MDd - 使用多线程调试 DLL 运行时库
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od /Ob0 /RTC1 /MDd" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi /Od /Ob0 /RTC1 /MDd" CACHE STRING "" FORCE)
    
    # 链接器选项：生成调试信息
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FULL" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DEBUG:FULL" CACHE STRING "" FORCE)
endif()

# 查找线程库
# Windows 上不需要 pthread，使用原生 Windows 线程 API
if(NOT WIN32)
    find_package(Threads REQUIRED)
endif()

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

# ============================================================================
# macOS 特定处理：通过 Homebrew 路径查找 libimobiledevice
# ============================================================================
if(APPLE AND NOT LIBIMOBILEDEVICE_AVAILABLE)
    message(STATUS "尝试使用 Homebrew 路径查找 libimobiledevice...")
    
    # 确定 Homebrew 前缀路径
    if(DEFINED ENV{HOMEBREW_PREFIX})
        set(HOMEBREW_PREFIX $ENV{HOMEBREW_PREFIX})
        message(STATUS "使用环境变量 HOMEBREW_PREFIX: ${HOMEBREW_PREFIX}")
    else()
        execute_process(
            COMMAND brew --prefix
            OUTPUT_VARIABLE HOMEBREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE BREW_RESULT
        )
        if(NOT BREW_RESULT EQUAL 0)
            # ARM64 macOS 默认使用 /opt/homebrew，Intel macOS 使用 /usr/local
            if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
                set(HOMEBREW_PREFIX "/opt/homebrew")
            else()
                set(HOMEBREW_PREFIX "/usr/local")
            endif()
            message(STATUS "brew 命令失败，使用默认路径: ${HOMEBREW_PREFIX}")
        else()
            message(STATUS "使用 brew --prefix 找到路径: ${HOMEBREW_PREFIX}")
        endif()
    endif()
    
    message(STATUS "Homebrew prefix: ${HOMEBREW_PREFIX}")
    
    # 预先获取 Cellar 路径（避免重复调用）
    file(GLOB IMOBILEDEVICE_CELLAR_DIRS "${HOMEBREW_PREFIX}/Cellar/libimobiledevice/*/include")
    file(GLOB PLIST_CELLAR_DIRS "${HOMEBREW_PREFIX}/Cellar/libplist/*/include")
    file(GLOB USBMUXD_CELLAR_DIRS "${HOMEBREW_PREFIX}/Cellar/libusbmuxd/*/include")
    
    # 检查 libimobiledevice 头文件
    set(IDEVICE_HEADER_FOUND FALSE)
    if(EXISTS "${HOMEBREW_PREFIX}/include/libimobiledevice/libimobiledevice.h")
        set(IDEVICE_HEADER_FOUND TRUE)
        message(STATUS "✓ libimobiledevice.h found at ${HOMEBREW_PREFIX}/include/libimobiledevice/")
    else()
        foreach(cellar_dir ${IMOBILEDEVICE_CELLAR_DIRS})
            if(EXISTS "${cellar_dir}/libimobiledevice/libimobiledevice.h")
                set(IDEVICE_HEADER_FOUND TRUE)
                message(STATUS "✓ libimobiledevice.h found at ${cellar_dir}/libimobiledevice/")
                break()
            endif()
        endforeach()
    endif()
    
    # 检查 plist 头文件
    set(PLIST_HEADER_FOUND FALSE)
    if(EXISTS "${HOMEBREW_PREFIX}/include/plist/plist.h")
        set(PLIST_HEADER_FOUND TRUE)
        message(STATUS "✓ plist.h found at ${HOMEBREW_PREFIX}/include/plist/")
    else()
        foreach(cellar_dir ${PLIST_CELLAR_DIRS})
            if(EXISTS "${cellar_dir}/plist/plist.h")
                set(PLIST_HEADER_FOUND TRUE)
                message(STATUS "✓ plist.h found at ${cellar_dir}/plist/")
                break()
            endif()
        endforeach()
    endif()
    
    if(IDEVICE_HEADER_FOUND AND PLIST_HEADER_FOUND)
        # 设置包含目录 - 包括标准位置和 Cellar 位置
        set(IMOBILEDEVICE_INCLUDE_DIRS "${HOMEBREW_PREFIX}/include" ${IMOBILEDEVICE_CELLAR_DIRS})
        set(PLIST_INCLUDE_DIRS "${HOMEBREW_PREFIX}/include" ${PLIST_CELLAR_DIRS})
        set(USBMUXD_INCLUDE_DIRS "${HOMEBREW_PREFIX}/include" ${USBMUXD_CELLAR_DIRS})
        
        # 设置库路径和编译标志
        set(IMOBILEDEVICE_LIBRARIES "-L${HOMEBREW_PREFIX}/lib -limobiledevice-1.0")
        set(PLIST_LIBRARIES "-L${HOMEBREW_PREFIX}/lib -lplist-2.0")
        set(USBMUXD_LIBRARIES "-L${HOMEBREW_PREFIX}/lib -lusbmuxd-2.0")
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

# 定义源文件目录
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 定义源文件
set(SOURCES
    # Main
    ${SRC_DIR}/main.cpp
    
    # UI Layer
    ${SRC_DIR}/ui/mainwindow.cpp
    ${SRC_DIR}/ui/mainwindow.h
    ${SRC_DIR}/ui/mainwindow.ui
    
    # Core - Device Management
    ${SRC_DIR}/core/device/devicemanager.cpp
    ${SRC_DIR}/core/device/devicemanager.h
    ${SRC_DIR}/core/device/deviceinfo.cpp
    ${SRC_DIR}/core/device/deviceinfo.h
    
    # Platform Layer
    ${SRC_DIR}/platform/libimobiledevice_dynamic.cpp
    ${SRC_DIR}/platform/libimobiledevice_dynamic.h
    ${SRC_DIR}/platform/lockdown_dynamic.h
    ${SRC_DIR}/platform/plist_dynamic.h
)

qt_add_executable(phone-linkc
    WIN32 MACOSX_BUNDLE
    ${SOURCES}
)

# 设置头文件包含目录
target_include_directories(phone-linkc PRIVATE
    ${SRC_DIR}
    ${SRC_DIR}/ui
    ${SRC_DIR}/core
    ${SRC_DIR}/core/device
    ${SRC_DIR}/platform
)

# 翻译支持暂时禁用以避免构建问题
# TODO: 在未来版本中重新启用翻译支持
message(STATUS "Translation support disabled (to avoid build issues)")

target_link_libraries(phone-linkc
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        $<$<NOT:$<PLATFORM_ID:Windows>>:Threads::Threads>
)

# ============================================================================
# Windows/MSVC 平台配置（合并所有 Windows 相关设置）
# ============================================================================
if(WIN32)
    # Windows 通用定义
    target_compile_definitions(phone-linkc PRIVATE
        QT_THREAD_SUPPORT                    # 启用 Qt 线程支持
        _WIN32_WINNT=0x0601                  # Windows 7 及以上版本支持
    )
    
    # MSVC 特定配置
    if(MSVC)
        # Qt 6.10.1 要求的 MSVC 编译器选项
        target_compile_options(phone-linkc PRIVATE
            /Zc:__cplusplus      # 启用正确的 C++ 标准宏
            /permissive-         # 禁用非标准扩展，启用标准一致性
            /utf-8               # 使用 UTF-8 编码（包含源码和执行字符集）
            /W3                  # 警告级别
            /Zc:lambda           # 改进的 lambda 处理
        )
        
        # Qt MOC 与 MSVC 兼容性和中文支持
        target_compile_definitions(phone-linkc PRIVATE
            QT_DISABLE_DEPRECATED_UP_TO=0x060000  # 禁用过时的 API 警告
            _ENABLE_EXTENDED_ALIGNED_STORAGE      # 启用扩展对齐存储支持
            UNICODE                               # 启用 Unicode 支持
            _UNICODE                              # 启用 Unicode 支持
            WIN32_LEAN_AND_MEAN                   # 减少 Windows 头文件包含
        )
        
        # 设置 Windows 子系统（GUI应用）
        set_target_properties(phone-linkc PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
    endif()
    
    # -------------------------------------------------------------------------
    # libimobiledevice DLL 动态加载方案
    # -------------------------------------------------------------------------
    # 检查是否存在 thirdparty DLL 文件
    set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libimobiledevice")
    
    if(EXISTS "${THIRDPARTY_DIR}/imobiledevice.dll" AND EXISTS "${THIRDPARTY_DIR}/plist.dll")
        message(STATUS "Windows 环境 - 启用 libimobiledevice DLL 动态加载方案")
        message(STATUS "DLL 路径: ${THIRDPARTY_DIR}")
        message(STATUS "  - imobiledevice.dll: ${THIRDPARTY_DIR}/imobiledevice.dll")
        message(STATUS "  - plist.dll: ${THIRDPARTY_DIR}/plist.dll")
        
        # 启用 libimobiledevice 支持（通过动态加载）
        set(LIBIMOBILEDEVICE_AVAILABLE TRUE)
        set(USE_DYNAMIC_LOADING TRUE)
        
        # 添加头文件路径
        if(EXISTS "${THIRDPARTY_DIR}/include")
            target_include_directories(phone-linkc PRIVATE "${THIRDPARTY_DIR}/include")
            message(STATUS "  - 头文件目录: ${THIRDPARTY_DIR}/include")
        endif()
        
        # 定义宏以启用动态加载功能
        target_compile_definitions(phone-linkc PRIVATE 
            HAVE_LIBIMOBILEDEVICE
            LIBIMOBILEDEVICE_DYNAMIC_LOADING
            WIN32_DYNAMIC_LOADING
        )
        
        # 配置 DLL 部署 - 将 DLL 文件复制到输出目录
        add_custom_command(TARGET phone-linkc POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${THIRDPARTY_DIR}/"
            "$<TARGET_FILE_DIR:phone-linkc>/"
        )
        
        message(STATUS "libimobiledevice Windows DLL 动态加载方案已启用")
        
    else()
        message(STATUS "Windows 环境 - 未找到 libimobiledevice DLL 文件")
        message(STATUS "搜索路径: ${THIRDPARTY_DIR}")
        message(STATUS "应用程序将在没有 iOS 设备支持的情况下构建")
        message(STATUS "如需启用 iOS 设备支持，请确保 DLL 文件存在于 thirdparty/libimobiledevice/ 目录")
        
        # 禁用状态
        set(LIBIMOBILEDEVICE_AVAILABLE FALSE)
        set(USE_DYNAMIC_LOADING FALSE)
    endif()
endif()  # END if(WIN32)

# 如果找到 libimobiledevice，则启用相关功能（非Windows或macOS）
if(LIBIMOBILEDEVICE_AVAILABLE AND NOT WIN32)
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
        )
        
        find_library(PLIST_LIB
            NAMES plist-2.0 libplist-2.0
            PATHS ${HOMEBREW_PREFIX}/lib ${HOMEBREW_PREFIX}/Cellar/libplist/*/lib
        )
        
        find_library(USBMUXD_LIB
            NAMES usbmuxd-2.0 libusbmuxd-2.0
            PATHS ${HOMEBREW_PREFIX}/lib ${HOMEBREW_PREFIX}/Cellar/libusbmuxd/*/lib
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
elseif(NOT WIN32 AND NOT LIBIMOBILEDEVICE_AVAILABLE)
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
