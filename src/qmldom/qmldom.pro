option(host_build)
TARGET     = QtQmlDom
QT         = core-private qmldevtools-private
CONFIG    += minimal_syncqt internal_module generated_privates

DEFINES += QMLDOM_LIBRARY

SOURCES += \
    $$PWD/qqmldomerrormessage.cpp \
    $$PWD/qqmldomitem.cpp \
    $$PWD/qqmldompath.cpp \
    $$PWD/qqmldomstringdumper.cpp

HEADERS += \
    $$PWD/qqmldom_global.h \
    $$PWD/qqmldomconstants_p.h \
    $$PWD/qqmldomerrormessage_p.h \
    $$PWD/qqmldomitem_p.h \
    $$PWD/qqmldompath_p.h \
    $$PWD/qqmldomstringdumper_p.h

load(qt_module)
