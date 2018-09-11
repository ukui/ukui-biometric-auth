#-------------------------------------------------
#
# Project created by QtCreator 2018-09-03T15:02:45
#
#-------------------------------------------------
QT  += core gui dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ukui-pam-biometric-dialog
TEMPLATE = app

CONFIG += debug c++11


# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += UKUI_BIOMETRIC=${UKUI_BIOMETRIC}

INCLUDEPATH += $$PWD/../bioAuthentication/include/ \
                $$PWD/../common/
LIBS += -L$$PWD/../bioAuthentication -lbioAuthentication

SOURCES += \
        src/main.cpp \
        src/mainwindow.cpp

HEADERS += \
        src/mainwindow.h

FORMS += \
        src/mainwindow.ui

RESOURCES += \
    assets.qrc

target.path = ${DESTDIR_PAM}/bin/

INSTALLS += target
