load(qt_module)

TARGET = QtQuick

CONFIG += module
CONFIG += dll warn_on
MODULE_PRI = ../../modules/qt_quick.pri

QT = core-private gui gui-private network v8-private declarative declarative-private

DEFINES   += QT_BUILD_QUICK_LIB QT_NO_URL_CAST_FROM_STRING QT_NO_INTEGER_EVENT_COORDINATES
win32-msvc*:DEFINES *= _CRT_SECURE_NO_WARNINGS
solaris-cc*:QMAKE_CXXFLAGS_RELEASE -= -O2

exists("qdeclarative_enable_gcov") {
    QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage -fno-elide-constructors
    LIBS += -lgcov
}

load(qt_module_config)

include(util/util.pri)
include(scenegraph/scenegraph.pri)
include(items/items.pri)
include(particles/particles.pri)
include(designer/designer.pri)

HEADERS += \
    qtquickglobal.h \
    qtquickglobal_p.h \
    qtquick2_p.h

SOURCES += qtquick2.cpp

mac {
    # FIXME: this is a workaround for broken qmake logic in qtAddModule()
    # This function refuses to use frameworks unless the framework exists on
    # the filesystem at the time qmake is run, resulting in a build failure
    # if QtQuick is qmaked before QtDeclarative is built and frameworks are
    # in use. qtAddLibrary() contains correct logic to deal with this, so
    # we'll explicitly call that for now.
    load(qt)
    LIBS -= -lQtDeclarative        # in non-framework builds, these should be re-added
    LIBS -= -lQtDeclarative_debug  # within the qtAddLibrary if appropriate, so no
    qtAddLibrary(QtDeclarative)    # harm done :)
}

