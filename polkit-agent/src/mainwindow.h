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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QLabel>
#include "biometric.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);

    enum Mode{UNDEFINED, PASSWORD, BIOMETRIC, DEVICES};

    void setIcon(const QString &iconName);
    void setHeader(const QString &text);
    void setUsers(const QStringList &usersList);
    void setDetails(const QString &subPid, const QString &callerPid,
                    const QString &actionId, const QString &actionDesc,
                    const QString vendorName, const QString vendorUrl);
    void setPrompt(const QString &text, bool echo);
    void setMessage(const QString &text);
    void setAuthResult(bool result, const QString &text="");
    void clearEdit();
    void switchAuthMode(Mode mode);

private:    
    uid_t getUid(const QString &userName);
    void setMovie();
    void setPixmap();
    void startBioAuth();
    void switchWidget(Mode mode);

private slots:
    void on_btnDetails_clicked();
    void on_lePassword_returnPressed();
    void on_btnBioAuth_clicked();
    void on_cmbUsers_currentTextChanged(const QString &userName);
    void on_btnCancel_clicked();
    void on_btnAuth_clicked();

signals:
    void accept(const QString &text);
    void canceled();
    void switchToPassword();
    void switchToBiometric();
    void userChanged(const QString &userName);

private:
    Ui::MainWindow *ui;
    QString userName;
    bool enableBioAuth;
    bool receiveBioPAM;
    BioDevices bioDevices;
    Mode authMode;
    BioDevicesWidget *widgetBioDevices;
    BioAuthWidget *widgetBioAuth;
};

#endif // MAINWINDOW_H
