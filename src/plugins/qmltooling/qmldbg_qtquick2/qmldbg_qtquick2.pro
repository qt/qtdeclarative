load(qt_module)

TARGET = qmldbg_qtquick2
QT       += declarative-private core-private gui-private opengl-private v8-private

load(qt_plugin)

DESTDIR  = $$QT.declarative.plugins/qmltooling

INCLUDEPATH *= $$PWD $$PWD/../shared

SOURCES += \
    qtquick2plugin.cpp \
    sghighlight.cpp \
    sgselectiontool.cpp \
    sgviewinspector.cpp \
    ../shared/abstracttool.cpp \
    ../shared/abstractviewinspector.cpp

HEADERS += \
    qtquick2plugin.h \
    sghighlight.h \
    sgselectiontool.h \
    sgviewinspector.h \
    ../shared/abstracttool.h \
    ../shared/abstractviewinspector.h \
    ../shared/qdeclarativeinspectorprotocol.h \
    ../shared/qmlinspectorconstants.h

target.path += $$[QT_INSTALL_PLUGINS]/qmltooling
INSTALLS += target
