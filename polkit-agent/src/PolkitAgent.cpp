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
#include <PolkitQt1/Subject>
#include "PolkitListener.h"
#include "generic.h"
#include "sessionmanager.h"

bool enableDebug;
QString logPrefix;




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
    QString qmfile_main = QString("%1/i18n_qm/polkit/%2.qm").arg(GET_STR(UKUI_BIOMETRIC)).arg(locale);
    QString qmfile_bio = QString("%1/i18n_qm/%2.qm").arg(GET_STR(UKUI_BIOMETRIC)).arg(locale);
    qDebug() << "load " << qmfile_main;
    qDebug() << "load " << qmfile_bio;

    translator_main.load(qmfile_main);
    translator_bio.load(qmfile_bio);
    agent.installTranslator(&translator_main);
    agent.installTranslator(&translator_bio);

//    SessionManager sm;

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

	agent.exec();
	return EXIT_SUCCESS;
}
