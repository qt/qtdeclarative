!wince*:warning("DEPLOYMENT support required. This project only works on WinCE.")

QT += qml network
SOURCES += $$PWD/qmlphotoviewer.cpp
include($$PWD/deployment.pri)
