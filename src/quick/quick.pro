TARGET = QtQuick

QT = core-private gui-private qml-private
QT_PRIVATE = v8-private network

DEFINES   += QT_NO_URL_CAST_FROM_STRING QT_NO_INTEGER_EVENT_COORDINATES
win32-msvc*:DEFINES *= _CRT_SECURE_NO_WARNINGS
solaris-cc*:QMAKE_CXXFLAGS_RELEASE -= -O2

exists("qqml_enable_gcov") {
    QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage -fno-elide-constructors
    LIBS += -lgcov
}

QMAKE_DOCS = $$PWD/doc/qtquick.qdocconf

ANDROID_LIB_DEPENDENCIES = \
    lib/libQt5QuickParticles.so
ANDROID_LIB_DEPENDENCY_REPLACEMENTS = \
    "plugins/platforms/android/libqtforandroid.so:plugins/platforms/android/libqtforandroidGL.so"
MODULE_PLUGIN_TYPES = \
    accessible
ANDROID_BUNDLED_FILES += \
    qml \
    lib/libQt5QuickParticles.so

load(qt_module)

include(util/util.pri)
include(scenegraph/scenegraph.pri)
include(items/items.pri)
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
    # if QtQuick is qmaked before QtQml is built and frameworks are
    # in use. qtAddLibrary() contains correct logic to deal with this, so
    # we'll explicitly call that for now.
    load(qt)
    LIBS -= -lQtQml        # in non-framework builds, these should be re-added
    LIBS -= -lQtQml_debug  # within the qtAddLibrary if appropriate, so no
    qtAddLibrary(QtQml)    # harm done :)
}

