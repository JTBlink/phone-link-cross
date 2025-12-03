#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
iOS 设备管理器 - Windows 依赖检测和安装脚本
智能检测 libimobiledevice 安装状态并提供解决方案
Python 跨平台实现
"""

import os
import sys
import subprocess
import platform
import shutil
import tempfile
import winreg
import urllib.request
import zipfile
from pathlib import Path
from typing import List, Dict, Optional, Tuple
import argparse

# 颜色定义（Windows 控制台支持）
class Colors:
    if sys.platform == "win32":
        # Windows 控制台颜色代码
        RED = '\033[91m'
        GREEN = '\033[92m'
        YELLOW = '\033[93m'
        BLUE = '\033[94m'
        MAGENTA = '\033[95m'
        CYAN = '\033[96m'
        WHITE = '\033[97m'
        NC = '\033[0m'  # No Color
    else:
        # Unix 控制台颜色代码
        RED = '\033[0;31m'
        GREEN = '\033[0;32m'
        YELLOW = '\033[1;33m'
        BLUE = '\033[0;34m'
        MAGENTA = '\033[0;35m'
        CYAN = '\033[0;36m'
        WHITE = '\033[1;37m'
        NC = '\033[0m'

class LibimobiledeviceChecker:
    def __init__(self):
        self.system = platform.system()
        self.architecture = platform.machine()
        self.missing_packages = []
        self.libimobiledevice_root = None
        self.pkg_config_available = False
        
        # 默认安装路径
        self.default_install_paths = {
            'Windows': [
                'C:\\libimobiledevice',
                'C:\\Program Files\\libimobiledevice',
                'C:\\Program Files (x86)\\libimobiledevice',
                os.path.expanduser('~\\libimobiledevice'),
            ]
        }
        
        # 需要检查的文件
        self.required_files = {
            'executables': ['idevice_id.exe', 'ideviceinfo.exe', 'ideviceinstaller.exe'],
            'headers': ['libimobiledevice/libimobiledevice.h', 'plist/plist.h'],
            'libraries': ['libimobiledevice-1.0.dll', 'libplist-2.0.dll'],
            'pkg_configs': ['libimobiledevice-1.0.pc', 'libplist-2.0.pc']
        }

    def print_header(self):
        """打印脚本头部信息"""
        print("=" * 50)
        print("iOS 设备管理器 - 依赖检测 (Python版)")
        print("=" * 50)
        print()

    def print_status(self, status: str, message: str, details: str = None):
        """打印状态信息"""
        if status == "success":
            print(f"{Colors.GREEN}✓{Colors.NC} {message}")
        elif status == "error":
            print(f"{Colors.RED}✗{Colors.NC} {message}")
        elif status == "warning":
            print(f"{Colors.YELLOW}⚠{Colors.NC} {message}")
        elif status == "info":
            print(f"{Colors.CYAN}ℹ{Colors.NC} {message}")
        
        if details:
            print(f"    {details}")

    def detect_system_info(self):
        """检测系统信息"""
        print()
        print("=== 系统信息检测 ===")
        print(f"操作系统: {self.system}")
        print(f"架构: {self.architecture}")
        print(f"Python版本: {sys.version}")
        
        if self.system == "Windows":
            try:
                # 检测 Windows 版本
                version = platform.version()
                print(f"Windows版本: {version}")
                self.print_status("success", "Windows 系统检测完成")
            except Exception as e:
                self.print_status("warning", f"无法获取详细 Windows 版本信息: {e}")
        else:
            self.print_status("error", f"此脚本专为 Windows 设计，当前系统: {self.system}")
            return False
        return True

    def check_administrator_privileges(self):
        """检查管理员权限"""
        print()
        print("=== 管理员权限检查 ===")
        
        try:
            # 尝试访问需要管理员权限的注册表位置
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, "SOFTWARE") as key:
                self.print_status("success", "具有足够的系统访问权限")
                return True
        except PermissionError:
            self.print_status("warning", "当前没有管理员权限")
            self.print_status("info", "某些安装操作可能需要管理员权限")
            return False
        except Exception as e:
            self.print_status("warning", f"权限检查失败: {e}")
            return False

    def find_libimobiledevice_installation(self) -> Optional[str]:
        """查找 libimobiledevice 安装路径"""
        print()
        print("=== libimobiledevice 安装路径检测 ===")
        
        # 检查环境变量
        env_paths = []
        if 'LIBIMOBILEDEVICE_ROOT' in os.environ:
            env_paths.append(os.environ['LIBIMOBILEDEVICE_ROOT'])
        
        # 检查 PATH 中的路径
        path_dirs = os.environ.get('PATH', '').split(os.pathsep)
        for path_dir in path_dirs:
            if 'libimobiledevice' in path_dir.lower():
                # 尝试找到父目录
                parent = Path(path_dir).parent
                if parent.exists():
                    env_paths.append(str(parent))
        
        # 合并所有候选路径
        all_paths = env_paths + self.default_install_paths['Windows']
        
        for path in all_paths:
            path_obj = Path(path)
            if path_obj.exists():
                # 检查是否包含必需的可执行文件
                bin_dir = path_obj / 'bin'
                if bin_dir.exists():
                    exe_files = list(bin_dir.glob('idevice*.exe'))
                    if exe_files:
                        self.print_status("success", f"找到 libimobiledevice 安装: {path}")
                        self.libimobiledevice_root = str(path_obj)
                        return str(path_obj)
        
        self.print_status("error", "未找到 libimobiledevice 安装")
        return None

    def check_required_files(self) -> Dict[str, List[str]]:
        """检查必需的文件"""
        print()
        print("=== 必需文件检查 ===")
        
        if not self.libimobiledevice_root:
            self.print_status("error", "无法检查文件 - 未找到安装路径")
            return {}
        
        root_path = Path(self.libimobiledevice_root)
        found_files = {category: [] for category in self.required_files.keys()}
        missing_files = {category: [] for category in self.required_files.keys()}
        
        # 检查可执行文件
        bin_dir = root_path / 'bin'
        print("检查可执行文件:")
        for exe in self.required_files['executables']:
            exe_path = bin_dir / exe
            if exe_path.exists():
                found_files['executables'].append(str(exe_path))
                self.print_status("success", f"{exe} 找到", str(exe_path))
            else:
                missing_files['executables'].append(exe)
                self.print_status("error", f"{exe} 未找到")
        
        # 检查头文件
        include_dir = root_path / 'include'
        print("\n检查头文件:")
        for header in self.required_files['headers']:
            header_path = include_dir / header
            if header_path.exists():
                found_files['headers'].append(str(header_path))
                self.print_status("success", f"{header} 找到", str(header_path))
            else:
                missing_files['headers'].append(header)
                self.print_status("error", f"{header} 未找到")
        
        # 检查库文件
        lib_dir = root_path / 'lib'
        print("\n检查库文件:")
        for lib in self.required_files['libraries']:
            lib_path = lib_dir / lib
            if lib_path.exists():
                found_files['libraries'].append(str(lib_path))
                self.print_status("success", f"{lib} 找到", str(lib_path))
            else:
                missing_files['libraries'].append(lib)
                self.print_status("error", f"{lib} 未找到")
        
        # 检查 pkg-config 文件
        pkgconfig_dir = lib_dir / 'pkgconfig'
        print("\n检查 pkg-config 文件:")
        for pc in self.required_files['pkg_configs']:
            pc_path = pkgconfig_dir / pc
            if pc_path.exists():
                found_files['pkg_configs'].append(str(pc_path))
                self.print_status("success", f"{pc} 找到", str(pc_path))
            else:
                missing_files['pkg_configs'].append(pc)
                self.print_status("error", f"{pc} 未找到")
        
        return {'found': found_files, 'missing': missing_files}

    def check_pkg_config(self) -> bool:
        """检查 pkg-config 工具"""
        print()
        print("=== pkg-config 检查 ===")
        
        # 检查 pkg-config 是否可用
        try:
            result = subprocess.run(['pkg-config', '--version'], 
                                  capture_output=True, text=True, check=True)
            version = result.stdout.strip()
            self.print_status("success", f"pkg-config 已安装: {version}")
            self.pkg_config_available = True
            
            # 检查 PKG_CONFIG_PATH
            pkg_config_path = os.environ.get('PKG_CONFIG_PATH', '')
            if pkg_config_path:
                self.print_status("info", f"PKG_CONFIG_PATH: {pkg_config_path}")
            else:
                self.print_status("warning", "PKG_CONFIG_PATH 未设置")
                if self.libimobiledevice_root:
                    suggested_path = os.path.join(self.libimobiledevice_root, 'lib', 'pkgconfig')
                    self.print_status("info", f"建议设置: {suggested_path}")
            
            return True
            
        except (subprocess.CalledProcessError, FileNotFoundError):
            self.print_status("error", "pkg-config 未找到")
            self.print_status("info", "pkg-config 不是必需的，但建议安装以获得更好的库检测")
            return False

    def test_pkg_config_queries(self):
        """测试 pkg-config 查询"""
        if not self.pkg_config_available:
            return
        
        print()
        print("=== pkg-config 查询测试 ===")
        
        modules = ['libimobiledevice-1.0', 'libplist-2.0', 'libusbmuxd-2.0']
        
        for module in modules:
            try:
                # 测试模块是否存在
                subprocess.run(['pkg-config', '--exists', module], 
                             check=True, capture_output=True)
                
                # 获取版本信息
                version_result = subprocess.run(['pkg-config', '--modversion', module],
                                              capture_output=True, text=True, check=True)
                version = version_result.stdout.strip()
                
                # 获取编译标志
                cflags_result = subprocess.run(['pkg-config', '--cflags', module],
                                             capture_output=True, text=True, check=True)
                cflags = cflags_result.stdout.strip()
                
                # 获取链接标志
                libs_result = subprocess.run(['pkg-config', '--libs', module],
                                           capture_output=True, text=True, check=True)
                libs = libs_result.stdout.strip()
                
                self.print_status("success", f"{module} 可查询")
                print(f"    版本: {version}")
                print(f"    CFLAGS: {cflags}")
                print(f"    LIBS: {libs}")
                
            except subprocess.CalledProcessError:
                self.print_status("error", f"{module} 无法查询")

    def test_device_connection(self):
        """测试设备连接功能"""
        print()
        print("=== 设备连接测试 ===")
        
        if not self.libimobiledevice_root:
            self.print_status("error", "无法测试 - 未找到 libimobiledevice 安装")
            return False
        
        idevice_id_path = Path(self.libimobiledevice_root) / 'bin' / 'idevice_id.exe'
        
        if not idevice_id_path.exists():
            self.print_status("error", "idevice_id.exe 不存在")
            return False
        
        try:
            # 尝试列出设备
            result = subprocess.run([str(idevice_id_path), '-l'], 
                                  capture_output=True, text=True, timeout=10)
            
            if result.returncode == 0:
                devices = result.stdout.strip().split('\n') if result.stdout.strip() else []
                if devices and devices[0]:
                    self.print_status("success", f"发现 {len(devices)} 台设备")
                    for i, device in enumerate(devices, 1):
                        print(f"    设备 {i}: {device}")
                else:
                    self.print_status("warning", "未发现连接的设备")
                    self.print_status("info", "请确保 iOS 设备已连接并信任此电脑")
                return True
            else:
                self.print_status("error", f"设备检测失败 (返回代码: {result.returncode})")
                if result.stderr:
                    print(f"    错误: {result.stderr.strip()}")
                return False
                
        except subprocess.TimeoutExpired:
            self.print_status("warning", "设备检测超时")
            return False
        except Exception as e:
            self.print_status("error", f"设备检测异常: {e}")
            return False

    def check_cmake_configuration(self):
        """检查 CMake 配置问题"""
        print()
        print("=== CMake 配置检查 ===")
        
        build_dir = Path('build')
        if not build_dir.exists():
            # 也检查父目录的 build
            parent_build = Path('..') / 'build'
            if parent_build.exists():
                build_dir = parent_build
            else:
                self.print_status("info", "未找到构建目录，需要先运行 CMake 配置")
                return
        
        cmake_cache = build_dir / 'CMakeCache.txt'
        if not cmake_cache.exists():
            self.print_status("warning", "未找到 CMakeCache.txt，可能需要重新配置")
            return
        
        self.print_status("success", f"找到 CMake 配置: {cmake_cache}")
        
        # 分析 CMakeCache.txt
        try:
            with open(cmake_cache, 'r', encoding='utf-8') as f:
                cache_content = f.read()
            
            # 检查 libimobiledevice 相关变量
            cmake_vars = {}
            for line in cache_content.split('\n'):
                if '=' in line and not line.startswith('#') and not line.startswith('//'):
                    key, value = line.split('=', 1)
                    cmake_vars[key] = value
            
            # 检查重要变量
            important_vars = [
                'LIBIMOBILEDEVICE_AVAILABLE',
                'HAVE_LIBIMOBILEDEVICE', 
                'IMOBILEDEVICE_FOUND',
                'PLIST_FOUND',
                'IMOBILEDEVICE_INCLUDE_DIRS',
                'IMOBILEDEVICE_LIBRARIES'
            ]
            
            print("CMake 变量检查:")
            for var in important_vars:
                if var in cmake_vars:
                    value = cmake_vars[var]
                    if value.lower() in ['true', '1', 'on']:
                        self.print_status("success", f"{var}: {value}")
                    elif value.lower() in ['false', '0', 'off']:
                        self.print_status("error", f"{var}: {value}")
                    else:
                        self.print_status("info", f"{var}: {value}")
                else:
                    self.print_status("warning", f"{var}: 未定义")
            
        except Exception as e:
            self.print_status("error", f"无法分析 CMakeCache.txt: {e}")

    def check_visual_studio_tools(self):
        """检查 Visual Studio 工具"""
        print()
        print("=== Visual Studio 工具检查 ===")
        
        # 检查 MSVC 编译器
        try:
            result = subprocess.run(['cl.exe'], capture_output=True, text=True)
            if 'Microsoft' in result.stderr:
                self.print_status("success", "MSVC 编译器 (cl.exe) 可用")
                # 提取版本信息
                for line in result.stderr.split('\n'):
                    if 'Microsoft' in line:
                        print(f"    {line.strip()}")
                        break
            else:
                self.print_status("error", "MSVC 编译器不可用")
        except FileNotFoundError:
            self.print_status("error", "MSVC 编译器 (cl.exe) 未找到")
            self.print_status("info", "请安装 Visual Studio 2019/2022 with C++ 支持")
        
        # 检查 CMake
        try:
            result = subprocess.run(['cmake', '--version'], 
                                  capture_output=True, text=True, check=True)
            version_line = result.stdout.split('\n')[0]
            self.print_status("success", f"CMake 可用: {version_line}")
        except (subprocess.CalledProcessError, FileNotFoundError):
            self.print_status("error", "CMake 未找到")
            self.print_status("info", "请从 https://cmake.org/download/ 安装 CMake")

    def provide_installation_advice(self):
        """提供安装建议"""
        print()
        print("=" * 50)
        print("安装建议")
        print("=" * 50)
        
        if not self.libimobiledevice_root:
            print(f"{Colors.YELLOW}libimobiledevice 未安装{Colors.NC}")
            print()
            print(f"{Colors.GREEN}推荐解决方案:{Colors.NC}")
            print("1. 运行新的依赖安装脚本:")
            print("   build.bat install-deps")
            print("   或者: python scripts\\install-deps.py")
            print()
            print("2. 使用 vcpkg 安装:")
            print("   vcpkg install libimobiledevice:x64-windows")
            print("   vcpkg install libplist:x64-windows")
        else:
            # 检查文件完整性
            files_result = self.check_required_files()
            if files_result and files_result.get('missing'):
                missing = files_result['missing']
                if any(missing.values()):
                    print(f"{Colors.YELLOW}libimobiledevice 安装不完整{Colors.NC}")
                    print()
                    for category, files in missing.items():
                        if files:
                            print(f"缺失的 {category}:")
                            for file in files:
                                print(f"  - {file}")
                    print()
                    print(f"{Colors.GREEN}建议解决方案:{Colors.NC}")
                    print("1. 重新安装 libimobiledevice")
                    print("2. 检查安装包是否完整")
                    print("3. 确认安装路径正确")
                else:
                    print(f"{Colors.GREEN}✓ libimobiledevice 安装完整{Colors.NC}")

    def auto_install_libimobiledevice(self) -> bool:
        """自动安装 libimobiledevice"""
        print()
        print("=== 自动安装 libimobiledevice ===")
        
        # 检查安装脚本是否存在
        possible_paths = [
            Path('scripts') / 'install-libimobiledevice-windows.bat',
            Path('.') / 'scripts' / 'install-libimobiledevice-windows.bat',
            Path('..') / 'scripts' / 'install-libimobiledevice-windows.bat',
            Path(__file__).parent / 'install-libimobiledevice-windows.bat'
        ]
        
        script_path = None
        for path in possible_paths:
            if path.exists():
                script_path = path
                break
        
        if not script_path:
            self.print_status("error", "未找到安装脚本")
            self.print_status("info", f"尝试查找的路径:")
            for path in possible_paths:
                self.print_status("info", f"  - {path}")
            return False
        
        try:
            self.print_status("info", f"运行安装脚本: {script_path}")
            # 使用绝对路径并在脚本所在目录运行
            abs_script_path = script_path.resolve()
            result = subprocess.run([str(abs_script_path)], 
                                  cwd=abs_script_path.parent, 
                                  shell=True,  # 在 Windows 上使用 shell
                                  timeout=600)  # 10分钟超时
            
            if result.returncode == 0:
                self.print_status("success", "自动安装完成")
                # 重新检测安装
                self.find_libimobiledevice_installation()
                return True
            else:
                self.print_status("error", f"安装脚本执行失败 (返回代码: {result.returncode})")
                return False
                
        except subprocess.TimeoutExpired:
            self.print_status("error", "安装超时")
            return False
        except Exception as e:
            self.print_status("error", f"安装异常: {e}")
            return False

    def generate_summary_report(self):
        """生成总结报告"""
        print()
        print("=" * 50)
        print("检查总结")
        print("=" * 50)
        
        # 系统信息
        print(f"系统: {self.system} {self.architecture}")
        
        # libimobiledevice 状态
        if self.libimobiledevice_root:
            print(f"{Colors.GREEN}✓{Colors.NC} libimobiledevice: 已安装 ({self.libimobiledevice_root})")
        else:
            print(f"{Colors.RED}✗{Colors.NC} libimobiledevice: 未安装")
        
        # pkg-config 状态
        if self.pkg_config_available:
            print(f"{Colors.GREEN}✓{Colors.NC} pkg-config: 可用")
        else:
            print(f"{Colors.YELLOW}⚠{Colors.NC} pkg-config: 不可用")
        
        # 后续步骤建议
        print()
        print("下一步操作:")
        if not self.libimobiledevice_root:
            print("1. 安装 libimobiledevice (运行安装脚本或手动安装)")
            print("2. 重新运行此检查脚本验证")
            print("3. 构建项目")
        else:
            print("1. 运行构建脚本: build.bat")
            print("2. 连接 iOS 设备测试功能")
            print("3. 如有问题，重新运行此脚本诊断")

    def run_full_check(self):
        """运行完整检查"""
        self.print_header()
        
        # 系统检测
        if not self.detect_system_info():
            return False
        
        # 权限检查
        self.check_administrator_privileges()
        
        # 查找 libimobiledevice
        self.find_libimobiledevice_installation()
        
        # 文件检查
        self.check_required_files()
        
        # pkg-config 检查
        self.check_pkg_config()
        self.test_pkg_config_queries()
        
        # 设备连接测试
        self.test_device_connection()
        
        # Visual Studio 工具检查
        self.check_visual_studio_tools()
        
        # CMake 配置检查
        self.check_cmake_configuration()
        
        # 提供建议
        self.provide_installation_advice()
        
        # 生成总结
        self.generate_summary_report()
        
        return True

    def interactive_install(self):
        """交互式安装"""
        if self.libimobiledevice_root:
            print(f"{Colors.GREEN}libimobiledevice 已安装，无需重新安装{Colors.NC}")
            return
        
        print()
        try:
            choice = input("是否自动安装 libimobiledevice? (y/n): ").lower().strip()
            if choice in ['y', 'yes', '是']:
                return self.auto_install_libimobiledevice()
            else:
                print("请手动安装 libimobiledevice 后重新运行检查")
                return False
        except KeyboardInterrupt:
            print("\n操作已取消")
            return False


def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='libimobiledevice 依赖检测工具')
    parser.add_argument('--install', action='store_true', help='自动安装 libimobiledevice')
    parser.add_argument('--no-interactive', action='store_true', help='非交互模式')
    parser.add_argument('--quiet', action='store_true', help='静默模式，只显示错误')
    
    args = parser.parse_args()
    
    # 启用 Windows 控制台颜色支持
    if sys.platform == "win32":
        try:
            import ctypes
            kernel32 = ctypes.windll.kernel32
            kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
        except:
            pass  # 如果失败，继续执行但可能没有颜色
    
    checker = LibimobiledeviceChecker()
    
    try:
        if args.install:
            # 直接安装模式
            checker.detect_system_info()
            if not checker.find_libimobiledevice_installation():
                checker.auto_install_libimobiledevice()
        else:
            # 完整检查模式
            checker.run_full_check()
            
            # 交互式安装（如果需要且不在静默模式）
            if not args.no_interactive and not args.quiet and not checker.libimobiledevice_root:
                checker.interactive_install()
    
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}操作已取消{Colors.NC}")
        return 1
    except Exception as e:
        print(f"{Colors.RED}错误: {e}{Colors.NC}")
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
