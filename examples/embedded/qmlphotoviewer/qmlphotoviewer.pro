!wince*:warning("DEPLOYMENT support required. This project only works on WinCE.")

QT += declarative network
SOURCES += $$PWD/qmlphotoviewer.cpp
include($$PWD/deployment.pri)
