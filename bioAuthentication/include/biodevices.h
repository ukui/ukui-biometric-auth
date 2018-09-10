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
#ifndef BIODEVICES_H
#define BIODEVICES_H

#include <QObject>
#include <QMap>
#include <QList>
#include "biotypes.h"

class QDBusInterface;

/*!
 * \brief The BioDevices class
 * this class saves the list of all device information
 * and list of available devices' info list for each uid
 */
class BioDevices : public QObject
{
    Q_OBJECT
public:
    explicit BioDevices(QObject *parent = nullptr);

    /*!
     * \brief the count of available devices
     */
    int count();

    /*!
     * \brief the feature's count of the given user
     * \param uid the given user's uid
     * \return
     */
    int getDevicesCount(qint32 uid);

    int getDevicesCount();

    /*!
     * \brief the list of device's info that the given
     *        user has features
     * \param uid   the given user's uid
     * \return
     */
    QMap<int, QList<DeviceInfo>> getDevicesList(uid_t uid);

    QMap<int, QList<DeviceInfo>> getAllDevices();

    DeviceInfo* getDefaultDevice();

    /*!
     * \brief clear the list of device
     */
    void clear();

private:
    void connectToService();
    void getDevicesList();
    void getFeaturesList(qint32 uid);

signals:
    void deviceCountChanged(int newNum);

private slots:
    void onUSBDeviceHotPlug(int deviceId, int action, int devNumNow);

private:
    QDBusInterface                  *serviceInterface;

    QList<DeviceInfo*>               deviceInfos;        //the list of al device info
    QMap<int, QList<DeviceInfo>>     deviceInfosMap;   //[uid, avaliable DeviceInfos]
};



#endif // BIODEVICES_H
