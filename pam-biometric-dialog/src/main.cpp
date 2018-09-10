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
#include <QApplication>
#include <QTranslator>
#include "generic.h"

int main(int argc, char *argv[])
{
    QApplication::setSetuidAllowed(true);
    QApplication a(argc, argv);

    QString locale = QLocale::system().name();
    QTranslator translator;
    QString qmfile = QString("%1/i18n_qm/%2.qm").arg(GET_STR(UKUI_BIOMETRIC)).arg(locale);
    qDebug() << "load translation file " << qmfile;
    translator.load(qmfile);
    a.installTranslator(&translator);

    MainWindow w;
    w.show();

    return a.exec();
}
