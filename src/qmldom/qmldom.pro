option(host_build)
TARGET     = QtQmlDom
QT         = core-private qmldevtools-private
CONFIG    += minimal_syncqt internal_module generated_privates

DEFINES += QMLDOM_LIBRARY

SOURCES += \
    $$PWD/qqmldomstringdumper.cpp

HEADERS += \
    $$PWD/qqmldom_global.h \
    $$PWD/qqmldomconstants_p.h \
    $$PWD/qqmldomstringdumper_p.h

load(qt_module)
