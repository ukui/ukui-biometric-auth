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

#include "generic.h"

#include <QDateTime>
#include <QByteArray>

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)

    if(!enableDebug)
        return;

    QDateTime dateTime = QDateTime::currentDateTime();
    QByteArray time = QString("[%1] ").arg(dateTime.toString("MM-dd hh:mm:ss.zzz")).toLocal8Bit();
    QByteArray localMsg = msg.toLocal8Bit();
    QByteArray prefix = logPrefix.toLocal8Bit();
    switch(type) {
    case QtDebugMsg:
        fprintf(stderr, "%s %s [Debug]: %s\n", prefix.constData(),
                time.constData(), localMsg.constData());
        break;
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    case QtInfoMsg:
        fprintf(stderr, "%s %s [Info]: %s\n", prefix.constData(),
                time.constData(), localMsg.constData());
        break;
#endif
    case QtWarningMsg:
        fprintf(stderr, "%s %s [Warnning]: %s\n", prefix.constData(),
                time.constData(), localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "%s %s [Critical]: %s\n", prefix.constData(),
                time.constData(), localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s %s [Fatal]: %s\n", prefix.constData(),
                time.constData(), localMsg.constData());
        abort();
    }
}
