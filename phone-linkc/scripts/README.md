# phone-linkc Scripts Directory

这个目录包含了用于 phone-linkc iOS 设备管理器项目的各种脚本和工具。

## 文件说明

### 依赖安装脚本 (新增)

#### `install-deps.py`
- **功能**: Python 实现的完整依赖安装器
- **支持平台**: Windows, macOS, Linux
- **要求**: Python 3.7+，Windows 需要管理员权限
- **用法**:
  ```bash
  # 完整安装
  python install-deps.py
  
  # 跳过 iTunes 安装
  python install-deps.py --skip-itunes
  
  # 跳过安装后测试
  python install-deps.py --no-tests
  
  # 指定临时目录
  python install-deps.py --temp-dir /tmp/myinstall
  ```
- **功能特性**:
  - 自动下载和安装 libimobiledevice
  - Windows: 支持 MobileDevice.framework (通过 iTunes)
  - macOS: 通过 Homebrew 安装依赖
  - Linux: 通过 apt 安装依赖
  - 带进度条的文件下载
  - 文件哈希验证 (可选)
  - 自动设置环境变量
  - 安装后验证和测试
  - 详细的错误诊断和建议

#### `install-deps.bat`
- **功能**: Windows 批处理入口脚本
- **要求**: 管理员权限, Python 3.7+
- **用法**:
  ```cmd
  # 以管理员身份运行
  .\install-deps.bat
  
  # 带参数运行
  .\install-deps.bat --skip-itunes --no-tests
  
  # 显示帮助
  .\install-deps.bat --help
  ```
- **功能特性**:
  - 自动检查管理员权限
  - Python 环境验证
  - 命令行参数解析和传递
  - 详细的错误处理和用户指导

#### `install-deps.sh`
- **功能**: Unix/Linux shell 入口脚本  
- **要求**: Python 3.7+，Linux 可能需要 sudo
- **用法**:
  ```bash
  # 标准安装
  ./install-deps.sh
  
  # 带参数运行
  ./install-deps.sh --skip-itunes --no-tests
  
  # 显示帮助
  ./install-deps.sh --help
  
  # Linux 如需要权限
  sudo ./install-deps.sh
  ```
- **功能特性**:
  - 自动检测操作系统 (macOS/Linux)
  - Python 环境验证和版本检查
  - 权限检查和提示
  - 信号处理 (Ctrl+C 中断)
  - 彩色输出和用户友好提示

### 依赖检测脚本

#### `check-deps.py`
- **功能**: Python 实现的依赖检测和诊断脚本
- **支持平台**: Windows (主要), 可扩展到其他平台
- **用法**:
  ```cmd
  # 完整依赖检查
  python check-deps.py
  
  # 自动安装 libimobiledevice
  python check-deps.py --install
  
  # 非交互模式
  python check-deps.py --no-interactive
  
  # 静默模式
  python check-deps.py --quiet
  ```
- **功能特性**:
  - 智能检测系统环境和架构
  - 检查管理员权限
  - 查找 libimobiledevice 安装路径
  - 验证必需文件（可执行文件、库、头文件）
  - 测试 pkg-config 配置
  - 测试设备连接功能
  - 检查 CMake 配置问题
  - 检查 Visual Studio 工具链
  - 提供详细的安装建议
  - 支持自动安装和交互式安装

### Windows 特定脚本

#### `install-libimobiledevice-windows.bat`
- **功能**: 专门的 Windows libimobiledevice 安装器
- **要求**: 管理员权限
- **用法**: 
  ```cmd
  # 以管理员身份运行
  .\install-libimobiledevice-windows.bat
  ```
- **功能特性**:
  - 自动下载最新的 libimobiledevice Windows 包
  - 解压到 `C:\libimobiledevice`
  - 自动设置环境变量 PATH 和 PKG_CONFIG_PATH
  - 验证安装完整性
  - 测试设备连接功能

#### `build-impl.bat`
- **功能**: 构建脚本的具体实现逻辑
- **说明**: 由主构建脚本调用，包含详细的构建步骤

### Unix/Linux 特定脚本

#### `check-deps.sh`
- **功能**: Bash 实现的依赖检测脚本
- **支持平台**: macOS, Linux
- **用法**:
  ```bash
  # 完整依赖检查
  ./check-deps.sh
  
  # 自动安装缺失的包
  ./check-deps.sh  # 然后选择 'y' 进行自动安装
  ```
- **功能特性**:
  - 检测 macOS 架构 (Intel vs Apple Silicon)
  - 检查 Homebrew 状态
  - 验证 pkg-config 和相关包
  - 检查 libimobiledevice 文件系统状态
  - 进行编译测试
  - 诊断 CMake 配置问题
  - 提供自动安装选项

#### `build-impl.sh`
- **功能**: Unix/Linux 系统的构建实现
- **说明**: 包含 Unix 系统特定的构建逻辑

## 使用工作流程

### 推荐流程 (使用新的安装脚本)

#### Windows 系统

1. **一键安装所有依赖**:
   ```cmd
   # 以管理员身份运行 (推荐方式)
   scripts\install-deps.bat
   
   # 跳过 iTunes 安装
   scripts\install-deps.bat --skip-itunes
   
   # 或直接运行 Python 脚本
   python scripts\install-deps.py
   ```

2. **验证安装**:
   ```cmd
   python scripts\check-deps.py
   ```

3. **构建项目**:
   ```cmd
   build.bat
   ```

#### macOS/Linux 系统

1. **一键安装所有依赖**:
   ```bash
   # 运行安装脚本
   ./scripts/install-deps.sh
   
   # 跳过可选组件
   ./scripts/install-deps.sh --skip-itunes --no-tests
   
   # 或直接运行 Python 脚本
   python3 scripts/install-deps.py
   ```

2. **验证安装**:
   ```bash
   ./scripts/check-deps.sh
   ```

3. **构建项目**:
   ```bash
   ./build.sh
   ```

### 传统流程 (手动安装)

#### Windows 系统

1. **检查依赖**:
   ```cmd
   # 使用构建脚本检查
   build.bat check-deps
   
   # 或使用 Python 脚本详细检查
   python scripts/check-deps.py
   ```

2. **安装 libimobiledevice**:
   ```cmd
   # 方法1: 使用构建脚本
   build.bat install-libimobiledevice
   
   # 方法2: 使用 Python 脚本
   python scripts/check-deps.py --install
   
   # 方法3: 直接运行安装脚本
   scripts/install-libimobiledevice-windows.bat
   ```

3. **构建项目**:
   ```cmd
   build.bat
   ```

#### macOS/Linux 系统

1. **检查依赖**:
   ```bash
   ./scripts/check-deps.sh
   ```

2. **安装缺失依赖**:
   ```bash
   # macOS (使用 Homebrew)
   brew install libimobiledevice libplist libusbmuxd
   
   # Ubuntu/Debian
   sudo apt-get install libimobiledevice-dev libplist-dev libusbmuxd-dev
   ```

3. **构建项目**:
   ```bash
   ./build.sh
   ```

## 故障排除

### 常见问题

1. **libimobiledevice 未找到**
   - **推荐**: 运行 `scripts/install-deps.bat` (Windows) 或 `./scripts/install-deps.sh` (Unix)
   - 传统方法:
     - Windows: 运行 `build.bat install-libimobiledevice`
     - macOS: 运行 `brew install libimobiledevice`
     - Linux: 运行 `sudo apt-get install libimobiledevice-dev`

2. **环境变量问题**
   - 重新打开命令提示符/终端 (新安装脚本会自动设置)
   - 检查 PATH 是否包含 libimobiledevice 路径
   - Windows: 检查 PKG_CONFIG_PATH 设置

3. **权限问题**
   - Windows: 以管理员身份运行 `install-deps.bat`
   - macOS/Linux: 根据提示使用 `sudo ./install-deps.sh`

4. **设备连接问题**
   - 确保 iOS 设备已连接并解锁
   - 点击设备上的"信任此电脑"
   - Windows: 确保已安装 iTunes 或 Apple Mobile Device Support
   - 新安装脚本会自动处理驱动问题

5. **安装失败问题**
   - 检查网络连接 (需要下载文件)
   - 确保磁盘空间充足
   - Windows: 临时禁用杀毒软件
   - 查看安装脚本的详细错误输出

### 日志分析

新的安装脚本 (`install-deps.py`) 提供详细的安装过程信息：
- 实时下载进度显示
- 文件校验和验证  
- 环境变量设置状态
- 安装后自动验证测试

检测脚本 (`check-deps.py`) 提供详细的诊断信息：
- 系统环境检测
- 文件路径验证
- pkg-config 查询测试
- CMake 缓存分析
- 设备连接测试

使用以下命令获得详细诊断：
```cmd
# Windows
python scripts/check-deps.py

# Unix/Linux
python3 scripts/check-deps.py
./scripts/check-deps.sh
```

### 快速诊断命令

```bash
# 检查安装状态
idevice_id --help                    # 测试 libimobiledevice
idevice_id -l                       # 列出连接的设备
pkg-config --exists libimobiledevice-1.0  # 测试 pkg-config (Unix)

# Windows 特定检查
where idevice_id                     # 查找可执行文件位置
echo %PATH%                          # 查看 PATH 环境变量
reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v PATH
```

## 开发说明

### 脚本结构

1. **模块化设计**: 每个脚本都采用函数式设计，便于维护和测试
2. **跨平台兼容**: Python 脚本可在不同操作系统上运行
3. **错误处理**: 包含完善的错误检测和用户友好的错误消息
4. **日志系统**: 统一的日志格式，便于问题诊断

### 扩展脚本

要添加新的检查或安装功能：

1. 在相应的脚本中添加新函数
2. 在 `check-deps.py` 中添加新的检查方法
3. 更新帮助文档和使用说明
4. 测试新功能在不同环境下的工作情况

## 技术参考

- [libimobiledevice 官方文档](http://www.libimobiledevice.org/)
- [libimobiledevice Windows 版本](https://github.com/libimobiledevice-win32/imobiledevice-net)
- [Qt CMake 文档](https://doc.qt.io/qt-6/cmake-manual.html)
- [CMake 文档](https://cmake.org/documentation/)
