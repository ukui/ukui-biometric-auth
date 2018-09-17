QT       += core gui dbus
CONFIG   += c++11 link_pkgconfig
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = polkit-ukui-authentication-agent-1
TEMPLATE = app

CONFIG += debug


PREFIX = ${UKUI_BIOMETRIC}/ukui-polkit-agent

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

DEFINES += INSTALL_PATH=$${PREFIX} \
           UKUI_BIOMETRIC=${UKUI_BIOMETRIC}

INCLUDEPATH +=  $$PWD/../common/ \
                $$PWD/../bioAuthentication/include/

VPATH += $$PWD/../common/

LIBS +=  -lpolkit-qt5-core-1 \
        -L$$PWD/../bioAuthentication -lbioAuthentication


PKGCONFIG += polkit-qt5-agent-1

HEADERS += \
    src/mainwindow.h \
    src/PolkitListener.h

FORMS += \
    src/mainwindow.ui

SOURCES += \
    src/PolkitAgent.cpp \
    src/mainwindow.cpp \
    src/PolkitListener.cpp \
    generic.cpp

RESOURCES += \
    assets.qrc

DISTFILES += \
    src/main.qss


TRANSLATIONS += i18n_ts/zh_CN.ts

system(lrelease i18n_ts/*.ts)

qm_file.files = i18n_ts/*.qm
qm_file.path = ${DESTDIR_POLKIT}$${PREFIX}/i18n_qm/

desktop_file.files = data/*.desktop
desktop_file.path = ${DESTDIR_POLKIT}/etc/xdg/autostart/

target.path = ${DESTDIR_POLKIT}${LIB_PATH}/ukui-polkit

INSTALLS += qm_file target desktop_file
