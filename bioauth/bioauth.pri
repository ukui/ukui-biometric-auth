QT += core dbus

greaterThan(QT_MAJOR_VERSION, 4)

HEADERS += \
    $$PWD/include/bioauth.h \
    $$PWD/include/biodevices.h \
    $$PWD/include/biotypes.h

SOURCES += \
    $$PWD/src/bioauth.cpp \
    $$PWD/src/biodevices.cpp \
    $$PWD/src/biotypes.cpp
