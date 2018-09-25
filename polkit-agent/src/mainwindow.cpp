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
#include <QMovie>
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QFontMetrics>
#include <QtMath>

#include <sys/types.h>
#include <pwd.h>

#include "generic.h"

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow),
    enableBioAuth(false),
    receiveBioPAM(false),
    authMode(UNDEFINED)
{
    ui->setupUi(this);
    setWindowTitle(tr("Authentication"));
    setWindowFlags(Qt::WindowStaysOnTopHint);

    widgetBioAuth = new BioAuthWidget(this);
    widgetBioDevices = new BioDevicesWidget(this);
    ui->formLayout->addWidget(widgetBioAuth);
    ui->formLayout->addWidget(widgetBioDevices);

    connect(widgetBioDevices, &BioDevicesWidget::deviceChanged,
            this, [&](const DeviceInfo &device){
        widgetBioAuth->startAuth(getUid(userName), device);
        switchWidget(BIOMETRIC);
    });

    connect(widgetBioDevices, &BioDevicesWidget::back,
            this, [&]{
        switchWidget(BIOMETRIC);
    });

    connect(&bioDevices, &BioDevices::deviceCountChanged,
            this, [&]{
        widgetBioAuth->setMoreDevices(bioDevices.count() > 1);
    });

    connect(widgetBioAuth, &BioAuthWidget::authComplete,
            this, [&](uid_t uid, bool ret){
        qDebug() << "biometric authentication complete: " << uid << ret;
        if(uid == getUid(userName) && ret)
            accept(BIOMETRIC_SUCCESS);
//        else
//            accept(BIOMETRIC_FAILED);
    });

    connect(widgetBioAuth, &BioAuthWidget::selectDevice,
            this, [&]{
        widgetBioDevices->init(getUid(userName));
        switchWidget(DEVICES);
    });

    connect(widgetBioAuth, &BioAuthWidget::switchToPassword,
            this, [&] {
        accept(BIOMETRIC_IGNORE);
    });

    QFile qssFile(":/qss/src/main.qss");
    qssFile.open(QIODevice::ReadOnly);
    setStyleSheet(qssFile.readAll());
    qssFile.close();

    ui->cmbUsers->hide();
    ui->widgetDetails->hide();
    ui->btnDetails->setIcon(QIcon(":/image/assets/right-arrow.png"));

    switchWidget(UNDEFINED);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    emit canceled();

    return QWidget::closeEvent(event);
}


/*** main ***/

void MainWindow::on_cmbUsers_currentTextChanged(const QString &userName)
{
    qDebug() << "user changed to " << userName;

    this->userName = userName;
    ui->lblMessage->clear();
    emit userChanged(userName);
}

void MainWindow::on_btnDetails_clicked()
{
    if(ui->widgetDetails->isHidden()) {
        ui->widgetDetails->show();
        ui->btnDetails->setIcon(QIcon(":/image/assets/left-arrow.png"));
//        resize(width(), height() + ui->widgetDetails->height());
    }
    else {
        ui->widgetDetails->hide();
        ui->btnDetails->setIcon(QIcon(":/image/assets/right-arrow.png"));
//        resize(width(), height() - ui->widgetDetails->height());
    }
    adjustSize();
}

void MainWindow::on_btnCancel_clicked()
{
    close();
}

void MainWindow::on_btnAuth_clicked()
{
    on_lePassword_returnPressed();
}

/*** pagePassword ***/

void MainWindow::on_lePassword_returnPressed()
{
    emit accept(ui->lePassword->text());
    switchWidget(UNDEFINED);
    setMessage(tr("in authentication, please wait..."));
}

void MainWindow::on_btnBioAuth_clicked()
{
    emit switchToBiometric();
    authMode = BIOMETRIC;
}

/*** end of control's slot ***/


/*** public member ***/

void MainWindow::setIcon(const QString &iconName)
{
    QPixmap icon = QIcon::fromTheme("dialog-password").pixmap(64, 64);
    QPixmap actionIcon = QIcon::fromTheme(iconName).pixmap(32, 32);
    QPainter painter;

    painter.begin(&icon);
    QRect rect(icon.width() - actionIcon.width(),
               icon.height() - actionIcon.height(),
               actionIcon.width(), actionIcon.height());
    painter.drawPixmap(rect, actionIcon);
    painter.end();

    ui->lblIcon->setPixmap(icon);
}

void MainWindow::setHeader(const QString &text)
{
    ui->lblHeader->setText(text);
    ui->lblContent->setText(tr("An application is attempting to perform an action that requires privileges."
                            " Authentication is required to perform this action."));
}

void MainWindow::setUsers(const QStringList &usersList)
{
    if(usersList.isEmpty())
        return;

    if(usersList.size() == 1) {
        userName = usersList.at(0);
        return;
    }

    ui->cmbUsers->addItems(usersList);
    ui->cmbUsers->show();
}

void MainWindow::setDetails(const QString &subPid, const QString &callerPid, const QString &actionId,
                            const QString &actionDesc, const QString vendorName,
                            const QString vendorUrl)
{
    ui->lblSubjectPid->setText(subPid);
    ui->lblCallerPid->setText(callerPid);
    ui->lblActionId->setText(actionId);
    ui->lblActionDesc->setText(actionDesc);
    QString vendor = QString("<a href=\"%1\">%2").arg(vendorUrl).arg(vendorName);
    ui->lblVendor->setText(vendor);
}

void MainWindow::setPrompt(const QString &text, bool echo)
{
    QString prompt = text;

    if(text == "Password: ")
        prompt = tr("Password: ");

    ui->lblPrompt->setText(prompt);
    ui->lePassword->setEchoMode(echo ? QLineEdit::Normal : QLineEdit::Password);
    switchWidget(PASSWORD);
}

void MainWindow::setMessage(const QString &text)
{
    ui->lblMessage->setText(text);
}

void MainWindow::setAuthResult(bool result, const QString &text)
{
    QString message = text;

    if(!result && text.isEmpty())
        message = tr("Authentication failed, please try again.");

    if(authMode == PASSWORD)
        ui->lblMessage->setText(message);
//    else if(authMode == BIOMETRIC)
//        ui->lblBioNotify->setText(message);

    switchWidget(PASSWORD);
}

void MainWindow::clearEdit()
{
    ui->lePassword->setText("");
}

void MainWindow::switchAuthMode(Mode mode)
{
    enableBioAuth  = bioDevices.count() > 0;

    switch(mode){
    case PASSWORD:
    {
        qDebug() << "switch to password";

        authMode = mode;

        if(!enableBioAuth || !receiveBioPAM) {
            ui->btnBioAuth->hide();
        }
    }
        break;
    case BIOMETRIC:
    {
        qDebug() << "switch to biometric";

        receiveBioPAM = true;

        if(authMode == PASSWORD) {
            emit accept(BIOMETRIC_IGNORE);
            break;
        } else if(authMode == BIOMETRIC) {
            DeviceInfo *device = bioDevices.getDefaultDevice(getUid(userName));
            if(!device)
                device = bioDevices.getFirstDevice();
            if(device) {
                widgetBioAuth->startAuth(getUid(userName), *device);
                widgetBioAuth->setMoreDevices(bioDevices.count() > 1);
            }
            authMode = BIOMETRIC;
        } else if(authMode == UNDEFINED){

            authMode = BIOMETRIC;

            if(enableBioAuth) {
                qDebug() << "enable biometric authenticaion";
                DeviceInfo *device = bioDevices.getDefaultDevice(getUid(userName));
                if(device) {
                    widgetBioAuth->startAuth(getUid(userName), *device);
                    widgetBioAuth->setMoreDevices(bioDevices.count() > 1);
                } else {
                    qDebug() << "No default device, switch to password";
                    emit accept(BIOMETRIC_IGNORE);
                    switchAuthMode(PASSWORD);
                }

            } else {
                /* pass biometric's pam module if there are not available devices */
                qDebug() << "It doesn't meet the condition for enabling biometric authentication, switch to password.";
                emit accept(BIOMETRIC_IGNORE);
                switchAuthMode(PASSWORD);
            }
        }
    }
        break;
    default:
        break;
    }
    switchWidget(mode);
}

/*** end of public memeber ***/


/*** private member ***/

uid_t MainWindow::getUid(const QString &userName)
{
    struct passwd *pwd = getpwnam(userName.toStdString().c_str());
    if(pwd == NULL) {
        qWarning() << "getpwnam error: " << strerror(errno);
        return -1;
    }
    return pwd->pw_uid;
}


void MainWindow::switchWidget(Mode mode)
{
    ui->widgetPasswdAuth->hide();
    ui->btnAuth->hide();
    widgetBioAuth->hide();
    widgetBioDevices->hide();

    switch(mode){
    case PASSWORD:
        setMinimumWidth(420);
        ui->widgetPasswdAuth->show();
        ui->lePassword->setFocus();
        ui->btnAuth->show();
        break;
    case BIOMETRIC:
        setMaximumWidth(380);
        widgetBioAuth->show();
        break;
    case DEVICES:
        widgetBioDevices->show();
    default:
        break;
    }
    adjustSize();
}

/*** end of private member ***/
