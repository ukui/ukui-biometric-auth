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
#ifndef POLKITLISTENER_H
#define POLKITLISTENER_H

#include <PolkitQt1/Agent/Listener>
#include <QWeakPointer>

#include "mainwindow.h"

using namespace PolkitQt1::Agent;

class PolkitListener : public Listener
{
	Q_OBJECT
public:
	PolkitListener(QObject *parent = 0);
	virtual ~PolkitListener();

public slots:
	void initiateAuthentication(const QString &actionId,
				    const QString &message,
				    const QString &iconName,
				    const PolkitQt1::Details &details,
				    const QString &cookie,
				    const PolkitQt1::Identity::List &identities,
				    PolkitQt1::Agent::AsyncResult *result);
	bool initiateAuthenticationFinish();
	void cancelAuthentication();
	void finishObtainPrivilege();


private:
	bool gainedAuthorization;
	bool wasCancelled;
    bool wasSwitchToBiometric;
    bool inProgress;
	int numTries;
	QWeakPointer<Session> session;
	PolkitQt1::Identity::List identities;
	PolkitQt1::Identity currentIdentity;
	PolkitQt1::Agent::AsyncResult *result;
	QString cookie;
    MainWindow *mainWindow;

private slots:

    void startAuthentication();
    void onShowPrompt(const QString &prompt, bool echo);
    void onShowError(const QString &text);
    void onShowInfo(const QString &text);
    void onResponse(const QString &text);
    void onAuthCompleted(bool);
};
#endif /* POLKITLISTENER_H */
