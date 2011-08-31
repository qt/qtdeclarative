load(qt_module)

TARGET     = QtQuick1
QPRO_PWD   = $$PWD

CONFIG += module
CONFIG += dll warn_on
MODULE_PRI += ../../modules/qt_qtquick1.pri

QT += testlib-private declarative testlib declarative-private core-private gui-private network widgets-private v8-private
DEFINES += QT_NO_URL_CAST_FROM_STRING

load(qt_module_config)

# Install qtquick1.prf into the Qt mkspecs so that "CONFIG += qtquick1"
# can be used in customer applications to build against QtQuick 1.
feature.path = $$[QT_INSTALL_DATA]/mkspecs/features
feature.files = $$PWD/features/qtquick1.prf
INSTALLS += feature

symbian {
    DEFINES += QT_MAKEDLL
    CONFIG += epocallowdlldata
    contains(QT_EDITION, OpenSource) {
        TARGET.CAPABILITY = LocalServices NetworkServices ReadUserData UserEnvironment WriteUserData
    } else {
        TARGET.CAPABILITY = All -Tcb
    }
}

#INCLUDEPATH += $$PWD/QtQuick1
#INCLUDEPATH += $$PWD

include(util/util.pri)
include(graphicsitems/graphicsitems.pri)

HEADERS += qtquick1_p.h
SOURCES += qtquick1.cpp

DEFINES += QT_NO_OPENTYPE
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

