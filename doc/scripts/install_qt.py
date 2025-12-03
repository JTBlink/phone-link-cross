#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Qt Cross-platform Installation Script
Supports Windows, macOS, and Linux
"""

import os
import sys
import platform
import subprocess
import shutil
import urllib.request
import tempfile
from pathlib import Path

class QtInstaller:
    def __init__(self):
        self.system = platform.system().lower()
        self.arch = platform.machine().lower()
        # Use user's Downloads directory instead of temp directory
        self.download_dir = self.get_user_downloads_dir()
        os.makedirs(self.download_dir, exist_ok=True)
        
        # Mirror configuration with fallbacks - updated with correct filenames
        self.mirrors = {
            'windows': {
                'primary': {
                    'url': 'https://mirrors.aliyun.com/qt/official_releases/online_installers',
                    'installer': 'qt-online-installer-windows-x64-online.exe'
                },
                'fallback': [
                    {
                        'url': 'https://download.qt.io/official_releases/online_installers',
                        'installer': 'qt-online-installer-windows-x64-online.exe'
                    },
                    {
                        'url': 'https://mirrors.tuna.tsinghua.edu.cn/qt/official_releases/online_installers',
                        'installer': 'qt-online-installer-windows-x64-online.exe'
                    },
                    {
                        'url': 'https://mirrors.ustc.edu.cn/qtproject/official_releases/online_installers',
                        'installer': 'qt-online-installer-windows-x64-online.exe'
                    }
                ]
            },
            'linux': {
                'primary': {
                    'url': 'https://mirrors.aliyun.com/qt/official_releases/online_installers',
                    'installer': 'qt-online-installer-linux-x64-online.run'
                },
                'fallback': [
                    {
                        'url': 'https://download.qt.io/official_releases/online_installers',
                        'installer': 'qt-online-installer-linux-x64-online.run'
                    },
                    {
                        'url': 'https://mirrors.tuna.tsinghua.edu.cn/qt/official_releases/online_installers',
                        'installer': 'qt-online-installer-linux-x64-online.run'
                    },
                    {
                        'url': 'https://mirrors.ustc.edu.cn/qtproject/official_releases/online_installers',
                        'installer': 'qt-online-installer-linux-x64-online.run'
                    }
                ]
            },
            'darwin': {
                'primary': {
                    'url': 'https://mirrors.aliyun.com/qt/official_releases/online_installers',
                    'installer': 'qt-online-installer-mac-x64-online.dmg'
                },
                'fallback': [
                    {
                        'url': 'https://download.qt.io/official_releases/online_installers',
                        'installer': 'qt-online-installer-mac-x64-online.dmg'
                    },
                    {
                        'url': 'https://mirrors.tuna.tsinghua.edu.cn/qt/official_releases/online_installers',
                        'installer': 'qt-online-installer-mac-x64-online.dmg'
                    },
                    {
                        'url': 'https://mirrors.ustc.edu.cn/qtproject/official_releases/online_installers',
                        'installer': 'qt-online-installer-mac-x64-online.dmg'
                    }
                ]
            }
        }
        
    def get_user_downloads_dir(self):
        """Get user's Downloads directory path"""
        if self.system == 'windows':
            # Windows: Use USERPROFILE/Downloads
            downloads_path = os.path.join(os.path.expanduser('~'), 'Downloads')
        elif self.system == 'darwin':
            # macOS: Use ~/Downloads
            downloads_path = os.path.join(os.path.expanduser('~'), 'Downloads')
        else:
            # Linux: Try XDG_DOWNLOAD_DIR first, then ~/Downloads
            downloads_path = os.environ.get('XDG_DOWNLOAD_DIR')
            if not downloads_path:
                downloads_path = os.path.join(os.path.expanduser('~'), 'Downloads')
        
        return downloads_path
        
    def print_info(self, message):
        print(f"[INFO] {message}")
        
    def print_warning(self, message):
        print(f"[WARNING] {message}")
        
    def print_error(self, message):
        print(f"[ERROR] {message}")
        
    def print_success(self, message):
        print(f"[SUCCESS] {message}")
        
    def check_admin_rights(self):
        """Check if running with admin/root privileges"""
        if self.system == 'windows':
            try:
                import ctypes
                return ctypes.windll.shell32.IsUserAnAdmin() != 0
            except:
                return False
        else:
            return os.geteuid() == 0
            
    def detect_architecture(self):
        """Detect system architecture"""
        if self.system == 'windows':
            if self.arch in ['amd64', 'x86_64']:
                return 'x64'
            elif self.arch in ['x86', 'i386', 'i686']:
                return 'x86'
        elif self.system in ['linux', 'darwin']:
            if self.arch in ['x86_64', 'amd64']:
                return 'x64'
            elif self.arch in ['i386', 'i686', 'x86']:
                return 'x86'
        return 'x64'  # default
        
    def check_disk_space(self, required_gb=5):
        """Check available disk space"""
        try:
            if self.system == 'windows':
                import psutil
                disk_usage = psutil.disk_usage('C:\\')
            else:
                import psutil
                disk_usage = psutil.disk_usage('/')
                
            available_gb = disk_usage.free / (1024**3)
            return available_gb >= required_gb, available_gb
        except ImportError:
            self.print_warning("psutil not available, skipping disk space check")
            return True, 0
            
    def download_file(self, url, filepath):
        """Download file with progress indication"""
        try:
            self.print_info(f"Downloading from: {url}")
            self.print_info(f"Saving to: {filepath}")
            
            def progress_hook(block_num, block_size, total_size):
                if total_size > 0:
                    percent = min(100, (block_num * block_size * 100) // total_size)
                    print(f"\r[INFO] Download progress: {percent}%", end='', flush=True)
                    
            urllib.request.urlretrieve(url, filepath, progress_hook)
            print()  # new line after progress
            return True
            
        except Exception as e:
            self.print_error(f"Download failed: {str(e)}")
            return False
            
    def get_installer_info(self):
        """Get installer information for current platform with fallback support"""
        if self.system not in self.mirrors:
            raise ValueError(f"Unsupported platform: {self.system}")
            
        arch = self.detect_architecture()
        mirror_config = self.mirrors[self.system]
        
        # Create list of mirrors to try (primary + fallbacks)
        mirrors_to_try = [mirror_config['primary']] + mirror_config['fallback']
        
        # Adjust installer name based on architecture
        for mirror_info in mirrors_to_try:
            installer_name = mirror_info['installer']
            if self.system == 'windows' and arch == 'x86':
                installer_name = installer_name.replace('x64', 'x86')
            mirror_info['installer'] = installer_name
            
        return mirrors_to_try
        
    def run_installer(self, installer_path):
        """Run the Qt installer"""
        self.print_info("Starting Qt installer...")
        
        if self.system == 'windows':
            # Windows: run .exe installer
            subprocess.run([installer_path, '--mirror', 'https://mirrors.aliyun.com/qt'], check=True)
        elif self.system == 'linux':
            # Linux: make .run file executable and run
            os.chmod(installer_path, 0o755)
            subprocess.run([installer_path, '--mirror', 'https://mirrors.aliyun.com/qt'], check=True)
        elif self.system == 'darwin':
            # macOS: open .dmg
            subprocess.run(['open', installer_path], check=True)
            
    def find_qt_installation(self):
        """Find Qt installation path"""
        qt_paths = []
        
        if self.system == 'windows':
            qt_paths = ['C:\\Qt', 'D:\\Qt', 'E:\\Qt']
        elif self.system == 'linux':
            qt_paths = ['~/Qt', '/opt/Qt', '/usr/local/Qt']
        elif self.system == 'darwin':
            qt_paths = ['~/Qt', '/Applications/Qt']
            
        for qt_path in qt_paths:
            qt_path = os.path.expanduser(qt_path)
            if os.path.exists(qt_path):
                for version_dir in os.listdir(qt_path):
                    bin_path = os.path.join(qt_path, version_dir, 'bin')
                    if os.path.exists(os.path.join(bin_path, 'qmake')):
                        return bin_path
                        
        return None
        
    def setup_environment(self, qt_path):
        """Setup environment variables"""
        self.print_info(f"Adding Qt path to environment: {qt_path}")
        
        if self.system == 'windows':
            try:
                subprocess.run(['setx', 'PATH', f'%PATH%;{qt_path}', '/M'], check=True)
                self.print_success("Environment variables configured successfully")
            except subprocess.CalledProcessError:
                self.print_warning("Admin rights required to configure system environment variables")
                self.print_info(f"Please manually add to PATH: {qt_path}")
        else:
            # For Unix-like systems, suggest adding to shell profile
            self.print_info("Please add the following to your shell profile (~/.bashrc, ~/.zshrc, etc.):")
            self.print_info(f"export PATH=\"$PATH:{qt_path}\"")
            
    def cleanup(self):
        """Clean up download files"""
        try:
            shutil.rmtree(self.download_dir)
            self.print_info("Download files cleaned up")
        except Exception as e:
            self.print_warning(f"Failed to cleanup download files: {str(e)}")
            
    def install(self):
        """Main installation process"""
        print("=" * 50)
        print("Qt Cross-platform Installation Script")
        print(f"Platform: {self.system} ({self.detect_architecture()})")
        print("=" * 50)
        
        # Check admin rights
        if not self.check_admin_rights():
            self.print_warning("Running without admin/root privileges")
            self.print_warning("Some features may require elevated privileges")
            
        # Check disk space
        enough_space, available_gb = self.check_disk_space()
        if not enough_space:
            self.print_warning(f"Less than 5GB available, current: {available_gb:.1f}GB")
            response = input("Continue anyway? (y/N): ")
            if response.lower() != 'y':
                self.print_info("Installation cancelled")
                return False
        else:
            self.print_info(f"Available disk space: {available_gb:.1f}GB")
            
        try:
            # Get installer information with fallback mirrors
            mirrors_to_try = self.get_installer_info()
            installer_name = mirrors_to_try[0]['installer']
            installer_path = os.path.join(self.download_dir, installer_name)
            
            # Check if installer already exists
            if os.path.exists(installer_path):
                self.print_info("Existing installer found")
                use_existing = input("Use existing installer? (Y/N): ")
                if use_existing.lower() != 'y':
                    os.remove(installer_path)
                else:
                    self.print_success("Using existing installer")
                    
            # Download installer if needed - try mirrors one by one
            if not os.path.exists(installer_path):
                downloaded = False
                for i, mirror_info in enumerate(mirrors_to_try):
                    mirror_name = "Aliyun" if i == 0 else f"Fallback {i}"
                    self.print_info(f"Trying {mirror_name} mirror...")
                    
                    download_url = f"{mirror_info['url']}/{mirror_info['installer']}"
                    if self.download_file(download_url, installer_path):
                        downloaded = True
                        self.print_success(f"Download completed from {mirror_name} mirror!")
                        break
                    else:
                        self.print_warning(f"{mirror_name} mirror failed, trying next...")
                        # Clean up partial download
                        if os.path.exists(installer_path):
                            os.remove(installer_path)
                
                if not downloaded:
                    self.print_error("All mirrors failed")
                    return False
            
            # Installation tips
            print("\n" + "=" * 50)
            print("Installation Tips:")
            print("1. Installer will use Alibaba Cloud mirror for faster download")
            print("2. Please login your Qt account in the installer")
            print("3. Recommended components:")
            print("   - Qt 6.x (latest version)")
            print("   - Qt Creator")
            print("   - CMake")
            if self.system == 'windows':
                print("   - MSVC 2019/2022 compiler")
            else:
                print("   - GCC/Clang compiler")
            print("4. Recommended installation path:")
            if self.system == 'windows':
                print("   D:\\Qt")
            else:
                print("   ~/Qt")
            print("=" * 50 + "\n")
            
            # Ask to continue
            response = input("Press Enter to continue installation, or N to cancel: ")
            if response.lower() == 'n':
                self.print_info("Installation cancelled")
                return False
                
            # Run installer
            self.run_installer(installer_path)
            
            # Post-installation configuration
            qt_path = self.find_qt_installation()
            if qt_path:
                setup_env = input("Configure environment variables? (Y/N): ")
                if setup_env.lower() == 'y':
                    self.setup_environment(qt_path)
            else:
                self.print_warning("Qt installation not found")
                self.print_info("Please manually configure environment variables")
                
            # Cleanup
            cleanup = input("Delete temporary installation files? (Y/N): ")
            if cleanup.lower() == 'y':
                self.cleanup()
            else:
                self.print_info(f"Download files kept at: {self.download_dir}")
                
            print("\n" + "=" * 50)
            print("Qt Installation Completed!")
            print("\nNext Steps:")
            print("1. Restart terminal/command prompt to apply environment variables")
            print("2. Run 'qmake --version' to verify installation")
            print("3. Start Qt Creator to begin development")
            print("=" * 50)
            
            return True
            
        except KeyboardInterrupt:
            self.print_info("Installation cancelled by user")
            return False
        except Exception as e:
            self.print_error(f"Installation failed: {str(e)}")
            return False
        finally:
            # Always cleanup download directory if needed
            if hasattr(self, 'download_dir') and os.path.exists(self.download_dir):
                pass  # Download directory is kept by default, cleanup only on user request

def main():
    """Main entry point"""
    try:
        installer = QtInstaller()
        success = installer.install()
        sys.exit(0 if success else 1)
    except Exception as e:
        print(f"[ERROR] {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
