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
#ifndef BIODEVICESWIDGET_H
#define BIODEVICESWIDGET_H

#include <QWidget>
#include "biodevices.h"

namespace Ui {
class BioDevicesWidget;
}

class BioDevicesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BioDevicesWidget(QWidget *parent = 0);
    void setCurrentDevice(DeviceInfo *device);
    ~BioDevicesWidget();
    void init(uid_t uid);
    enum FLAG{FIRST,AGAIN};
signals:
    void back();
    void deviceChanged(const DeviceInfo &device);
    void deviceCountChanged(int count);

private slots:
//    void on_btnBack_clicked();
//    void on_btnOK_clicked();
    void on_lwDevices_currentIndexChanged();
    void on_cmbDeviceTypes_currentIndexChanged(int index);

    void onDeviceCountChanged();

private:
    Ui::BioDevicesWidget *ui;
    BioDevices bioDevices;
    QMap<int, QList<DeviceInfo>> devicesMap;

    uid_t uid;
};

#endif // BIODEVICESWIDGET_H
