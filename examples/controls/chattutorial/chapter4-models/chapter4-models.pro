TEMPLATE = app

QT += qml quick sql
CONFIG += c++11

!contains(sql-drivers, sqlite): QTPLUGIN += qsqlite

HEADERS += sqlcontactmodel.h \
    sqlconversationmodel.h

SOURCES += main.cpp \
    sqlcontactmodel.cpp \
    sqlconversationmodel.cpp

RESOURCES += \
    qml.qrc \
    ../shared/shared.qrc

include(deployment.pri)

