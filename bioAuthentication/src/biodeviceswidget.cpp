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
#include "biodeviceswidget.h"
#include "ui_biodeviceswidget.h"

BioDevicesWidget::BioDevicesWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BioDevicesWidget)
{
    ui->setupUi(this);

    connect(&bioDevices, &BioDevices::deviceCountChanged,
            this, &BioDevicesWidget::onDeviceCountChanged);
}

BioDevicesWidget::~BioDevicesWidget()
{
    delete ui;
}

void BioDevicesWidget::init(uid_t uid)
{
    this->uid = uid;
    devicesMap = bioDevices.getAllDevices();

    ui->cmbDeviceTypes->clear();

    for(auto i : devicesMap.keys()) {
        ui->cmbDeviceTypes->addItem(BioDevices::bioTypeToString_tr(i), i);
    }

    /* set the default device as current device */
    DeviceInfo *device = bioDevices.getDefaultDevice(uid);
    if(device) {
        int index = ui->cmbDeviceTypes->findData(device->biotype);
        QList<DeviceInfo> &deviceList = devicesMap[index];
        auto iter = std::find_if(deviceList.constBegin(), deviceList.constEnd(),
                                 [&](const DeviceInfo &deviceInfo){
            return deviceInfo.device_shortname == device->device_shortname;
        });
        int row = iter - deviceList.constBegin();

        ui->cmbDeviceTypes->setCurrentIndex(index);
        ui->lwDevices->setCurrentRow(row);
    }
    Q_EMIT deviceCountChanged(bioDevices.count());
}

void BioDevicesWidget::on_btnBack_clicked()
{
    Q_EMIT back();
}

void BioDevicesWidget::on_btnOK_clicked()
{
    int row = ui->lwDevices->currentRow();
    int type = ui->cmbDeviceTypes->itemData(ui->cmbDeviceTypes->currentIndex()).toInt();

    Q_EMIT deviceChanged(devicesMap[type].at(row));
}

void BioDevicesWidget::on_lwDevices_doubleClicked(const QModelIndex &)
{
    on_btnOK_clicked();
}

void BioDevicesWidget::on_cmbDeviceTypes_currentIndexChanged(int index)
{
    qDebug() << "type changed";
    ui->lwDevices->clear();

    int i = ui->cmbDeviceTypes->itemData(index).toInt();
    for(auto device : devicesMap[i])
        ui->lwDevices->addItem(device.device_shortname);
}

void BioDevicesWidget::onDeviceCountChanged()
{
    int type = ui->cmbDeviceTypes->itemData(ui->cmbDeviceTypes->currentIndex()).toInt();
    init(uid);
    int index = ui->cmbDeviceTypes->findData(type);
    ui->cmbDeviceTypes->setCurrentIndex(index >= 0 ? index : 0);

    Q_EMIT deviceCountChanged(bioDevices.count());
}
