TARGET = QtQuickParticles

CONFIG += internal_module

QT = core-private gui-private qml-private quick-private
QT_PRIVATE = v8-private

DEFINES   += QT_NO_URL_CAST_FROM_STRING QT_NO_INTEGER_EVENT_COORDINATES
win32-msvc*:DEFINES *= _CRT_SECURE_NO_WARNINGS
solaris-cc*:QMAKE_CXXFLAGS_RELEASE -= -O2

exists("qqml_enable_gcov") {
    QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage -fno-elide-constructors
    LIBS += -lgcov
}

MODULE = quickparticles
load(qt_module)

include(particles.pri)

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
    LIBS -= -lQtQuick
    LIBS -= -lQtQuick_debug
    qtAddLibrary(QtQuick)
}

