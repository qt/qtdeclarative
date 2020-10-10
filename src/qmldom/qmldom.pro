option(host_build)
TARGET     = QtQmlDom
QT         = core-private qmldevtools-private
CONFIG    += minimal_syncqt internal_module generated_privates

DEFINES += QMLDOM_LIBRARY

SOURCES += \
    $$PWD/qqmldomerrormessage.cpp \
    $$PWD/qqmldomexternalitems.cpp \
    $$PWD/qqmldomitem.cpp \
    $$PWD/qqmldompath.cpp \
    $$PWD/qqmldomtop.cpp \
    $$PWD/qqmldomstringdumper.cpp

HEADERS += \
    $$PWD/qqmldom_fwd_p.h \
    $$PWD/qqmldom_global.h \
    $$PWD/qqmldomconstants_p.h \
    $$PWD/qqmldomerrormessage_p.h \
    $$PWD/qqmldomexternalitems_p.h \
    $$PWD/qqmldomitem_p.h \
    $$PWD/qqmldompath_p.h \
    $$PWD/qqmldomtop_p.h \
    $$PWD/qqmldomstringdumper_p.h

load(qt_module)
