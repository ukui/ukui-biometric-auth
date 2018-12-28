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
//#include "mainwindow.h"
#include <QCoreApplication>
#include <QTranslator>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include <pwd.h>

#include "generic.h"
#include "keywatcher.h"
#include "bioauth.h"
#include "biodevices.h"

#define START_COLOR     "\033[1;31m"
#define RESULT_COLOR    "\033[1;31m"
#define PROMPT_COLOR    "\033[33m"
#define NOTIFY_COLOR    //"\033[37m"
#define QUESTION_COLOR  //"\033[1;37m"
#define RESET_COLOR     "\033[0m"

bool enableDebug;
QString logPrefix;

enum MsgType
{
    START,
    PROMPT,
    NOTIFY,
    QUESTION,
    RESULT
};

enum _Option
{
    OPTION_TRY_AGAIN,
    OPTION_SELECT_DEVICE,
    OPTION_CANCEL,
    OPTION_UNDEFINED
};
typedef enum _Option Option;

static QString lastMessage;


void showMessage(const QString &message, int type)
{
    // filter out the same message
    if(message == lastMessage)
        return;

    lastMessage = message;

    char *text = message.toLocal8Bit().data();

    switch(type) {
    case START:
        fprintf(stdout, START_COLOR "=== %s ===\n", text);
        break;
    case PROMPT:
        fprintf(stdout, PROMPT_COLOR "%s\n", text);
        break;
    case NOTIFY:
        fprintf(stdout, NOTIFY_COLOR "%s\n", text);
        break;
    case QUESTION:
        fprintf(stdout, QUESTION_COLOR "%s", text);
        break;
    case RESULT:
        fprintf(stdout, RESULT_COLOR "=== %s ===\n", text);
        break;
    default:
        fprintf(stdout, "%s\n", text);
        break;
    }
    fprintf(stdout, RESET_COLOR);
}

Option showOption(bool showSelectDevices)
{
    QStringList optionList;
    QString message;

    optionList << QObject::tr("Try it again");
    if(showSelectDevices)
        optionList << QObject::tr("Use other Devices");
    optionList << QObject::tr("Cancel");

    for(int i = 0; i < optionList.size(); i++){
        message = QString(" %1. %2\n")
                .arg(QString::number(i+1))
                .arg(optionList.at(i));
        showMessage(message, QUESTION);
    }
    message = QString("%1 (1-%2):")
            .arg(QObject::tr("Choose the option"))
            .arg(QString::number(optionList.size()));
    showMessage(message, QUESTION);

    char line[1024];
    fgets(line, sizeof(line), stdin);

    if(line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';

    if(!strcmp(line, "1"))
        return OPTION_TRY_AGAIN;
    else if(!strcmp(line, "2"))
    {
        if(showSelectDevices)
            return OPTION_SELECT_DEVICE;
        else
            return OPTION_CANCEL;
    }
    else if(showSelectDevices && !strcmp(line, "3"))
        return OPTION_CANCEL;
    else {
        showMessage(QObject::tr("Invaild response \"") + line + "\"", NOTIFY);
        showMessage(QObject::tr("AUTHENTICATION CANCELED"), START);
        exit(BIO_FAILED);
    }
    return OPTION_UNDEFINED;
}

DeviceInfo showDevices(const QMap<int, QList<DeviceInfo>> &devicesMap)
{
    int count = 0;
    QString message;

    showMessage(QObject::tr("=== BIOMETRIC DEVICES ==="), PROMPT);
    for(auto type : devicesMap.keys()) {
        for(auto deviceInfo : devicesMap[type]) {
            message = QString(" %1. [%2] %3")
                    .arg(QString::number(++count))
                    .arg(BioDevices::bioTypeToString_tr(type))
                    .arg(deviceInfo.device_shortname);
            showMessage(message, NOTIFY);
        }
    }
    message = QString("%1 (1-%2):")
            .arg(QObject::tr("Choose the device"))
            .arg(QString::number(count));
    showMessage(message, QUESTION);

    char line[1024];
    fgets(line, sizeof(line), stdin);

    if(line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';

    for(auto ch : QString(line)) {
        if(!ch.isDigit()){
            showMessage(QObject::tr("Invaild response \"") + line + "\"", NOTIFY);
            showMessage(QObject::tr("AUTHENTICATION CANCELED"), START);
            exit(BIO_FAILED);
        }
    }

    int deviceIndex = QString(line).toInt();

    if(deviceIndex > count) {
        showMessage(QObject::tr("Invaild response \"") + line + "\"", NOTIFY);
        showMessage(QObject::tr("AUTHENTICATION CANCELED"), START);
        exit(BIO_FAILED);
    }

    int i = 0;
    for(auto type : devicesMap.keys()) {
        int size = devicesMap[type].size();
        if(i + size >= deviceIndex) {
            return devicesMap[type][deviceIndex - i - 1];
        }
        i += size;
    }
    return DeviceInfo();
}

int main(int argc, char *argv[])
{
    QCoreApplication::setSetuidAllowed(true);
    QCoreApplication a(argc, argv);

    QString locale = QLocale::system().name();
    QTranslator translator, translator_bin;
    QString qmfile = QString("%1/i18n_qm/%2.qm").arg(GET_STR(UKUI_BIOMETRIC)).arg(locale);
    QString qmfile_bin = QString("%1/i18n_qm/bioauth-bin/%2.qm").arg(GET_STR(UKUI_BIOMETRIC)).arg(locale);
    qDebug() << "load translation file " << qmfile << qmfile_bin;
    translator.load(qmfile);
    translator_bin.load((qmfile_bin));
    a.installTranslator(&translator);
    a.installTranslator(&translator_bin);

    QCommandLineParser parser;
    QCommandLineOption serviceOption({"s", "service"}, QObject::tr("Sevice Name"));
    QCommandLineOption displayOption({"x", "display"}, QObject::tr("DISPLAY env"), "display", ":0");
    QCommandLineOption usernameOption({"u", "user"}, QObject::tr("User"), "user", "");
    QCommandLineOption debugOption({"d", "debug"}, QObject::tr("Display debug information"));
    QCommandLineOption deviceOption({"e", "device"}, QObject::tr("Device short name"), "");

    parser.addOptions({serviceOption, displayOption, usernameOption, debugOption, deviceOption});
    parser.process(a);

    if(parser.isSet(debugOption))
        enableDebug = true;
    else
        enableDebug = false;

    logPrefix = "[pam-diaglog]:";
    qInstallMessageHandler(outputMessage);

    QString userName = parser.value(usernameOption);
    if(userName.isEmpty())
        exit(BIO_ERROR);
    qDebug() << "authentication user: " << userName;

    uid_t uid;
    struct passwd *pwd = getpwnam(userName.toLocal8Bit().data());
    if(pwd)
        uid = pwd->pw_uid;
    else
        exit(BIO_ERROR);

    BioDevices bioDevices;
    DeviceInfo *deviceInfo = bioDevices.getDefaultDevice(uid);
    if(!deviceInfo)
        exit(BIO_ERROR);

    showMessage(QObject::tr("BIOMETRIC AUTHENTICATION"), START);
    showMessage(QObject::tr("Press Q or Esc to cancel"), PROMPT);

    BioAuth bioAuth(uid, *deviceInfo);
    KeyWatcher watcher;

    QObject::connect(&bioAuth, &BioAuth::notify, &a, [&](const QString &msg){
        showMessage(msg, NOTIFY);
    });
    QObject::connect(&bioAuth, &BioAuth::authComplete, &a, [&](uid_t uid_, int result){

        watcher.stop();
        if(result && uid == uid_) {
            showMessage(QObject::tr("AUTHENTICATION SUCCESS"), RESULT);
            exit(BIO_SUCCESS);
        }
        else {
            showMessage(QObject::tr("AUTHENTICATION FAILED"), RESULT);

            Option option = showOption(bioDevices.count() > 1);
            switch(option) {
            case OPTION_TRY_AGAIN:
                bioAuth.startAuth();
                watcher.start();
                break;
            case OPTION_SELECT_DEVICE:
            {
                DeviceInfo deviceInfo = showDevices(bioDevices.getAllDevices());
                bioAuth.setDevice(deviceInfo);
                bioAuth.startAuth();
                watcher.start();
                break;
            }
            case OPTION_CANCEL:
                showMessage(QObject::tr("BIOMETRIC AUTHENTICATION END"), START);
                exit(BIO_FAILED);
                break;
            default:
                break;
            }
        }
    });
    bioAuth.startAuth();


    QObject::connect(&watcher, &KeyWatcher::exit, &a, [&]{
        showMessage(QObject::tr("AUTHENTICATION CANCELED"), START);
        bioAuth.stopAuth();
        exit(BIO_ERROR);
    });
    watcher.start();

    return a.exec();
}
