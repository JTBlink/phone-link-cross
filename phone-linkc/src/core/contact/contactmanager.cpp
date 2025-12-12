// contactmanager.cpp — 使用 mobilesync 协议获取通讯录
// 通过 com.apple.mobilesync 服务直接同步联系人数据

#include "contactmanager.h"
#include "platform/libimobiledevice_dynamic.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDateTime>
#include <QUuid>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// 通讯录数据类标识符
static const char* CONTACTS_DATA_CLASS = "com.apple.Contacts";

ContactManager::ContactManager(QObject *parent)
    : QObject(parent)
    , m_isConnected(false)
    , m_device(nullptr)
{
}

ContactManager::~ContactManager()
{
    cleanup();
}

bool ContactManager::connectToDevice(const QString &udid)
{
    qDebug() << "ContactManager: 正在连接设备..." << udid;
    if (m_isConnected) {
        qDebug() << "ContactManager: 断开现有连接";
        disconnectFromDevice();
    }
    
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    if (!lib.isInitialized()) {
        qCritical() << "ContactManager: libimobiledevice 未初始化";
        emit errorOccurred("libimobiledevice 未初始化");
        return false;
    }
    
    // 连接设备
    qDebug() << "ContactManager: 创建设备句柄...";
    idevice_error_t ret = lib.idevice_new(&m_device, udid.toUtf8().constData());
    if (ret != IDEVICE_E_SUCCESS) {
        qWarning() << "ContactManager: idevice_new 失败，错误代码:" << ret;
        emit errorOccurred("无法连接到设备");
        return false;
    }
    
    m_currentUdid = udid;
    m_isConnected = true;
    
    // 加载之前保存的锚点
    loadAnchors();
    
    qDebug() << "ContactManager: 已成功连接到设备" << udid;
    return true;
}

void ContactManager::disconnectFromDevice()
{
    cleanup();
    m_isConnected = false;
    m_currentUdid.clear();
    m_contacts.clear();
    qDebug() << "ContactManager: 已断开设备连接";
}

bool ContactManager::syncContacts()
{
    if (!m_isConnected) {
        emit errorOccurred("设备未连接");
        return false;
    }

    emit syncProgress(0, 100);
    qDebug() << "ContactManager: 开始使用 mobilesync 同步通讯录...";

    bool result = syncContactsViaMobileSync();
    
    if (result) {
        int totalContacts = m_contacts.size();
        QString message = QString("成功同步 %1 个联系人").arg(totalContacts);
        qDebug() << "ContactManager:" << message;
        emit syncProgress(100, 100);
        emit syncCompleted(true, message);
    }
    
    return result;
}

bool ContactManager::syncContactsViaMobileSync()
{
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    // 检查 mobilesync 函数是否可用
    if (!lib.mobilesync_client_start_service ||
        !lib.mobilesync_start ||
        !lib.mobilesync_get_all_records_from_device ||
        !lib.mobilesync_receive_changes ||
        !lib.mobilesync_acknowledge_changes_from_device ||
        !lib.mobilesync_finish ||
        !lib.mobilesync_client_free ||
        !lib.mobilesync_anchors_new ||
        !lib.mobilesync_anchors_free) {
        qWarning() << "ContactManager: mobilesync 函数未加载";
        emit errorOccurred("mobilesync 服务不可用");
        return false;
    }
    
    mobilesync_client_t sync_client = nullptr;
    
    // 启动 mobilesync 服务
    qDebug() << "ContactManager: 启动 mobilesync 服务...";
    mobilesync_error_t sync_err = lib.mobilesync_client_start_service(
        m_device, &sync_client, "phone-linkc");
    
    if (sync_err != MOBILESYNC_E_SUCCESS || !sync_client) {
        qWarning() << "ContactManager: 启动 mobilesync 服务失败，错误码:" << sync_err;
        emit errorOccurred(QString("启动 mobilesync 服务失败，错误码: %1").arg(sync_err));
        return false;
    }
    
    qDebug() << "ContactManager: mobilesync 服务已启动";
    
    // 生成新的计算机锚点
    QString newComputerAnchor = generateComputerAnchor();
    
    // 创建锚点结构
    mobilesync_anchors_t anchors = lib.mobilesync_anchors_new(
        m_deviceAnchor.isEmpty() ? nullptr : m_deviceAnchor.toUtf8().constData(),
        newComputerAnchor.toUtf8().constData()
    );
    
    if (!anchors) {
        qWarning() << "ContactManager: 创建锚点失败";
        lib.mobilesync_client_free(sync_client);
        emit errorOccurred("创建同步锚点失败");
        return false;
    }
    
    // 开始同步会话
    mobilesync_sync_type_t sync_type;
    uint64_t device_data_class_version = 0;
    char* error_description = nullptr;
    
    qDebug() << "ContactManager: 开始同步会话，数据类:" << CONTACTS_DATA_CLASS;
    qDebug() << "ContactManager: 设备锚点:" << (m_deviceAnchor.isEmpty() ? "(空)" : m_deviceAnchor);
    qDebug() << "ContactManager: 计算机锚点:" << newComputerAnchor;
    
    sync_err = lib.mobilesync_start(
        sync_client,
        CONTACTS_DATA_CLASS,
        anchors,
        106,  // 计算机数据类版本（通讯录版本号）
        &sync_type,
        &device_data_class_version,
        &error_description
    );
    
    if (sync_err != MOBILESYNC_E_SUCCESS) {
        qWarning() << "ContactManager: 开始同步会话失败，错误码:" << sync_err;
        if (error_description) {
            qWarning() << "ContactManager: 错误描述:" << error_description;
            // 注意：error_description 由库分配，但我们无法安全释放它（跨 DLL 堆问题）
        }
        lib.mobilesync_anchors_free(anchors);
        lib.mobilesync_client_free(sync_client);
        emit errorOccurred(QString("开始同步会话失败，错误码: %1").arg(sync_err));
        return false;
    }
    
    qDebug() << "========== 同步会话信息 ==========";
    qDebug() << "数据类:" << CONTACTS_DATA_CLASS;
    qDebug() << "同步类型:" << sync_type;
    qDebug() << "设备数据类版本:" << device_data_class_version;
    qDebug() << "计算机数据类版本: 106";
    qDebug() << "设备锚点:" << (m_deviceAnchor.isEmpty() ? "(空)" : m_deviceAnchor);
    qDebug() << "计算机锚点:" << newComputerAnchor;
    
    if (error_description) {
        qDebug() << "错误描述:" << error_description;
    }
    
    // 根据同步类型决定策略并打印详细说明
    switch (sync_type) {
        case MOBILESYNC_SYNC_TYPE_FAST:
            qDebug() << "同步模式: FAST (快速同步) - 仅同步变更的联系人";
            break;
        case MOBILESYNC_SYNC_TYPE_SLOW:
            qDebug() << "同步模式: SLOW (慢速同步) - 完整同步所有联系人";
            m_contacts.clear();
            break;
        case MOBILESYNC_SYNC_TYPE_RESET:
            qDebug() << "同步模式: RESET (重置同步) - 清除本地数据并重新同步";
            m_contacts.clear();
            break;
        default:
            qDebug() << "同步模式: UNKNOWN (未知类型:" << sync_type << ")";
    }
    qDebug() << "===================================";
    
    emit syncProgress(10, 100);
    
    // 请求从设备获取所有记录
    qDebug() << "ContactManager: 请求获取所有联系人记录...";
    sync_err = lib.mobilesync_get_all_records_from_device(sync_client);
    
    if (sync_err != MOBILESYNC_E_SUCCESS) {
        qWarning() << "ContactManager: 请求获取记录失败，错误码:" << sync_err;
        lib.mobilesync_anchors_free(anchors);
        lib.mobilesync_client_free(sync_client);
        emit errorOccurred(QString("请求获取记录失败，错误码: %1").arg(sync_err));
        return false;
    }
    
    emit syncProgress(20, 100);
    
    // 接收联系人数据
    qDebug() << "ContactManager: 开始接收联系人数据...";
    uint8_t is_last_record = 0;
    int batch_count = 0;
    bool receive_done = false;
    
    while (!receive_done) {
        plist_t entities = nullptr;
        plist_t actions = nullptr;
        
        sync_err = lib.mobilesync_receive_changes(sync_client, &entities, &is_last_record, &actions);
        
        if (sync_err != MOBILESYNC_E_SUCCESS) {
            // 超时错误 (-5) 在已有数据的情况下可能表示传输结束
            if (sync_err == MOBILESYNC_E_RECEIVE_TIMEOUT && batch_count > 0) {
                qDebug() << "ContactManager: 接收超时，已收到" << batch_count << "批数据，视为传输完成";
                receive_done = true;
                break;
            }
            qWarning() << "ContactManager: 接收变更失败，错误码:" << sync_err;
            lib.mobilesync_anchors_free(anchors);
            lib.mobilesync_client_free(sync_client);
            emit errorOccurred(QString("接收联系人数据失败，错误码: %1").arg(sync_err));
            return false;
        }
        
        batch_count++;
        
        // 解析联系人实体
        if (entities) {
            // 打印原始 plist 数据（调试用）
            char* xml_out = nullptr;
            uint32_t xml_len = 0;
            lib.plist_to_xml(entities, &xml_out, &xml_len);
            if (xml_out && xml_len > 0) {
                // 只打印前 5000 个字符以避免日志过长
                QString xmlStr = QString::fromUtf8(xml_out, qMin((uint32_t)5000, xml_len));
                qDebug() << "ContactManager: 批次" << batch_count << "原始数据（前5000字符）:";
                qDebug().noquote() << xmlStr;
                if (xml_len > 5000) {
                    qDebug() << "... (数据已截断，总长度:" << xml_len << "字节)";
                }
                if (lib.plist_mem_free) {
                    lib.plist_mem_free(xml_out);
                }
            }
            
            QVector<Contact> batchContacts = parseContactEntities(entities);
            qDebug() << "ContactManager: 批次" << batch_count << "收到" << batchContacts.size() << "个联系人";
            
            // 打印前 5 个联系人的详细信息
            int printCount = qMin(5, batchContacts.size());
            for (int i = 0; i < printCount; i++) {
                const Contact& c = batchContacts[i];
                qDebug() << "  联系人" << (i+1) << ":"
                         << "ID=" << c.id
                         << "姓名=" << c.fullName()
                         << "电话=" << c.phoneNumbers.join(", ")
                         << "邮箱=" << c.emails.join(", ");
            }
            if (batchContacts.size() > 5) {
                qDebug() << "  ... 还有" << (batchContacts.size() - 5) << "个联系人未显示";
            }
            
            m_contacts.append(batchContacts);
            lib.plist_free(entities);
        }
        
        if (actions) {
            // 打印 actions 数据（如果有）
            char* actions_xml = nullptr;
            uint32_t actions_len = 0;
            lib.plist_to_xml(actions, &actions_xml, &actions_len);
            if (actions_xml && actions_len > 0) {
                qDebug() << "ContactManager: 批次" << batch_count << "actions 数据:";
                qDebug().noquote() << QString::fromUtf8(actions_xml, actions_len);
                if (lib.plist_mem_free) {
                    lib.plist_mem_free(actions_xml);
                }
            }
            lib.plist_free(actions);
        }
        
        // 检查是否是最后一批
        if (is_last_record) {
            qDebug() << "ContactManager: 收到最后一批数据标志";
            receive_done = true;
        }
        
        // 更新进度
        int progress = 20 + (batch_count * 10) % 60;
        emit syncProgress(progress, 100);
        
        // 处理事件以防止 UI 卡死
        QCoreApplication::processEvents();
    }
    
    qDebug() << "ContactManager: 共接收" << batch_count << "批数据，总计" << m_contacts.size() << "个联系人";
    
    emit syncProgress(80, 100);
    
    // 确认已接收变更
    qDebug() << "ContactManager: 确认接收变更...";
    sync_err = lib.mobilesync_acknowledge_changes_from_device(sync_client);
    
    if (sync_err != MOBILESYNC_E_SUCCESS) {
        qWarning() << "ContactManager: 确认变更失败，错误码:" << sync_err;
        // 继续执行，不中断
    }
    
    emit syncProgress(90, 100);
    
    // 完成同步会话
    qDebug() << "ContactManager: 完成同步会话...";
    sync_err = lib.mobilesync_finish(sync_client);
    
    if (sync_err != MOBILESYNC_E_SUCCESS) {
        qWarning() << "ContactManager: 完成同步会话失败，错误码:" << sync_err;
        // 继续执行，不中断
    }
    
    // 更新并保存锚点
    if (anchors->device_anchor) {
        m_deviceAnchor = QString::fromUtf8(anchors->device_anchor);
        qDebug() << "ContactManager: 新设备锚点:" << m_deviceAnchor;
    }
    m_computerAnchor = newComputerAnchor;
    saveAnchors();
    
    // 清理资源
    lib.mobilesync_anchors_free(anchors);
    lib.mobilesync_client_free(sync_client);
    
    qDebug() << "ContactManager: mobilesync 同步完成";
    return true;
}

QVector<Contact> ContactManager::parseContactEntities(plist_t entities)
{
    QVector<Contact> contacts;
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    if (!entities) {
        return contacts;
    }
    
    plist_type entity_type = lib.plist_get_node_type(entities);
    
    if (entity_type == PLIST_DICT) {
        // 遍历实体字典
        plist_dict_iter iter = nullptr;
        lib.plist_dict_new_iter(entities, &iter);
        
        if (iter) {
            char* key = nullptr;
            plist_t value = nullptr;
            int item_count = 0;
            
            while (true) {
                lib.plist_dict_next_item(entities, iter, &key, &value);
                if (!key) break;
                
                item_count++;
                
                // 打印前3个联系人的完整 plist 结构（调试用）
                if (item_count <= 3) {
                    char* item_xml = nullptr;
                    uint32_t item_xml_len = 0;
                    lib.plist_to_xml(value, &item_xml, &item_xml_len);
                    if (item_xml && item_xml_len > 0) {
                        QString itemXml = QString::fromUtf8(item_xml, qMin((uint32_t)2000, item_xml_len));
                        qDebug() << "ContactManager: [调试] 联系人" << item_count << "(ID:" << key << ") 完整结构:";
                        qDebug().noquote() << itemXml;
                        if (lib.plist_mem_free) {
                            lib.plist_mem_free(item_xml);
                        }
                    }
                }
                
                // 每个键是联系人的 ID，值是联系人数据
                Contact contact = parseContactFromPlist(value);
                if (!contact.id.isEmpty() || !contact.fullName().isEmpty()) {
                    if (contact.id.isEmpty()) {
                        contact.id = QString::fromUtf8(key);
                    }
                    contacts.append(contact);
                }
                
                // 注意：key 由库分配，但我们无法安全释放它（跨 DLL 堆问题）
            }
            
            // 注意：iter 由库分配，但我们无法安全释放它
        }
    } else if (entity_type == PLIST_ARRAY) {
        // 遍历实体数组
        uint32_t count = lib.plist_array_get_size(entities);
        for (uint32_t i = 0; i < count; i++) {
            plist_t item = lib.plist_array_get_item(entities, i);
            if (item) {
                Contact contact = parseContactFromPlist(item);
                if (!contact.id.isEmpty() || !contact.fullName().isEmpty()) {
                    if (contact.id.isEmpty()) {
                        contact.id = QString::number(i);
                    }
                    contacts.append(contact);
                }
            }
        }
    }
    
    return contacts;
}

Contact ContactManager::parseContactFromPlist(plist_t contactDict) const
{
    Contact contact;
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    if (!contactDict || lib.plist_get_node_type(contactDict) != PLIST_DICT) {
        return contact;
    }
    
    // 获取联系人 ID
    contact.id = getStringValue(contactDict, "com.apple.syncdevicedatabase.RecordId");
    if (contact.id.isEmpty()) {
        contact.id = getStringValue(contactDict, "RecordId");
    }
    
    // 获取姓名
    contact.firstName = getStringValue(contactDict, "first name");
    if (contact.firstName.isEmpty()) {
        contact.firstName = getStringValue(contactDict, "First");
    }
    
    contact.lastName = getStringValue(contactDict, "last name");
    if (contact.lastName.isEmpty()) {
        contact.lastName = getStringValue(contactDict, "Last");
    }
    
    // 显示名称
    contact.displayName = getStringValue(contactDict, "display name");
    if (contact.displayName.isEmpty()) {
        contact.displayName = getStringValue(contactDict, "DisplayName");
    }
    
    // 组织
    contact.organization = getStringValue(contactDict, "organization");
    if (contact.organization.isEmpty()) {
        contact.organization = getStringValue(contactDict, "Organization");
    }
    
    // 备注
    contact.note = getStringValue(contactDict, "notes");
    if (contact.note.isEmpty()) {
        contact.note = getStringValue(contactDict, "Note");
    }
    
    // 获取电话号码
    plist_t phones = lib.plist_dict_get_item(contactDict, "phone number");
    if (!phones) {
        phones = lib.plist_dict_get_item(contactDict, "phone numbers");
    }
    if (phones) {
        plist_type phones_type = lib.plist_get_node_type(phones);
        if (phones_type == PLIST_ARRAY) {
            uint32_t phone_count = lib.plist_array_get_size(phones);
            for (uint32_t i = 0; i < phone_count; i++) {
                plist_t phone_item = lib.plist_array_get_item(phones, i);
                if (phone_item) {
                    QString phone;
                    if (lib.plist_get_node_type(phone_item) == PLIST_STRING) {
                        const char* val = lib.plist_get_string_ptr(phone_item, nullptr);
                        if (val) {
                            phone = QString::fromUtf8(val);
                        }
                    } else if (lib.plist_get_node_type(phone_item) == PLIST_DICT) {
                        phone = getStringValue(phone_item, "value");
                        if (phone.isEmpty()) {
                            phone = getStringValue(phone_item, "Value");
                        }
                    }
                    if (!phone.isEmpty()) {
                        contact.phoneNumbers.append(phone);
                    }
                }
            }
        } else if (phones_type == PLIST_DICT) {
            // 遍历电话号码字典
            plist_dict_iter iter = nullptr;
            lib.plist_dict_new_iter(phones, &iter);
            if (iter) {
                char* key = nullptr;
                plist_t value = nullptr;
                while (true) {
                    lib.plist_dict_next_item(phones, iter, &key, &value);
                    if (!key) break;
                    
                    QString phone;
                    if (lib.plist_get_node_type(value) == PLIST_STRING) {
                        const char* val = lib.plist_get_string_ptr(value, nullptr);
                        if (val) {
                            phone = QString::fromUtf8(val);
                        }
                    } else if (lib.plist_get_node_type(value) == PLIST_DICT) {
                        phone = getStringValue(value, "value");
                    }
                    if (!phone.isEmpty()) {
                        contact.phoneNumbers.append(phone);
                    }
                }
            }
        }
    }
    
    // 获取邮箱
    plist_t emails = lib.plist_dict_get_item(contactDict, "email address");
    if (!emails) {
        emails = lib.plist_dict_get_item(contactDict, "email addresses");
    }
    if (emails) {
        plist_type emails_type = lib.plist_get_node_type(emails);
        if (emails_type == PLIST_ARRAY) {
            uint32_t email_count = lib.plist_array_get_size(emails);
            for (uint32_t i = 0; i < email_count; i++) {
                plist_t email_item = lib.plist_array_get_item(emails, i);
                if (email_item) {
                    QString email;
                    if (lib.plist_get_node_type(email_item) == PLIST_STRING) {
                        const char* val = lib.plist_get_string_ptr(email_item, nullptr);
                        if (val) {
                            email = QString::fromUtf8(val);
                        }
                    } else if (lib.plist_get_node_type(email_item) == PLIST_DICT) {
                        email = getStringValue(email_item, "value");
                        if (email.isEmpty()) {
                            email = getStringValue(email_item, "Value");
                        }
                    }
                    if (!email.isEmpty()) {
                        contact.emails.append(email);
                    }
                }
            }
        } else if (emails_type == PLIST_DICT) {
            plist_dict_iter iter = nullptr;
            lib.plist_dict_new_iter(emails, &iter);
            if (iter) {
                char* key = nullptr;
                plist_t value = nullptr;
                while (true) {
                    lib.plist_dict_next_item(emails, iter, &key, &value);
                    if (!key) break;
                    
                    QString email;
                    if (lib.plist_get_node_type(value) == PLIST_STRING) {
                        const char* val = lib.plist_get_string_ptr(value, nullptr);
                        if (val) {
                            email = QString::fromUtf8(val);
                        }
                    } else if (lib.plist_get_node_type(value) == PLIST_DICT) {
                        email = getStringValue(value, "value");
                    }
                    if (!email.isEmpty()) {
                        contact.emails.append(email);
                    }
                }
            }
        }
    }
    
    return contact;
}

QString ContactManager::getStringValue(plist_t dict, const char *key) const
{
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    if (!dict || !key) {
        return QString();
    }
    
    plist_t node = lib.plist_dict_get_item(dict, key);
    if (!node) {
        return QString();
    }
    
    if (lib.plist_get_node_type(node) != PLIST_STRING) {
        return QString();
    }
    
    const char* val = lib.plist_get_string_ptr(node, nullptr);
    if (val) {
        return QString::fromUtf8(val);
    }
    
    return QString();
}

QStringList ContactManager::extractStringArray(plist_t array) const
{
    QStringList result;
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    if (!array || lib.plist_get_node_type(array) != PLIST_ARRAY) {
        return result;
    }
    
    uint32_t count = lib.plist_array_get_size(array);
    for (uint32_t i = 0; i < count; i++) {
        plist_t item = lib.plist_array_get_item(array, i);
        if (item && lib.plist_get_node_type(item) == PLIST_STRING) {
            const char* val = lib.plist_get_string_ptr(item, nullptr);
            if (val) {
                result.append(QString::fromUtf8(val));
            }
        }
    }
    
    return result;
}

QString ContactManager::generateComputerAnchor() const
{
    // 生成唯一的计算机锚点（使用时间戳和 UUID）
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz");
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return QString("phone-linkc-%1-%2").arg(timestamp, uuid.left(8));
}

void ContactManager::loadAnchors()
{
    // 从配置文件加载锚点
    // 使用 %appdata%/iPhonLinkC/ 目录
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/iPhonLinkC";
    QDir().mkpath(configPath);
    
    QString anchorFile = QDir(configPath).filePath(QString("anchors_%1.json").arg(m_currentUdid));
    
    QFile file(anchorFile);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            m_deviceAnchor = obj["deviceAnchor"].toString();
            m_computerAnchor = obj["computerAnchor"].toString();
            qDebug() << "ContactManager: 已加载锚点 - 设备:" << m_deviceAnchor << "计算机:" << m_computerAnchor;
        }
    } else {
        qDebug() << "ContactManager: 无已保存的锚点，将进行完整同步";
        m_deviceAnchor.clear();
        m_computerAnchor.clear();
    }
}

void ContactManager::saveAnchors()
{
    // 保存锚点到配置文件
    // 使用 %appdata%/iPhonLinkC/ 目录
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/iPhonLinkC";
    QDir().mkpath(configPath);
    
    QString anchorFile = QDir(configPath).filePath(QString("anchors_%1.json").arg(m_currentUdid));
    
    QJsonObject obj;
    obj["deviceAnchor"] = m_deviceAnchor;
    obj["computerAnchor"] = m_computerAnchor;
    obj["lastSync"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QFile file(anchorFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
        qDebug() << "ContactManager: 已保存锚点到" << anchorFile;
    } else {
        qWarning() << "ContactManager: 无法保存锚点到" << anchorFile;
    }
}

QVector<Contact> ContactManager::searchContacts(const QString &keyword) const
{
    QVector<Contact> results;
    
    if (keyword.isEmpty()) {
        return m_contacts;
    }
    
    QString lowerKeyword = keyword.toLower();
    
    for (const Contact& contact : m_contacts) {
        bool matched = false;
        
        // 搜索姓名
        if (contact.fullName().toLower().contains(lowerKeyword)) {
            matched = true;
        }
        
        // 搜索电话号码
        if (!matched) {
            for (const QString& phone : contact.phoneNumbers) {
                if (phone.contains(keyword)) {
                    matched = true;
                    break;
                }
            }
        }
        
        // 搜索邮箱
        if (!matched) {
            for (const QString& email : contact.emails) {
                if (email.toLower().contains(lowerKeyword)) {
                    matched = true;
                    break;
                }
            }
        }
        
        // 搜索组织
        if (!matched && contact.organization.toLower().contains(lowerKeyword)) {
            matched = true;
        }
        
        if (matched) {
            results.append(contact);
        }
    }
    
    return results;
}

bool ContactManager::exportToVCard(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    for (const Contact& contact : m_contacts) {
        // vCard 3.0 格式
        out << "BEGIN:VCARD\n";
        out << "VERSION:3.0\n";
        
        // 姓名
        QString fullName = contact.fullName();
        out << "FN:" << fullName << "\n";
        out << "N:" << contact.lastName << ";" << contact.firstName << ";;;\n";
        
        // 组织
        if (!contact.organization.isEmpty()) {
            out << "ORG:" << contact.organization << "\n";
        }
        
        // 电话号码
        for (const QString& phone : contact.phoneNumbers) {
            out << "TEL:" << phone << "\n";
        }
        
        // 邮箱
        for (const QString& email : contact.emails) {
            out << "EMAIL:" << email << "\n";
        }
        
        // 备注
        if (!contact.note.isEmpty()) {
            out << "NOTE:" << contact.note << "\n";
        }
        
        out << "END:VCARD\n";
    }
    
    file.close();
    return true;
}

void ContactManager::cleanup()
{
    LibimobiledeviceDynamic& lib = LibimobiledeviceDynamic::instance();
    
    if (m_device) {
        lib.idevice_free(m_device);
        m_device = nullptr;
    }
}
