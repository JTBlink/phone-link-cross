#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
iOS 设备管理器 - 依赖安装脚本
自动安装 libimobiledevice 和相关驱动
支持 Windows MobileDevice.framework
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
import urllib.parse
import zipfile
import json
import hashlib
from pathlib import Path
from typing import List, Dict, Optional, Tuple
import argparse
import time
import threading

# 颜色定义
class Colors:
    if sys.platform == "win32":
        RED = '\033[91m'
        GREEN = '\033[92m'
        YELLOW = '\033[93m'
        BLUE = '\033[94m'
        MAGENTA = '\033[95m'
        CYAN = '\033[96m'
        WHITE = '\033[97m'
        NC = '\033[0m'
    else:
        RED = '\033[0;31m'
        GREEN = '\033[0;32m'
        YELLOW = '\033[1;33m'
        BLUE = '\033[0;34m'
        MAGENTA = '\033[0;35m'
        CYAN = '\033[0;36m'
        WHITE = '\033[1;37m'
        NC = '\033[0m'

class ProgressBar:
    """简单的进度条显示"""
    def __init__(self, total=100, width=50):
        self.total = total
        self.width = width
        self.current = 0
        
    def update(self, value):
        self.current = value
        filled = int(self.width * value // self.total)
        bar = '█' * filled + '▒' * (self.width - filled)
        percent = 100 * value // self.total
        print(f'\r[{bar}] {percent}%', end='', flush=True)
    
    def finish(self):
        print()

class DependencyInstaller:
    """依赖安装器主类"""
    
    def __init__(self):
        self.system = platform.system()
        self.architecture = platform.machine()
        self.temp_dir = None
        self.install_root = None
        
        # 安装配置
        self.config = {
            'Windows': {
                'libimobiledevice': {
                    'url': 'https://github.com/libimobiledevice-win32/imobiledevice-net/releases/download/v1.3.17/libimobiledevice.1.2.1-r1122-win-x64.zip',
                    'filename': 'libimobiledevice.1.2.1-r1122-win-x64.zip',
                    'install_dir': 'thirdparty\\libimobiledevice',
                    'sha256': ''  # 可选的文件校验
                },
                'itunes_driver': {
                    'url': 'https://secure-appldnld.apple.com/itunes12/001-97787-20210421-F0E5A3C2-A2C9-11EB-A40B-A128318AD179/iTunes64Setup.exe',
                    'filename': 'iTunes64Setup.exe',
                    'install_silent': True
                }
            },
            'Darwin': {  # macOS
                'homebrew_packages': ['libimobiledevice', 'libplist', 'libusbmuxd', 'pkg-config']
            },
            'Linux': {
                'apt_packages': ['libimobiledevice-dev', 'libplist-dev', 'libusbmuxd-dev', 'pkg-config', 'build-essential']
            }
        }

    def print_header(self):
        """打印脚本头部信息"""
        print("=" * 60)
        print("iOS 设备管理器 - 依赖安装器")
        print("=" * 60)
        print(f"系统: {self.system} {self.architecture}")
        print(f"Python: {sys.version.split()[0]}")
        print()

    def print_status(self, status: str, message: str, details: str = None):
        """打印状态信息"""
        status_colors = {
            "success": Colors.GREEN + "✓" + Colors.NC,
            "error": Colors.RED + "✗" + Colors.NC,
            "warning": Colors.YELLOW + "⚠" + Colors.NC,
            "info": Colors.CYAN + "ℹ" + Colors.NC,
            "progress": Colors.BLUE + "►" + Colors.NC
        }
        
        symbol = status_colors.get(status, "•")
        print(f"{symbol} {message}")
        
        if details:
            print(f"    {details}")

    def check_admin_privileges(self) -> bool:
        """检查管理员权限（Windows）"""
        if self.system != "Windows":
            return True  # Unix 系统使用 sudo
        
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, "SOFTWARE") as key:
                return True
        except PermissionError:
            return False

    def setup_temp_directory(self):
        """设置临时目录"""
        self.temp_dir = tempfile.mkdtemp(prefix="phone_linkc_install_")
        self.print_status("info", f"临时目录: {self.temp_dir}")
    
    def get_downloads_directory(self) -> Path:
        """获取用户下载目录"""
        if self.system == "Windows":
            # Windows 下载目录
            downloads_path = Path.home() / "Downloads"
            if not downloads_path.exists():
                # 备选路径
                downloads_path = Path(os.environ.get('USERPROFILE', '')) / "Downloads"
        else:
            # macOS/Linux 下载目录
            downloads_path = Path.home() / "Downloads"
        
        # 如果下载目录不存在，创建它
        downloads_path.mkdir(exist_ok=True)
        return downloads_path

    def cleanup_temp_directory(self):
        """清理临时目录"""
        if self.temp_dir and os.path.exists(self.temp_dir):
            try:
                shutil.rmtree(self.temp_dir)
                self.print_status("info", "临时文件已清理")
            except Exception as e:
                self.print_status("warning", f"清理临时文件失败: {e}")

    def download_with_progress(self, url: str, filename: str) -> bool:
        """带进度条的文件下载"""
        try:
            self.print_status("progress", f"下载: {url}")
            
            def progress_hook(block_num, block_size, total_size):
                if total_size > 0:
                    downloaded = block_num * block_size
                    if downloaded <= total_size:
                        progress_bar.update(downloaded)
            
            # 获取文件大小
            req = urllib.request.Request(url)
            req.add_header('User-Agent', 'phone-linkc/1.0')
            
            with urllib.request.urlopen(req) as response:
                total_size = int(response.headers.get('content-length', 0))
                
                if total_size > 0:
                    progress_bar = ProgressBar(total_size, 40)
                    self.print_status("info", f"文件大小: {total_size / 1024 / 1024:.1f} MB")
                else:
                    progress_bar = ProgressBar(100, 40)
            
            urllib.request.urlretrieve(url, filename, progress_hook)
            progress_bar.finish()
            
            self.print_status("success", f"下载完成: {filename}")
            return True
            
        except Exception as e:
            self.print_status("error", f"下载失败: {e}")
            return False

    def verify_file_hash(self, filename: str, expected_hash: str) -> bool:
        """验证文件哈希"""
        if not expected_hash:
            return True
            
        try:
            hasher = hashlib.sha256()
            with open(filename, 'rb') as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    hasher.update(chunk)
            
            actual_hash = hasher.hexdigest()
            if actual_hash.lower() == expected_hash.lower():
                self.print_status("success", "文件哈希验证通过")
                return True
            else:
                self.print_status("error", f"文件哈希不匹配\n期望: {expected_hash}\n实际: {actual_hash}")
                return False
                
        except Exception as e:
            self.print_status("error", f"哈希验证失败: {e}")
            return False

    def install_libimobiledevice_windows(self) -> bool:
        """安装 libimobiledevice (Windows)"""
        self.print_status("info", "开始安装 libimobiledevice (Windows)")
        
        config = self.config['Windows']['libimobiledevice']
        # 从脚本目录找到项目根目录，然后构建目标路径
        script_dir = Path(__file__).parent  # scripts 目录
        project_root = script_dir.parent    # phone-linkc 目录
        install_dir = project_root / "thirdparty" / "libimobiledevice"
        
        # 下载到用户Downloads目录
        downloads_dir = self.get_downloads_directory()
        zip_path = downloads_dir / config['filename']
        
        # 检查是否已存在文件
        if zip_path.exists():
            self.print_status("info", f"使用已存在的文件: {zip_path}")
        else:
            # 下载
            if not self.download_with_progress(config['url'], str(zip_path)):
                return False
        
        # 验证文件哈希（如果提供）
        if config.get('sha256') and not self.verify_file_hash(str(zip_path), config['sha256']):
            return False
        
        # 删除旧安装
        if install_dir.exists():
            self.print_status("info", "删除旧版本...")
            try:
                shutil.rmtree(install_dir)
            except Exception as e:
                self.print_status("error", f"无法删除旧版本: {e}")
                return False
        
        # 解压
        self.print_status("info", f"解压到: {install_dir}")
        try:
            install_dir.mkdir(parents=True, exist_ok=True)
            
            with zipfile.ZipFile(zip_path, 'r') as zip_ref:
                zip_ref.extractall(install_dir)
                
        except Exception as e:
            self.print_status("error", f"解压失败: {e}")
            return False
        
        # 查找实际的可执行文件位置
        bin_dir = self._find_executable_dir(install_dir, 'idevice_id.exe')
        if not bin_dir:
            self.print_status("error", "未找到可执行文件目录")
            return False
            
        self.print_status("success", f"可执行文件位置: {bin_dir}")
        
        # 设置环境变量
        if not self._setup_windows_environment(install_dir, bin_dir):
            self.print_status("warning", "环境变量设置可能失败")
        
        # 验证安装
        if self._verify_libimobiledevice_installation(bin_dir):
            self.print_status("success", "libimobiledevice 安装成功")
            return True
        else:
            self.print_status("error", "libimobiledevice 安装验证失败")
            return False

    def _find_executable_dir(self, base_dir: Path, target_exe: str) -> Optional[Path]:
        """在目录中查找可执行文件"""
        for root, dirs, files in os.walk(base_dir):
            if target_exe in files:
                return Path(root)
        return None

    def _setup_windows_environment(self, install_dir: Path, bin_dir: Path) -> bool:
        """设置 Windows 环境变量"""
        try:
            # 设置 PATH
            current_path = self._get_registry_value(r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment", "PATH")
            
            if str(bin_dir) not in current_path:
                new_path = f"{current_path};{bin_dir}"
                self._set_registry_value(r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment", "PATH", new_path)
                self.print_status("success", "PATH 环境变量已更新")
            else:
                self.print_status("info", "PATH 已包含目标路径")
            
            # 设置 PKG_CONFIG_PATH
            pkgconfig_dir = install_dir / "lib" / "pkgconfig"
            if pkgconfig_dir.exists():
                self._set_registry_value(r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment", 
                                       "PKG_CONFIG_PATH", str(pkgconfig_dir))
                self.print_status("success", "PKG_CONFIG_PATH 已设置")
            
            return True
            
        except Exception as e:
            self.print_status("error", f"设置环境变量失败: {e}")
            return False

    def _get_registry_value(self, key_path: str, value_name: str) -> str:
        """获取注册表值"""
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, key_path) as key:
                value, _ = winreg.QueryValueEx(key, value_name)
                return str(value)
        except Exception:
            return ""

    def _set_registry_value(self, key_path: str, value_name: str, value: str):
        """设置注册表值"""
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, key_path, 0, winreg.KEY_SET_VALUE) as key:
            winreg.SetValueEx(key, value_name, 0, winreg.REG_EXPAND_SZ, value)

    def _verify_libimobiledevice_installation(self, bin_dir: Path) -> bool:
        """验证 libimobiledevice 安装"""
        exe_path = bin_dir / 'idevice_id.exe'
        if not exe_path.exists():
            return False
        
        try:
            # 尝试运行 idevice_id --help
            result = subprocess.run([str(exe_path), '--help'], 
                                  capture_output=True, text=True, timeout=10)
            return result.returncode == 0
        except Exception:
            return False

    def install_itunes_driver_windows(self) -> bool:
        """安装 iTunes 驱动 (MobileDevice.framework)"""
        self.print_status("info", "检查 Apple Mobile Device 驱动")
        
        # 检查是否已安装
        if self._check_apple_mobile_device_support():
            self.print_status("success", "Apple Mobile Device 支持已安装")
            return True
        
        self.print_status("warning", "Apple Mobile Device 支持未找到")
        self.print_status("info", "需要安装 iTunes 或 Apple Mobile Device Support")
        
        # 询问用户是否安装
        try:
            choice = input("是否下载并安装 iTunes? (y/n): ").lower().strip()
            if choice not in ['y', 'yes', '是']:
                self.print_status("info", "跳过 iTunes 安装")
                return False
        except KeyboardInterrupt:
            self.print_status("info", "用户取消安装")
            return False
        
        # 下载 iTunes 安装程序到 Downloads 目录
        config = self.config['Windows']['itunes_driver']
        downloads_dir = self.get_downloads_directory()
        installer_path = downloads_dir / config['filename']
        
        # 检查是否已存在文件
        if installer_path.exists():
            self.print_status("info", f"使用已存在的文件: {installer_path}")
        else:
            if not self.download_with_progress(config['url'], str(installer_path)):
                return False
        
        # 运行安装程序
        self.print_status("info", "启动 iTunes 安装程序...")
        self.print_status("warning", "请按照安装程序提示完成 iTunes 安装")
        
        try:
            if config.get('install_silent'):
                # 尝试静默安装
                result = subprocess.run([str(installer_path), '/S'], timeout=1800)  # 30分钟超时
            else:
                # 交互式安装
                result = subprocess.run([str(installer_path)], timeout=1800)
            
            if result.returncode == 0:
                self.print_status("success", "iTunes 安装完成")
                return self._check_apple_mobile_device_support()
            else:
                self.print_status("error", f"iTunes 安装失败 (返回码: {result.returncode})")
                return False
                
        except subprocess.TimeoutExpired:
            self.print_status("error", "iTunes 安装超时")
            return False
        except Exception as e:
            self.print_status("error", f"iTunes 安装异常: {e}")
            return False

    def _check_apple_mobile_device_support(self) -> bool:
        """检查 Apple Mobile Device 支持"""
        possible_paths = [
            "C:\\Program Files\\Common Files\\Apple\\Mobile Device Support",
            "C:\\Program Files (x86)\\Common Files\\Apple\\Mobile Device Support",
            "C:\\Program Files\\Common Files\\Apple\\Apple Application Support",
            "C:\\Program Files (x86)\\Common Files\\Apple\\Apple Application Support"
        ]
        
        for path in possible_paths:
            path_obj = Path(path)
            if path_obj.exists():
                # 检查关键文件
                key_files = ['MobileDevice.dll', 'iTunesMobileDevice.dll']
                for key_file in key_files:
                    if (path_obj / key_file).exists():
                        self.print_status("info", f"找到: {path_obj / key_file}")
                        return True
        
        # 检查服务
        try:
            result = subprocess.run(['sc', 'query', 'Apple Mobile Device Service'], 
                                  capture_output=True, text=True)
            if result.returncode == 0 and 'RUNNING' in result.stdout:
                self.print_status("info", "Apple Mobile Device Service 正在运行")
                return True
        except Exception:
            pass
        
        return False

    def install_dependencies_macos(self) -> bool:
        """安装依赖 (macOS)"""
        self.print_status("info", "安装 macOS 依赖...")
        
        # 检查 Homebrew
        if not shutil.which('brew'):
            self.print_status("error", "未找到 Homebrew")
            self.print_status("info", "请先安装 Homebrew: /bin/bash -c \"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"")
            return False
        
        packages = self.config['Darwin']['homebrew_packages']
        
        for package in packages:
            self.print_status("progress", f"安装 {package}...")
            try:
                result = subprocess.run(['brew', 'install', package], 
                                      capture_output=True, text=True, timeout=300)
                if result.returncode == 0:
                    self.print_status("success", f"{package} 安装成功")
                else:
                    self.print_status("warning", f"{package} 安装可能失败: {result.stderr}")
            except Exception as e:
                self.print_status("error", f"{package} 安装异常: {e}")
                return False
        
        return True

    def install_dependencies_linux(self) -> bool:
        """安装依赖 (Linux)"""
        self.print_status("info", "安装 Linux 依赖...")
        
        packages = self.config['Linux']['apt_packages']
        
        # 更新包列表
        self.print_status("progress", "更新包列表...")
        try:
            subprocess.run(['sudo', 'apt-get', 'update'], 
                         capture_output=True, text=True, timeout=300, check=True)
        except Exception as e:
            self.print_status("error", f"更新包列表失败: {e}")
            return False
        
        # 安装包
        for package in packages:
            self.print_status("progress", f"安装 {package}...")
            try:
                result = subprocess.run(['sudo', 'apt-get', 'install', '-y', package], 
                                      capture_output=True, text=True, timeout=300)
                if result.returncode == 0:
                    self.print_status("success", f"{package} 安装成功")
                else:
                    self.print_status("warning", f"{package} 安装可能失败: {result.stderr}")
            except Exception as e:
                self.print_status("error", f"{package} 安装异常: {e}")
                return False
        
        return True

    def run_post_install_tests(self) -> bool:
        """运行安装后测试"""
        self.print_status("info", "运行安装后测试...")
        
        if self.system == "Windows":
            # 测试 idevice_id
            try:
                result = subprocess.run(['idevice_id', '--help'], 
                                      capture_output=True, text=True, timeout=10)
                if result.returncode == 0:
                    self.print_status("success", "idevice_id 可用")
                else:
                    self.print_status("warning", "idevice_id 不可用，可能需要重启命令行")
            except FileNotFoundError:
                self.print_status("warning", "idevice_id 未在 PATH 中找到，可能需要重启命令行")
            except Exception as e:
                self.print_status("warning", f"测试 idevice_id 失败: {e}")
            
            # 测试设备连接
            try:
                result = subprocess.run(['idevice_id', '-l'], 
                                      capture_output=True, text=True, timeout=15)
                if result.returncode == 0:
                    devices = result.stdout.strip().split('\n') if result.stdout.strip() else []
                    if devices and devices[0]:
                        self.print_status("success", f"发现 {len(devices)} 台设备:")
                        for device in devices:
                            print(f"    - {device}")
                    else:
                        self.print_status("info", "未发现连接的设备（这是正常的）")
                else:
                    self.print_status("warning", "设备检测命令失败")
            except Exception as e:
                self.print_status("info", f"设备检测跳过: {e}")
        
        return True

    def install_all_dependencies(self) -> bool:
        """安装所有依赖"""
        success = True
        
        if self.system == "Windows":
            # Windows 安装流程
            if not self.install_libimobiledevice_windows():
                success = False
            
            if not self.install_itunes_driver_windows():
                self.print_status("warning", "iTunes/Apple Mobile Device 安装跳过或失败")
                self.print_status("info", "iOS 设备连接可能不可用，请手动安装 iTunes")
        
        elif self.system == "Darwin":
            # macOS 安装流程
            if not self.install_dependencies_macos():
                success = False
        
        elif self.system == "Linux":
            # Linux 安装流程
            if not self.install_dependencies_linux():
                success = False
        
        else:
            self.print_status("error", f"不支持的系统: {self.system}")
            return False
        
        # 运行测试
        self.run_post_install_tests()
        
        return success

    def print_installation_summary(self):
        """打印安装总结"""
        print()
        print("=" * 60)
        print("安装总结")
        print("=" * 60)
        
        if self.system == "Windows":
            print("已安装组件:")
            print("- libimobiledevice: iOS 设备通信库")
            print("- Apple Mobile Device Support (如果安装 iTunes)")
            print()
            
            print("安装位置:")
            print(f"- libimobiledevice: thirdparty/libimobiledevice/")
            
            downloads_dir = self.get_downloads_directory()
            print("下载文件位置:")
            print(f"- Downloads目录: {downloads_dir}")
            print("- libimobiledevice.1.2.1-r1122-win-x64.zip")
            print("- iTunes64Setup.exe (如果已下载)")
            print()
            
            print("重要提醒:")
            print("1. 重新打开命令提示符使环境变量生效")
            print("2. 连接 iOS 设备并信任此电脑")
            print("3. 运行测试: python scripts/check-deps.py")
            print("4. 构建项目: build.bat")
        
        elif self.system == "Darwin":
            print("已安装组件 (通过 Homebrew):")
            print("- libimobiledevice")
            print("- libplist") 
            print("- libusbmuxd")
            print("- pkg-config")
            print()
            print("接下来:")
            print("1. 连接 iOS 设备并信任此电脑")
            print("2. 运行测试: ./scripts/check-deps.sh")
            print("3. 构建项目: ./build.sh")
        
        elif self.system == "Linux":
            print("已安装组件 (通过 apt):")
            print("- libimobiledevice-dev")
            print("- libplist-dev")
            print("- libusbmuxd-dev") 
            print("- pkg-config")
            print("- build-essential")
            print()
            print("接下来:")
            print("1. 连接 iOS 设备")
            print("2. 运行测试: ./scripts/check-deps.sh")
            print("3. 构建项目: ./build.sh")


def main():
    """主函数"""
    # 设置控制台编码为 UTF-8
    if sys.platform == "win32":
        try:
            import ctypes
            kernel32 = ctypes.windll.kernel32
            # 设置控制台输出编码为 UTF-8
            kernel32.SetConsoleOutputCP(65001)
            kernel32.SetConsoleCP(65001)
            # 启用控制台颜色支持
            kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
        except Exception:
            pass  # 如果失败，继续执行但可能没有颜色或UTF-8支持
    
    parser = argparse.ArgumentParser(description='iOS 设备管理器依赖安装工具')
    parser.add_argument('--skip-itunes', action='store_true', help='跳过 iTunes 安装')
    parser.add_argument('--temp-dir', type=str, help='指定临时目录')
    parser.add_argument('--no-tests', action='store_true', help='跳过安装后测试')
    
    args = parser.parse_args()
    
    installer = DependencyInstaller()
    
    try:
        installer.print_header()
        
        # 权限检查
        if installer.system == "Windows" and not installer.check_admin_privileges():
            installer.print_status("error", "需要管理员权限")
            installer.print_status("info", "请以管理员身份运行此脚本")
            return 1
        
        # 设置临时目录
        if args.temp_dir:
            installer.temp_dir = args.temp_dir
            os.makedirs(installer.temp_dir, exist_ok=True)
        else:
            installer.setup_temp_directory()
        
        # 安装依赖
        installer.print_status("info", "开始安装依赖...")
        success = installer.install_all_dependencies()
        
        if success:
            installer.print_status("success", "所有依赖安装完成")
        else:
            installer.print_status("error", "某些依赖安装失败")
        
        # 打印总结
        installer.print_installation_summary()
        
        return 0 if success else 1
    
    except KeyboardInterrupt:
        installer.print_status("warning", "用户取消安装")
        return 1
    except Exception as e:
        installer.print_status("error", f"安装异常: {e}")
        return 1
    finally:
        installer.cleanup_temp_directory()


if __name__ == "__main__":
    sys.exit(main())
