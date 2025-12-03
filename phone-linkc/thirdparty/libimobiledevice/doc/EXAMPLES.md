# libimobiledevice 使用示例

## 概述

本文档提供libimobiledevice在实际项目中的使用示例，包括常见场景的完整代码实现和最佳实践。

## 快速示例

### 1. 基础设备连接

```cpp
// 简单的设备连接示例
#include <QCoreApplication>
#include <QDebug>
extern "C" {
#include "libimobiledevice/libimobiledevice.h"
#include "libimobiledevice/lockdown.h"
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    // 获取设备列表
    char **device_list = nullptr;
    int count = 0;
    
    idevice_error_t error = idevice_get_device_list(&device_list, &count);
    if (error == IDEVICE_E_SUCCESS) {
        qDebug() << "发现" << count << "个iOS设备:";
        
        for (int i = 0; i < count; i++) {
            qDebug() << "  设备" << i+1 << ":" << device_list[i];
        }
        
        // 连接第一个设备
        if (count > 0) {
            idevice_t device = nullptr;
            error = idevice_new(&device, device_list[0]);
            
            if (error == IDEVICE_E_SUCCESS) {
                qDebug() << "成功连接到设备:" << device_list[0];
                
                // 创建lockdown客户端
                lockdownd_client_t client = nullptr;
                lockdownd_error_t lockdown_error = lockdownd_client_new(device, &client, "example-app");
                
                if (lockdown_error == LOCKDOWN_E_SUCCESS) {
                    // 获取设备名称
                    plist_t device_name = nullptr;
                    lockdown_error = lockdownd_get_value(client, nullptr, "DeviceName", &device_name);
                    
                    if (lockdown_error == LOCKDOWN_E_SUCCESS && device_name) {
                        char *name_str = nullptr;
                        plist_get_string_val(device_name, &name_str);
                        qDebug() << "设备名称:" << name_str;
                        
                        free(name_str);
                        plist_free(device_name);
                    }
                    
                    lockdownd_client_free(client);
                } else {
                    qWarning() << "创建lockdown客户端失败:" << lockdown_error;
                }
                
                idevice_free(device);
            } else {
                qWarning() << "连接设备失败:" << error;
            }
        }
        
        idevice_device_list_free(device_list);
    } else {
        qWarning() << "获取设备列表失败:" << error;
    }
    
    return 0;
}
```

### 2. 设备信息批量获取

```cpp
#include <QMap>
#include <QString>

struct DeviceInfo {
    QString udid;
    QString name;
    QString model;
    QString iosVersion;
    QString serialNumber;
    QString buildVersion;
};

class DeviceInfoCollector {
public:
    static DeviceInfo collectDeviceInfo(const char* udid) {
        DeviceInfo info;
        info.udid = QString::fromUtf8(udid);
        
        idevice_t device = nullptr;
        if (idevice_new(&device, udid) != IDEVICE_E_SUCCESS) {
            qWarning() << "无法连接设备:" << udid;
            return info;
        }
        
        lockdownd_client_t client = nullptr;
        if (lockdownd_client_new(device, &client, "info-collector") != LOCKDOWN_E_SUCCESS) {
            qWarning() << "无法创建lockdown客户端";
            idevice_free(device);
            return info;
        }
        
        // 定义要获取的设备属性
        QMap<QString, QString*> properties = {
            {"DeviceName", &info.name},
            {"ProductType", &info.model},
            {"ProductVersion", &info.iosVersion},
            {"SerialNumber", &info.serialNumber},
            {"BuildVersion", &info.buildVersion}
        };
        
        // 批量获取属性
        for (auto it = properties.begin(); it != properties.end(); ++it) {
            plist_t value = nullptr;
            if (lockdownd_get_value(client, nullptr, it.key().toUtf8().constData(), &value) == LOCKDOWN_E_SUCCESS) {
                char *str_value = nullptr;
                plist_get_string_val(value, &str_value);
                *(it.value()) = QString::fromUtf8(str_value);
                
                free(str_value);
                plist_free(value);
            }
        }
        
        lockdownd_client_free(client);
        idevice_free(device);
        
        return info;
    }
    
    static QList<DeviceInfo> getAllDevicesInfo() {
        QList<DeviceInfo> devicesList;
        
        char **device_list = nullptr;
        int count = 0;
        
        if (idevice_get_device_list(&device_list, &count) == IDEVICE_E_SUCCESS) {
            for (int i = 0; i < count; i++) {
                DeviceInfo info = collectDeviceInfo(device_list[i]);
                devicesList.append(info);
                
                qDebug() << QString("设备 %1: %2 (%3) - iOS %4")
                           .arg(i+1)
                           .arg(info.name)
                           .arg(info.model)
                           .arg(info.iosVersion);
            }
            
            idevice_device_list_free(device_list);
        }
        
        return devicesList;
    }
};

// 使用示例
void printAllDevicesInfo() {
    QList<DeviceInfo> devices = DeviceInfoCollector::getAllDevicesInfo();
    
    qDebug() << "\n=== 设备信息汇总 ===";
    for (const DeviceInfo& device : devices) {
        qDebug() << "UDID:" << device.udid;
        qDebug() << "名称:" << device.name;
        qDebug() << "型号:" << device.model;
        qDebug() << "iOS版本:" << device.iosVersion;
        qDebug() << "序列号:" << device.serialNumber;
        qDebug() << "构建版本:" << device.buildVersion;
        qDebug() << "---";
    }
}
```

### 3. 屏幕截图工具

```cpp
#include <QImage>
#include <QPixmap>
#include <QLabel>
#include <QApplication>
#include <QVBoxLayout>
#include <QPushButton>
#include <QWidget>

extern "C" {
#include "libimobiledevice/screenshotr.h"
}

class ScreenshotTool : public QWidget {
    Q_OBJECT
    
public:
    ScreenshotTool(const QString& udid, QWidget* parent = nullptr) 
        : QWidget(parent), udid_(udid) {
        setupUI();
        connectDevice();
    }
    
    ~ScreenshotTool() {
        if (device_) idevice_free(device_);
    }

private slots:
    void takeScreenshot() {
        if (!device_) {
            qWarning() << "设备未连接";
            return;
        }
        
        screenshotr_client_t screenshotr = nullptr;
        screenshotr_error_t error = screenshotr_client_start_service(device_, &screenshotr, "screenshot-tool");
        
        if (error != SCREENSHOTR_E_SUCCESS) {
            qWarning() << "启动截图服务失败:" << error;
            return;
        }
        
        char *imgdata = nullptr;
        uint64_t imgsize = 0;
        
        error = screenshotr_take_screenshot(screenshotr, &imgdata, &imgsize);
        
        if (error == SCREENSHOTR_E_SUCCESS && imgdata) {
            QImage screenshot = QImage::fromData(reinterpret_cast<const uchar*>(imgdata), 
                                               static_cast<int>(imgsize), "PNG");
            
            if (!screenshot.isNull()) {
                // 显示截图
                QPixmap pixmap = QPixmap::fromImage(screenshot);
                imageLabel_->setPixmap(pixmap.scaled(400, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                
                // 保存截图
                QString filename = QString("screenshot_%1.png")
                                  .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
                screenshot.save(filename);
                qDebug() << "截图已保存:" << filename;
                
                statusLabel_->setText(QString("截图成功 - %1x%2").arg(screenshot.width()).arg(screenshot.height()));
            }
            
            free(imgdata);
        } else {
            qWarning() << "截图失败:" << error;
            statusLabel_->setText("截图失败");
        }
        
        screenshotr_client_free(screenshotr);
    }
    
    void saveScreenshot() {
        if (imageLabel_->pixmap().isNull()) {
            qWarning() << "没有可保存的截图";
            return;
        }
        
        QString filename = QFileDialog::getSaveFileName(this, 
                                                       "保存截图", 
                                                       QString("screenshot_%1.png")
                                                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
                                                       "PNG文件 (*.png)");
        
        if (!filename.isEmpty()) {
            if (imageLabel_->pixmap().save(filename)) {
                qDebug() << "截图已保存到:" << filename;
                statusLabel_->setText("保存成功: " + QFileInfo(filename).fileName());
            } else {
                qWarning() << "保存截图失败";
                statusLabel_->setText("保存失败");
            }
        }
    }

private:
    void setupUI() {
        setWindowTitle("iOS设备截图工具");
        setFixedSize(500, 750);
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        // 设备信息
        deviceInfoLabel_ = new QLabel(QString("设备: %1").arg(udid_));
        layout->addWidget(deviceInfoLabel_);
        
        // 按钮
        QPushButton* screenshotBtn = new QPushButton("截图");
        QPushButton* saveBtn = new QPushButton("保存截图");
        
        connect(screenshotBtn, &QPushButton::clicked, this, &ScreenshotTool::takeScreenshot);
        connect(saveBtn, &QPushButton::clicked, this, &ScreenshotTool::saveScreenshot);
        
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(screenshotBtn);
        buttonLayout->addWidget(saveBtn);
        layout->addLayout(buttonLayout);
        
        // 图像显示
        imageLabel_ = new QLabel();
        imageLabel_->setMinimumSize(400, 600);
        imageLabel_->setScaledContents(true);
        imageLabel_->setStyleSheet("border: 2px solid gray; background-color: white;");
        imageLabel_->setAlignment(Qt::AlignCenter);
        imageLabel_->setText("点击"截图"按钮开始");
        layout->addWidget(imageLabel_);
        
        // 状态信息
        statusLabel_ = new QLabel("就绪");
        layout->addWidget(statusLabel_);
    }
    
    bool connectDevice() {
        QByteArray udidBytes = udid_.toUtf8();
        idevice_error_t error = idevice_new(&device_, udidBytes.constData());
        
        if (error == IDEVICE_E_SUCCESS) {
            statusLabel_->setText("设备连接成功");
            return true;
        } else {
            statusLabel_->setText(QString("设备连接失败: %1").arg(error));
            return false;
        }
    }
    
    QString udid_;
    idevice_t device_ = nullptr;
    QLabel* deviceInfoLabel_;
    QLabel* imageLabel_;
    QLabel* statusLabel_;
};

// 使用示例
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 获取第一个可用设备
    char **device_list = nullptr;
    int count = 0;
    
    if (idevice_get_device_list(&device_list, &count) == IDEVICE_E_SUCCESS && count > 0) {
        QString udid = QString::fromUtf8(device_list[0]);
        
        ScreenshotTool tool(udid);
        tool.show();
        
        idevice_device_list_free(device_list);
        return app.exec();
    } else {
        qWarning() << "未找到iOS设备";
        return -1;
    }
}

#include "screenshot_example.moc"
```

### 4. 应用安装器

```cpp
#include <QProgressDialog>
#include <QFileDialog>
#include <QMessageBox>

extern "C" {
#include "libimobiledevice/installation_proxy.h"
}

class AppInstaller : public QObject {
    Q_OBJECT
    
public:
    explicit AppInstaller(const QString& udid, QObject* parent = nullptr) 
        : QObject(parent), udid_(udid) {
        connectDevice();
    }
    
    ~AppInstaller() {
        if (device_) idevice_free(device_);
    }
    
    bool installIPA(const QString& ipaPath, QWidget* parent = nullptr) {
        if (!device_) {
            QMessageBox::warning(parent, "错误", "设备未连接");
            return false;
        }
        
        QFileInfo fileInfo(ipaPath);
        if (!fileInfo.exists() || fileInfo.suffix().toLower() != "ipa") {
            QMessageBox::warning(parent, "错误", "IPA文件不存在或格式不正确");
            return false;
        }
        
        // 创建进度对话框
        QProgressDialog progress("正在安装应用...", "取消", 0, 100, parent);
        progress.setWindowModality(Qt::WindowModal);
        progress.show();
        
        // 启动安装代理服务
        instproxy_client_t instproxy = nullptr;
        instproxy_error_t error = instproxy_client_start_service(device_, &instproxy, "app-installer");
        
        if (error != INSTPROXY_E_SUCCESS) {
            QMessageBox::critical(parent, "错误", QString("启动安装服务失败: %1").arg(error));
            return false;
        }
        
        // 设置安装选项
        plist_t options = plist_new_dict();
        plist_dict_set_item(options, "PackageType", plist_new_string("Developer"));
        
        // 设置回调数据
        InstallCallbackData callbackData;
        callbackData.progress = &progress;
        callbackData.success = false;
        callbackData.errorMessage = QString();
        
        // 开始安装
        QByteArray ipaPathBytes = ipaPath.toUtf8();
        error = instproxy_install(instproxy, ipaPathBytes.constData(), options, 
                                 installStatusCallback, &callbackData);
        
        bool success = (error == INSTPROXY_E_SUCCESS) && callbackData.success;
        
        if (!success) {
            QString message = callbackData.errorMessage.isEmpty() ? 
                            QString("安装失败 (错误码: %1)").arg(error) : callbackData.errorMessage;
            QMessageBox::critical(parent, "安装失败", message);
        } else {
            QMessageBox::information(parent, "成功", "应用安装完成！");
        }
        
        plist_free(options);
        instproxy_client_free(instproxy);
        progress.hide();
        
        return success;
    }
    
    QStringList getInstalledApps() {
        QStringList appList;
        
        if (!device_) return appList;
        
        instproxy_client_t instproxy = nullptr;
        if (instproxy_client_start_service(device_, &instproxy, "app-lister") != INSTPROXY_E_SUCCESS) {
            return appList;
        }
        
        // 设置浏览选项（只获取用户应用）
        plist_t options = plist_new_dict();
        plist_t app_types = plist_new_array();
        plist_array_append_item(app_types, plist_new_string("User"));
        plist_dict_set_item(options, "ApplicationType", app_types);
        
        plist_t result = nullptr;
        if (instproxy_browse(instproxy, options, &result) == INSTPROXY_E_SUCCESS) {
            uint32_t app_count = plist_array_get_size(result);
            
            for (uint32_t i = 0; i < app_count; i++) {
                plist_t app_dict = plist_array_get_item(result, i);
                
                // 获取应用信息
                plist_t bundle_id_node = plist_dict_get_item(app_dict, "CFBundleIdentifier");
                plist_t display_name_node = plist_dict_get_item(app_dict, "CFBundleDisplayName");
                
                if (bundle_id_node && display_name_node) {
                    char *bundle_id = nullptr, *display_name = nullptr;
                    plist_get_string_val(bundle_id_node, &bundle_id);
                    plist_get_string_val(display_name_node, &display_name);
                    
                    QString appInfo = QString("%1 (%2)")
                                     .arg(QString::fromUtf8(display_name))
                                     .arg(QString::fromUtf8(bundle_id));
                    appList.append(appInfo);
                    
                    free(bundle_id);
                    free(display_name);
                }
            }
            
            plist_free(result);
        }
        
        plist_free(options);
        instproxy_client_free(instproxy);
        
        return appList;
    }

private:
    struct InstallCallbackData {
        QProgressDialog* progress;
        bool success;
        QString errorMessage;
    };
    
    static void installStatusCallback(const char *operation, plist_t status, void *user_data) {
        InstallCallbackData* data = static_cast<InstallCallbackData*>(user_data);
        
        if (!status || !data->progress) return;
        
        // 检查用户是否取消
        if (data->progress->wasCanceled()) {
            return;
        }
        
        // 获取进度
        plist_t progress_node = plist_dict_get_item(status, "PercentComplete");
        if (progress_node) {
            uint64_t progress = 0;
            plist_get_uint_val(progress_node, &progress);
            data->progress->setValue(static_cast<int>(progress));
            
            // 更新状态文本
            plist_t status_node = plist_dict_get_item(status, "Status");
            if (status_node) {
                char *status_str = nullptr;
                plist_get_string_val(status_node, &status_str);
                
                QString statusText = QString::fromUtf8(status_str);
                if (statusText == "Complete") {
                    data->success = true;
                    data->progress->setLabelText("安装完成");
                } else if (statusText.contains("Error")) {
                    plist_t error_node = plist_dict_get_item(status, "ErrorDescription");
                    if (error_node) {
                        char *error_str = nullptr;
                        plist_get_string_val(error_node, &error_str);
                        data->errorMessage = QString::fromUtf8(error_str);
                        free(error_str);
                    }
                } else {
                    data->progress->setLabelText(statusText);
                }
                
                free(status_str);
            }
        }
        
        // 处理Qt事件以更新UI
        QCoreApplication::processEvents();
    }
    
    bool connectDevice() {
        QByteArray udidBytes = udid_.toUtf8();
        idevice_error_t error = idevice_new(&device_, udidBytes.constData());
        return (error == IDEVICE_E_SUCCESS);
    }
    
    QString udid_;
    idevice_t device_ = nullptr;
};

// 使用示例 - 应用安装对话框
class AppInstallerDialog : public QDialog {
    Q_OBJECT
    
public:
    AppInstallerDialog(const QString& udid, QWidget* parent = nullptr) 
        : QDialog(parent), installer_(new AppInstaller(udid, this)) {
        setupUI();
        refreshAppList();
    }

private slots:
    void selectAndInstallIPA() {
        QString ipaPath = QFileDialog::getOpenFileName(this, 
                                                      "选择IPA文件", 
                                                      QString(), 
                                                      "IPA文件 (*.ipa)");
        
        if (!ipaPath.isEmpty()) {
            if (installer_->installIPA(ipaPath, this)) {
                refreshAppList(); // 刷新应用列表
            }
        }
    }
    
    void refreshAppList() {
        appListWidget_->clear();
        
        QStringList apps = installer_->getInstalledApps();
        for (const QString& app : apps) {
            appListWidget_->addItem(app);
        }
        
        statusLabel_->setText(QString("共 %1 个用户应用").arg(apps.size()));
    }

private:
    void setupUI() {
        setWindowTitle("iOS应用安装器");
        setMinimumSize(500, 400);
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        // 操作按钮
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* installBtn = new QPushButton("安装IPA");
        QPushButton* refreshBtn = new QPushButton("刷新列表");
        
        connect(installBtn, &QPushButton::clicked, this, &AppInstallerDialog::selectAndInstallIPA);
        connect(refreshBtn, &QPushButton::clicked, this, &AppInstallerDialog::refreshAppList);
        
        buttonLayout->addWidget(installBtn);
        buttonLayout->addWidget(refreshBtn);
        buttonLayout->addStretch();
        layout->addLayout(buttonLayout);
        
        // 应用列表
        QLabel* listLabel = new QLabel("已安装的用户应用:");
        layout->addWidget(listLabel);
        
        appListWidget_ = new QListWidget();
        layout->addWidget(appListWidget_);
        
        // 状态栏
        statusLabel_ = new QLabel("就绪");
        layout->addWidget(statusLabel_);
    }
    
    AppInstaller* installer_;
    QListWidget* appListWidget_;
    QLabel* statusLabel_;
};

#include "app_installer_example.moc"
```

### 5. 文件传输工具

```cpp
#include <QTreeWidget>
#include <QSplitter>
#include <QHeaderView>

extern "C" {
#include "libimobiledevice/afc.h"
}

class FileTransferTool : public QWidget {
    Q_OBJECT
    
public:
    FileTransferTool(const QString& udid, QWidget* parent = nullptr) 
        : QWidget(parent), udid_(udid) {
        setupUI();
        connectDevice();
        refreshRemoteFiles();
    }
    
    ~FileTransferTool() {
        if (device_) idevice_free(device_);
    }

private slots:
    void uploadFile() {
        QString localPath = QFileDialog::getOpenFileName(this, "选择要上传的文件");
        if (localPath.isEmpty()) return;
        
        QFileInfo fileInfo(localPath);
        QString remotePath = QString("/tmp/%1").arg(fileInfo.fileName());
        
        if (transferFile(localPath, remotePath, true)) {
            QMessageBox::information(this, "成功", "文件上传成功");
            refreshRemoteFiles();
        }
    }
    
    void downloadFile() {
        QTreeWidgetItem* item = remoteTreeWidget_->currentItem();
        if (!item || item->data(0, Qt::UserRole).toString() != "file") {
            QMessageBox::warning(this, "提示", "请选择一个文件");
            return;
        }
        
        QString remotePath = item->data(1, Qt::UserRole).toString();
        QString fileName = QFileInfo(remotePath).fileName();
        
        QString localPath = QFileDialog::getSaveFileName(this, "保存文件", fileName);
        if (localPath.isEmpty()) return;
        
        if (transferFile(localPath, remotePath, false)) {
            QMessageBox::information(this, "成功", "文件下载成功");
        }
    }
    
    void refreshRemoteFiles() {
        remoteTreeWidget_->clear();
        
        if (!device_) return;
        
        afc_client_t afc = nullptr;
        if (afc_client_start_service(device_, &afc, "file-transfer") != AFC_E_SUCCESS) {
            statusLabel_->setText("无法启动AFC服务");
            return;
        }
        
        loadDirectory(afc, "/", nullptr);
        
        afc_client_free(afc);
        statusLabel_->setText("目录列表已更新");
    }
    
    void onItemDoubleClicked(QTreeWidgetItem* item, int column) {
        Q_UNUSED(column)
        
        if (item->data(0, Qt::UserRole).toString() == "directory") {
            // 切换目录展开状态
            item->setExpanded(!item->isExpanded());
            
            // 如果是首次展开，加载子目录
            if (item->isExpanded() && item->childCount() == 0) {
                QString dirPath = item->data(1, Qt::UserRole).toString();
                
                afc_client_t afc = nullptr;
                if (afc_client_start_service(device_, &afc, "file-browser") == AFC_E_SUCCESS) {
                    loadDirectory(afc, dirPath, item);
                    afc_client_free(afc);
                }
            }
        }
    }

private:
    void setupUI() {
        setWindowTitle("iOS文件传输工具");
        setMinimumSize(800, 600);
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        // 工具栏
        QHBoxLayout* toolbarLayout = new QHBoxLayout();
        QPushButton* uploadBtn = new QPushButton("上传文件");
        QPushButton* downloadBtn = new QPushButton("下载文件");
        QPushButton* refreshBtn = new QPushButton("刷新");
        
        connect(uploadBtn, &QPushButton::clicked, this, &FileTransferTool::uploadFile);
        connect(downloadBtn, &QPushButton::clicked, this, &FileTransferTool::downloadFile);
        connect(refreshBtn, &QPushButton::clicked, this, &FileTransferTool::refreshRemoteFiles);
        
        toolbarLayout->addWidget(uploadBtn);
        toolbarLayout->addWidget(downloadBtn);
        toolbarLayout->addWidget(refreshBtn);
        toolbarLayout->addStretch();
        layout->addLayout(toolbarLayout);
        
        // 文件列表
        QSplitter* splitter = new QSplitter(Qt::Horizontal);
        
        // 远程文件列表
        remoteTreeWidget_ = new QTreeWidget();
        remoteTreeWidget_->setHeaderLabels({"名称", "大小", "类型"});
        remoteTreeWidget_->header()->resizeSection(0, 300);
        connect(remoteTreeWidget_, &QTreeWidget::itemDoubleClicked, 
                this, &FileTransferTool::onItemDoubleClicked);
        
        QWidget* remoteWidget = new QWidget();
        QVBoxLayout* remoteLayout = new QVBoxLayout(remoteWidget);
        remoteLayout->addWidget(new QLabel("设备文件:"));
        remoteLayout->addWidget(remoteTreeWidget_);
        
        splitter->addWidget(remoteWidget);
        layout->addWidget(splitter);
        
        // 状态栏
        statusLabel_ = new QLabel("就绪");
        layout->addWidget(statusLabel_);
        
        // 进度条
        progressBar_ = new QProgressBar();
        progressBar_->setVisible(false);
        layout->addWidget(progressBar_);
    }
    
    bool connectDevice() {
        QByteArray udidBytes = udid_.toUtf8();
        idevice_error_t error = idevice_new(&device_, udidBytes.constData());
        
        if (error == IDEVICE_E_SUCCESS) {
            statusLabel_->setText("设备连接成功");
            return true;
        } else {
            statusLabel_->setText(QString("设备连接失败: %1").arg(error));
            return false;
        }
    }
    
    void loadDirectory(afc_client_t afc, const QString& path, QTreeWidgetItem* parentItem) {
        char **list = nullptr;
        QByteArray pathBytes = path.toUtf8();
        
        if (afc_read_directory(afc, pathBytes.constData(), &list) != AFC_E_SUCCESS) {
            return;
        }
        
        int i = 0;
        while (list[i]) {
            QString filename = QString::fromUtf8(list[i]);
            if (filename != "." && filename != "..") {
                QString fullPath = path.endsWith("/") ? path + filename : path + "/" + filename;
