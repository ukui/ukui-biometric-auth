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

#include <QApplication>
#include <QTranslator>
#include <QDebug>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <PolkitQt1/Subject>
#include "PolkitListener.h"
#include "generic.h"

bool enableDebug;
QString logPrefix;

#define SM_DBUS_SERVICE "org.gnome.SessionManager"
#define SM_DBUS_PATH "/org/gnome/SessionManager"
#define SM_DBUS_INTERFACE "org.gnome.SessionManager"

bool registerToGnomeSession()
{
    QDBusInterface interface(SM_DBUS_SERVICE,
                             SM_DBUS_PATH,
                             SM_DBUS_INTERFACE,
                             QDBusConnection::sessionBus());
    QString appId("polkit-ukui-authentication-agent-1.desktop");
    QString clientStartupId(qgetenv("DESKTOP_AUTOSTART_ID"));

    QDBusReply<QDBusObjectPath> reply = interface.call("RegisterClient",
                                                       appId, clientStartupId);

    if(!reply.isValid()) {
        qWarning() << "Register Client to gnome session failed";
        return false;
    }
    qDebug() << "Register Client to gnome session: " << reply.value().path();
    return true;
}

int main(int argc, char *argv[])
{
    enableDebug = true;
    logPrefix = "[ukui-polkit]:";
    qInstallMessageHandler(outputMessage);

	qDebug() << "Polkit Agent Started";

	QApplication agent(argc, argv);

    QString locale = QLocale::system().name();
    qDebug() << "Language: " <<locale;
    QTranslator translator_main, translator_bio;
    QString qmfile_main = QString("%1/i18n_qm/%2.qm").arg(GET_STR(INSTALL_PATH)).arg(locale);
    QString qmfile_bio = QString("%1/i18n_qm/%2.qm").arg(GET_STR(UKUI_BIOMETRIC)).arg(locale);
    qDebug() << "load " << qmfile_main;
    qDebug() << "load " << qmfile_bio;

    translator_main.load(qmfile_main);
    translator_bio.load(qmfile_bio);
    agent.installTranslator(&translator_main);
    agent.installTranslator(&translator_bio);

    /* Run forever */
    agent.setQuitOnLastWindowClosed(false);
    PolkitListener listener;
    PolkitQt1::UnixSessionSubject session(getpid());
    if (listener.registerListener(session, POLKIT_LISTENER_ID)) {
        qDebug() << "Listener" << POLKIT_LISTENER_ID << "registered.";
    } else {
        qDebug() << "Could not register listener"
                    << POLKIT_LISTENER_ID << "Aborting";
        return EXIT_FAILURE;
    }

    registerToGnomeSession();

	agent.exec();
	return EXIT_SUCCESS;
}
