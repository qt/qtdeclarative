TARGET = QtQuick

QT = core-private gui-private qml-private qmlmodels-private
qtConfig(qml-network): \
    QT_PRIVATE += network

### To support compatibility API that is only available when using RHI/OpenGL
qtConfig(opengl) {
    QT_PRIVATE += opengl-private
    QT += opengl
}

TRACEPOINT_PROVIDER = $$PWD/qtquick.tracepoints
CONFIG += qt_tracepoints

DEFINES   += QT_NO_URL_CAST_FROM_STRING QT_NO_INTEGER_EVENT_COORDINATES
msvc:DEFINES *= _CRT_SECURE_NO_WARNINGS
solaris-cc*:QMAKE_CXXFLAGS_RELEASE -= -O2
win32: LIBS += -luser32

DEFINES += QT_NO_FOREACH

exists("qqml_enable_gcov") {
    QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage -fno-elide-constructors
    LIBS_PRIVATE += -lgcov
}

QMAKE_DOCS = $$PWD/doc/qtquick.qdocconf

MODULE_PLUGIN_TYPES += \
    scenegraph
ANDROID_BUNDLED_FILES += \
    qml

include(util/util.pri)
include(scenegraph/scenegraph.pri)
include(items/items.pri)
include(handlers/handlers.pri)
qtConfig(quick-designer): \
    include(designer/designer.pri)
qtConfig(accessibility) {
    include(accessible/accessible.pri)
}

HEADERS += \
    qtquickglobal.h \
    qtquickglobal_p.h

SOURCES += qtquick2.cpp

# To make #include "qquickcontext2d_jsclass.cpp" work
INCLUDEPATH += $$PWD

load(qt_module)

QMLTYPES_FILENAME = plugins.qmltypes
QMLTYPES_INSTALL_DIR = $$[QT_INSTALL_QML]/QtQuick
QML_IMPORT_NAME = QtQuick
QML_IMPORT_VERSION = $$QT_VERSION
CONFIG += qmltypes install_qmltypes install_metatypes

# Install QtQuick.Window qmldir
qmldir.files = $$PWD/../imports/window/qmldir
qmldir.path = $$[QT_INSTALL_QML]/QtQuick/Window
prefix_build: INSTALLS += qmldir
else: COPIES += qmldir
