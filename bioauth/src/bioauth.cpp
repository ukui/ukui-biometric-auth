/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * 
**/
#include "bioauth.h"
#include <QList>
#include <pwd.h>

#define UKUI_BIOMETRIC_CONFIG_PATH  ".biometric_auth/ukui_biometric.conf"
#define UKUI_BIOMETRIC_SYS_CONFIG_PATH  "/etc/biometric-auth/ukui-biometric.conf"

QString GetDefaultDevice(const QString &userName)
{
    QString configPath = QString("/home/%1/" UKUI_BIOMETRIC_CONFIG_PATH).arg(userName);
    QSettings settings(configPath, QSettings::IniFormat);

    QString defaultDevice = settings.value("DefaultDevice").toString();
    if(defaultDevice.isEmpty())
    {
        QSettings sysSettings(UKUI_BIOMETRIC_SYS_CONFIG_PATH, QSettings::IniFormat);
        defaultDevice = sysSettings.value("DefaultDevice").toString();
    }

    return defaultDevice;
}

static int getValueFromSettings(const QString &userName, const QString &key, int defaultValue = 3)
{
    //从家目录下的配置文件中获取
    QString configPath = QString("/home/%1/" UKUI_BIOMETRIC_CONFIG_PATH).arg(userName);
    QSettings settings(configPath, QSettings::IniFormat);
    QString valueStr = settings.value(key).toString();

    //如果没有获取到，则从系统配置文件中获取
    if(valueStr.isEmpty())
    {
        QSettings sysSettings(UKUI_BIOMETRIC_SYS_CONFIG_PATH, QSettings::IniFormat);
        valueStr = sysSettings.value(key).toString();
    }

    bool ok;
    int value = valueStr.toInt(&ok);
    if( (value == 0 && !ok) || valueStr.isEmpty() )
    {
        value = defaultValue;
    }
    return value;
}

int GetMaxFailedAutoRetry(const QString &userName)
{
    return getValueFromSettings(userName, "MaxFailedAutoRetry");
}

int GetMaxTimeoutAutoRetry(const QString &userName)
{
    return getValueFromSettings(userName, "MaxTimeoutAutoRetry");
}


BioAuth::BioAuth(qint32 uid, const DeviceInfo &deviceInfo, QObject *parent)
    : QObject(parent),
      userName(getpwuid(uid)->pw_name),
      uid(uid),
      deviceInfo(deviceInfo),
      isInAuthentication(false),
      failedCount(0),
      beStopped(false)
{
    serviceInterface = new QDBusInterface(BIO_DBUS_SERVICE,
                                          BIO_DBUS_PATH,
                                          BIO_DBUS_INTERFACE,
                                          QDBusConnection::systemBus());

    connect(serviceInterface, SIGNAL(StatusChanged(int, int)),
            this, SLOT(onStatusChanged(int,int)));
    serviceInterface->setTimeout(2147483647);
}

BioAuth::~BioAuth()
{
    stopAuth();
}

void BioAuth::setDevice(const DeviceInfo &deviceInfo)
{
    this->deviceInfo = deviceInfo;
}

void BioAuth::startAuth()
{
    if(isInAuthentication)
        stopAuth();

    /* 开始认证识别 */
    LOG() << "start biometric verification";

    failedCount = 0;
    beStopped = false;

    _startAuth();
}

void BioAuth::_startAuth()
{
    isInAuthentication = true;

    QList<QVariant> args;
    args << QVariant(deviceInfo.device_id) << QVariant(uid)
         << QVariant(0) << QVariant(-1);
    QDBusPendingCall call = serviceInterface->asyncCallWithArgumentList("Identify", args);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &BioAuth::onIdentityComplete);
}


void BioAuth::stopAuth()
{
    beStopped = true;

    QDBusReply<int> reply = serviceInterface->call("StopOps", QVariant(deviceInfo.device_id), QVariant(5));
    if(!reply.isValid())
        qWarning() << "StopOps error: " << reply.error();

    isInAuthentication = false;
}

bool BioAuth::isAuthenticating()
{
    return isInAuthentication;
}

void BioAuth::onIdentityComplete(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<qint32, qint32> reply = *watcher;
    if(reply.isError()){
        isInAuthentication = false;
        LOG() << reply.error();
        Q_EMIT authComplete(-1, false);
        return;
    }
    qint32 result = reply.argumentAt(0).toInt();
    qint32 retUid = reply.argumentAt(1).toInt();
    LOG() << "Identify complete: " << result << " " << retUid;

    /* 识别生物特征成功，发送认证结果 */
    if(isInAuthentication){

        isInAuthentication = false;

        if(result == DBUS_RESULT_SUCCESS && retUid == uid)
        {
            Q_EMIT authComplete(retUid, true);
        }
        else if(result == DBUS_RESULT_NOTMATCH)
        {
            failedCount++;
            if(failedCount >= GetMaxFailedAutoRetry(userName))
            {
                Q_EMIT authComplete(retUid, false);
            }
            else
            {
                Q_EMIT notify(tr("Identify failed, Please retry."));
                QTimer::singleShot(1000, [&]{
                    if(!beStopped)
                    {
                        _startAuth();
                    }
                });
            }
        }
    }
}


void BioAuth::onStatusChanged(int deviceId, int statusType)
{
    if(statusType != STATUS_NOTIFY)
        return;
    LOG() << "status changed " << deviceId << " " << statusType;
    QDBusMessage msg = serviceInterface->call("GetNotifyMesg", QVariant(deviceId));
    if(msg.type() == QDBusMessage::ErrorMessage){
        LOG() << msg.errorMessage();
        return;
    }
    QString message = msg.arguments().at(0).toString();
    LOG() << message;
    Q_EMIT notify(message);
}

