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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDesktopWidget>
#include "generic.h"

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    widgetBioAuth = new BioAuthWidget(this);
    widgetBioAuth->setMaximumWidth(230);
    widgetBioDevices = new BioDevicesWidget(this);
    widgetBioDevices->setMaximumWidth(230);

    ui->mainLayout->addWidget(widgetBioAuth);
    ui->mainLayout->addWidget(widgetBioDevices);

    connect(widgetBioDevices, &BioDevicesWidget::deviceChanged,
            this, [&](const DeviceInfo &device){
        widgetBioAuth->startAuth(1000, device);
        switchWidget(DEVICES);
    });

    connect(widgetBioDevices, &BioDevicesWidget::back,
            this, [&]{
        switchWidget(BIOMETRIC);
    });

    connect(widgetBioAuth, &BioAuthWidget::authComplete,
            this, [&](uid_t uid, bool ret){
        qDebug() << "biometric authentication complete: " << uid << ret;
        if(uid == 1000 && ret)
            exit(BIO_SUCCESS);
        else
            exit(BIO_FAILED);
    });

    connect(widgetBioAuth, &BioAuthWidget::selectDevice,
            this, [&]{
        widgetBioDevices->init();
        widgetBioAuth->hide();
        widgetBioDevices->show();
    });

    connect(widgetBioAuth, &BioAuthWidget::switchToPassword,
            this, [&] {
        exit(BIO_IGNORE);
    });

    DeviceInfo *defaultDevice = bioDevices.getDefaultDevice();
    if(!defaultDevice) {
        LOG() << "no available device";
        exit(BIO_IGNORE);
    }
    widgetBioAuth->startAuth(1000, *defaultDevice);
    switchWidget(BIOMETRIC);

    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    QDesktopWidget *desktop = QApplication::desktop();
    int x = (desktop->geometry().width() - width()) / 2;
    int y = (desktop->geometry().height() - height()) / 2;
    move(x, y);

    QFile qssFile(":/qss/src/pam_gui.qss");
    qssFile.open(QIODevice::ReadOnly);
    setStyleSheet(qssFile.readAll());
    qssFile.close();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::switchWidget(Mode mode)
{
    widgetBioAuth->hide();
    widgetBioDevices->hide();

    switch(mode){
    case BIOMETRIC:
        widgetBioAuth->show();
        break;
    case DEVICES:
        widgetBioDevices->show();
        break;
    default:
        break;
    }
    adjustSize();
}
