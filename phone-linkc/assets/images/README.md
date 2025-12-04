# 图片素材目录

## 目录结构

```
phone-linkc/assets/images/
├── screenshots/        # 应用截图
├── icons/             # 图标文件
├── diagrams/          # 架构图表
├── logos/             # 项目标志
└── README.md          # 本文件
```

## 图片规范

### 截图 (screenshots/)
- **格式**: PNG/JPEG
- **尺寸**: 推荐 1920x1080 或 1280x720
- **命名**: `功能名称_平台.png` (如: `main-window_windows.png`)

### 图标 (icons/)
- **格式**: PNG/SVG (推荐SVG)
- **尺寸**: 16x16, 32x32, 64x64, 128x128, 256x256
- **命名**: `icon_尺寸.png` 或 `icon.svg`

### 架构图 (diagrams/)
- **格式**: PNG/SVG/PDF
- **命名**: 描述性名称，如 `architecture-overview.png`

### 标志 (logos/)
- **格式**: PNG/SVG (推荐SVG)
- **变体**: 完整标志、图标版本、深色/浅色主题版本
- **命名**: `logo.svg`, `logo-dark.svg`, `logo-light.svg`

## 待添加图片清单

### 截图
- [ ] 主界面截图 (Windows/macOS/Linux)
- [ ] 设备列表界面
- [ ] 设备信息详情页
- [ ] 设置界面
- [ ] 连接状态显示
- [ ] 错误处理界面

### 图标
- [ ] 应用程序图标 (多尺寸)
- [ ] 设备类型图标 (iPhone, iPad等)
- [ ] 状态图标 (连接中、已连接、断开等)
- [ ] 功能图标 (刷新、设置、信息等)

### 架构图
- [ ] 系统架构图
- [ ] 数据流图
- [ ] 组件关系图
- [ ] 部署结构图

### 标志
- [ ] 项目主标志
- [ ] 深色主题标志
- [ ] 浅色主题标志
- [ ] 图标版本标志

## 使用示例

### 在README中引用截图
```markdown
![主界面截图](assets/images/screenshots/main-window_windows.png)
```

### 在文档中使用图标
```markdown
![连接状态](assets/images/icons/connected_32.png) 设备已连接
```

## 贡献指南

1. 图片文件名使用英文和下划线
2. 截图尽量包含窗口边框，展示完整界面
3. 图标使用统一的设计风格
4. SVG文件优于位图格式
5. 提供高分辨率版本以适配高DPI显示器
