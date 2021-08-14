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
#include <PolkitQt1/Agent/Listener>
#include <PolkitQt1/Agent/Session>
#include <PolkitQt1/Subject>
#include <PolkitQt1/Identity>
#include <PolkitQt1/Details>
#include <PolkitQt1/Authority>
#include <PolkitQt1/ActionDescription>
#include <qdialogbuttonbox.h>

#include <QPoint>
#include <QCursor>
#include <QDebug>
#include <QDesktopWidget>
#include <QApplication>
#include <QScreen>
#include <fcntl.h>

#include "PolkitListener.h"
#include "mainwindow.h"
#include "generic.h"


PolkitListener::PolkitListener(QObject *parent)
    : Listener(parent),
      inProgress(false),
      currentIdentity(0),
      mainWindow(nullptr)
{
}

PolkitListener::~PolkitListener()
{
}

/* initiateAuthentication message from polkit */
void PolkitListener::initiateAuthentication(
    const QString &actionId, const QString &message,
    const QString &iconName, const PolkitQt1::Details &details,
    const QString &cookie, const PolkitQt1::Identity::List &identities,
    PolkitQt1::Agent::AsyncResult *result)
{
    if(inProgress){
        result->setError(tr("Another client is already authenticating, please try again later."));
        result->setCompleted();
        qDebug() << "Another client is already authenticating, please try again later.";
        return;
    }

    QStringList usersList;
    QString subjectPid, callerPid;
    PolkitQt1::ActionDescription actionDesc;

    QString username = getenv("USER");
    for(auto identity : identities){
        if(identity.toString().remove("unix-user:") == username)
            usersList.prepend(identity.toString().remove("unix-user:"));
        else
            usersList.append(identity.toString().remove("unix-user:"));
    }

    subjectPid = details.lookup("polkit.subject-pid");
    callerPid = details.lookup("polkit.caller-pid");

    /* find action description for actionId */
    foreach(const PolkitQt1::ActionDescription &desc,
            PolkitQt1::Authority::instance()->enumerateActionsSync()) {
        if (actionId == desc.actionId()) {
            actionDesc = desc;
            qDebug() << "Action description has been found" ;
            break;
        }
    }

    qDebug() << "Initiating authentication";
    qDebug() << "icon name: " << iconName;
    qDebug() << "identities: " << usersList;
    qDebug() << "message: " << message;
    qDebug() << "action id: " << actionId;
    qDebug() << "polkit.subject-pid" << subjectPid;
    qDebug() << "polkit.caller-pid" << callerPid;
    qDebug() << "action: " << actionDesc.description();
    qDebug() << "Vendor:" << actionDesc.vendorName() << actionDesc.vendorUrl();

    this->inProgress = true;

    this->identities = identities;
    this->cookie = cookie;
    this->result = result;
    session.clear();
    if (identities.length() == 1) {
    	this->currentIdentity = identities[0];
    } else {
        currentIdentity = identities[0];
    }

    /* Create the polkit window */

    mainWindow = new MainWindow;
    mainWindow->setWindowFlags(Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);
    mainWindow->setIcon(iconName);
    mainWindow->setHeader(message);
    mainWindow->setUsers(usersList);
    /*
    mainWindow->setDetails(subjectPid, callerPid,
                           actionDesc.actionId(),
                           actionDesc.description(),
                           actionDesc.vendorName(),
                           actionDesc.vendorUrl());
    */
    /* set the position of the mainwindow */
    QPoint pos = QCursor::pos();
    for(auto screen : QGuiApplication::screens())
    {
        if(screen->geometry().contains(pos))
        {
            mainWindow->move(screen->geometry().left() + (screen->geometry().width() - mainWindow->width()) / 2,
                             screen->geometry().top() + (screen->geometry().height() - mainWindow->height()) / 2);
        }
    }

    connect(mainWindow, &MainWindow::accept, this, &PolkitListener::onResponse);
    connect(mainWindow, &MainWindow::canceled, this, [&]{
        wasCancelled = true;
        if(!session.isNull()) {
            session.data()->cancel();
        }
    });

    connect(mainWindow, &MainWindow::switchToBiometric, this, [&]{
        wasSwitchToBiometric = true;
        startAuthentication();
    });

    connect(mainWindow, &MainWindow::userChanged, this, [&](const QString &userName){
        for(int i = 0; i < this->identities.size(); i++) {
            auto identity = this->identities.at(i);
            if(identity.toString().remove("unix-user:") == userName) {
                this->currentIdentity = this->identities.at(i);
                numTries = 0;
                startAuthentication();
                break;
            }
        }
    });

    numTries = 0;
    wasCancelled = false;
    wasSwitchToBiometric = false;

    mainWindow->userChanged(usersList.at(0));

}

static
int get_pam_tally(int *deny, int *unlock_time)
{
    char buf[128];
    FILE *auth_file;

    if( (auth_file = fopen("/etc/pam.d/common-auth", "r")) == NULL)
        return -1;

    while(fgets(buf, sizeof(buf), auth_file)) {
        if(strlen(buf) == 0 || buf[0] == '#')
            continue;
        if(!strstr(buf, "deny"))
            continue;

        char *ptr = strtok(buf, " \t");
        while(ptr) {
            if(strncmp(ptr, "deny=", 5)==0){
                sscanf(ptr, "deny=%d", deny);
                //gs_debug("-------------------- deny=%d", *deny);
            }
            if(strncmp(ptr, "unlock_time=", 12)==0){
                sscanf(ptr, "unlock_time=%d", unlock_time);
                //gs_debug("-------------------- unlock_time=%d", *unlock_time);
            }
            ptr = strtok(NULL, " \t");
        }
        return 1;
    }
    return 0;
}

void PolkitListener::finishObtainPrivilege()
{
    /* Number of tries increase only when some user is selected */
    if (currentIdentity.isValid()) {
    	numTries++;
    }
    qDebug().noquote() << QString("Finishing obtaining "
                    "privileges (G:%1, C:%2, D:%3).")
                    .arg(gainedAuthorization)
                    .arg(wasCancelled)
                    .arg(mainWindow != NULL);

    if (!gainedAuthorization && !wasCancelled && (mainWindow != NULL)) {
         int deny = 0, unlock_time = 0;
         mainWindow->stopDoubleAuth();
         if(!get_pam_tally(&deny, &unlock_time)||(deny  == 0 &&unlock_time == 0)) {
             if(!wasSwitchToBiometric){
                 mainWindow->setAuthResult(gainedAuthorization, tr("Authentication failure, please try again."));
             }
             startAuthentication();
             return;
         }
         else {
             startAuthentication();
             return;
         }
        
        startAuthentication();
            return;
    }

    if (mainWindow) {
        mainWindow->hide();
        mainWindow->stopDoubleAuth();
        mainWindow->deleteLater();
        mainWindow = NULL;
    }

    if (!session.isNull()) {
        session.data()->result()->setCompleted();
    } else {
        result->setCompleted();
    }
    session.data()->deleteLater();

    this->inProgress = false;
    qDebug() << "Finish obtain authorization:" << gainedAuthorization;
}

void establishToBioPAM()
{
    FILE *file;
    const char *data = "polkit-ukui-authentication-agent-1";

    if( (file = fopen(BIO_COM_FILE, "w")) == NULL){
        qWarning() << "open communication file failed: " << strerror(errno);
        return;
    }

    if(fputs(data, file) == EOF) {
        qWarning() << "write to communication file error: " << strerror(errno);
    }
    fclose(file);
}

void PolkitListener::startAuthentication()
{
    qDebug() << "start authenticate user " << currentIdentity.toString();
    if(!session.isNull()) {
        session.data()->deleteLater();
    }

    /* We will create a new session only when some user is selected */
    if (currentIdentity.isValid()) {

        establishToBioPAM();

    	session = new Session(currentIdentity, cookie, result);
    	connect(session.data(), SIGNAL(request(QString, bool)), this,
                SLOT(onShowPrompt(QString,bool)));
    	connect(session.data(), SIGNAL(completed(bool)), this,
                SLOT(onAuthCompleted(bool)));
    	connect(session.data(), SIGNAL(showError(QString)), this,
                SLOT(onShowError(QString)));
        connect(session.data(), SIGNAL(showInfo(QString)), this,
                SLOT(onShowError(QString)));
    	session.data()->initiate();
    }

    mainWindow->clearEdit();
}

void PolkitListener::onShowPrompt(const QString &prompt, bool echo)
{
    qDebug() << "Prompt: " << prompt << "echo: " << echo;

    if(prompt == BIOMETRIC_PAM) {
        mainWindow->setDoubleAuth(false);
        mainWindow->switchAuthMode(MainWindow::BIOMETRIC);
    }else if(prompt == BIOMETRIC_PAM_DOUBLE){
        mainWindow->setDoubleAuth(true);
        mainWindow->switchAuthMode(MainWindow::BIOMETRIC);
        //这时候不需要显示授权弹窗，下一次收到prompt请求的时候再显示
        return;
    }
    else {
        mainWindow->switchAuthMode(MainWindow::PASSWORD);
        mainWindow->setPrompt(prompt, echo);
    }

    mainWindow->show();
    QPoint pos = QCursor::pos();
    for(auto screen : QGuiApplication::screens())
    {
        if(screen->geometry().contains(pos))
        {
            mainWindow->move(screen->geometry().left() + (screen->geometry().width() - mainWindow->width()) / 2,
                             screen->geometry().top() + (screen->geometry().height() - mainWindow->height()) / 2);
        }
    }

    mainWindow->activateWindow();
}

//目前返回的pam错误就onShowError，onShowInfo两类
void PolkitListener::onShowError(const QString &text)
{
    qDebug() << "[Polkit]:"    << "Error:" << text;

    if(mainWindow){
        QString strText = mainWindow->check_is_pam_message(text);
        mainWindow->setMessage(strText);
    }
}

void PolkitListener::onShowInfo(const QString &text)
{
    qDebug() << "[Polkit]:"    << "Info:" << text;
    if(mainWindow){
        QString strText = mainWindow->check_is_pam_message(text);
        mainWindow->setMessage(strText);
    }
}

void PolkitListener::onResponse(const QString &text)
{
    session.data()->setResponse(text);
}

void PolkitListener::onAuthCompleted(bool gainedAuthorization)
{
    qDebug() << "completed: " << gainedAuthorization;
    this->gainedAuthorization = gainedAuthorization;
    finishObtainPrivilege();
}

bool PolkitListener::initiateAuthenticationFinish()
{
    qDebug() << "initiateAuthenticationFinish.";
    return true;
}

void PolkitListener::cancelAuthentication()
{
    wasCancelled = true;
    qDebug() << "cancelAuthentication.";
    finishObtainPrivilege();
}
