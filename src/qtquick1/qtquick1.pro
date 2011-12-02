load(qt_module)

TARGET     = QtQuick1
QPRO_PWD   = $$PWD

CONFIG += module
CONFIG += dll warn_on
MODULE_PRI += ../../modules/qt_qtquick1.pri

QT += testlib-private declarative testlib declarative-private core-private gui-private network widgets-private v8-private
DEFINES += QT_BUILD_QTQUICK1_LIB QT_NO_URL_CAST_FROM_STRING

load(qt_module_config)

# Install qtquick1.prf into the Qt mkspecs so that "CONFIG += qtquick1"
# can be used in customer applications to build against QtQuick 1.
feature.path = $$[QT_INSTALL_DATA]/mkspecs/features
feature.files = $$PWD/features/qtquick1.prf
INSTALLS += feature

win32-msvc*:DEFINES *= _CRT_SECURE_NO_WARNINGS

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

mac {
    # FIXME: this is a workaround for broken qmake logic in qtAddModule()
    # This function refuses to use frameworks unless the framework exists on
    # the filesystem at the time qmake is run, resulting in a build failure
    # if QtQuick1 is qmaked before QtDeclarative is built and frameworks are
    # in use. qtAddLibrary() contains correct logic to deal with this, so
    # we'll explicitly call that for now.
    load(qt)
    LIBS -= -lQtDeclarative        # in non-framework builds, these should be re-added
    LIBS -= -lQtDeclarative_debug  # within the qtAddLibrary if appropriate, so no
    qtAddLibrary(QtDeclarative)    # harm done :)
}

