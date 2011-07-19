load(qt_module)

TARGET     = QtDeclarative
QPRO_PWD   = $$PWD

CONFIG += module
MODULE_PRI += ../../modules/qt_declarative.pri

QT         = core-private gui-private script-private network script opengl-private
contains(QT_CONFIG, svg): QT += svg
DEFINES   += QT_BUILD_DECLARATIVE_LIB QT_NO_URL_CAST_FROM_STRING
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x66000000
solaris-cc*:QMAKE_CXXFLAGS_RELEASE -= -O2

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore QtGui

exists("qdeclarative_enable_gcov") {
    QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage -fno-elide-constructors
    LIBS += -lgcov
}

load(qt_module_config)

HEADERS += qtdeclarativeversion.h

#INCLUDEPATH -= $$QMAKE_INCDIR_QT/$$TARGET
#DESTDIR=.

#modules
include(util/util.pri)
include(qml/qml.pri)
include(debugger/debugger.pri)
include(scenegraph/scenegraph.pri)
include(items/items.pri)
include(particles/particles.pri)

symbian: {
    TARGET.UID3=0x2001E623
    LIBS += -lefsrv

    contains(QT_CONFIG, freetype) {
        DEFINES += QT_NO_FONTCONFIG
        INCLUDEPATH += \
            ../3rdparty/freetype/src \
            ../3rdparty/freetype/include
    }
}

linux-g++-maemo:DEFINES += QDECLARATIVEVIEW_NOBACKGROUND

DEFINES += QT_NO_OPENTYPE
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src


macx:CONFIG(debug, debug|release) {
    QMAKE_LIBS_PRIVATE += -L../v8/ -lv8_debug
} else {
    QMAKE_LIBS_PRIVATE += -L../v8/ -lv8
}
