load(qt_module)

TARGET = qmldbg_inspector
QT       += declarative-private core-private gui-private opengl-private qtquick1 widgets widgets-private v8-private

load(qt_plugin)

DESTDIR  = $$QT.declarative.plugins/qmltooling
QTDIR_build:REQUIRES += "contains(QT_CONFIG, declarative)"

INCLUDEPATH *= $$PWD

SOURCES += \
    abstractviewinspector.cpp \
    qdeclarativeinspectorplugin.cpp \
    qtquick1/qdeclarativeviewinspector.cpp \
    qtquick1/abstractliveedittool.cpp \
    qtquick1/liveselectiontool.cpp \
    qtquick1/livelayeritem.cpp \
    qtquick1/livesingleselectionmanipulator.cpp \
    qtquick1/liverubberbandselectionmanipulator.cpp \
    qtquick1/liveselectionrectangle.cpp \
    qtquick1/liveselectionindicator.cpp \
    qtquick1/boundingrecthighlighter.cpp \
    qtquick1/subcomponentmasklayeritem.cpp \
    qtquick1/zoomtool.cpp \
    qtquick1/colorpickertool.cpp \
    abstracttool.cpp \
    sgviewinspector.cpp \
    sgselectiontool.cpp \
    sghighlight.cpp

HEADERS += \
    abstractviewinspector.h \
    qdeclarativeinspectorplugin.h \
    qdeclarativeinspectorprotocol.h \
    qmlinspectorconstants.h \
    qtquick1/qdeclarativeviewinspector.h \
    qtquick1/qdeclarativeviewinspector_p.h \
    qtquick1/abstractliveedittool.h \
    qtquick1/liveselectiontool.h \
    qtquick1/livelayeritem.h \
    qtquick1/livesingleselectionmanipulator.h \
    qtquick1/liverubberbandselectionmanipulator.h \
    qtquick1/liveselectionrectangle.h \
    qtquick1/liveselectionindicator.h \
    qtquick1/boundingrecthighlighter.h \
    qtquick1/subcomponentmasklayeritem.h \
    qtquick1/zoomtool.h \
    qtquick1/colorpickertool.h \
    abstracttool.h \
    sgviewinspector.h \
    sgselectiontool.h \
    sghighlight.h

target.path += $$[QT_INSTALL_PLUGINS]/qmltooling
INSTALLS += target

symbian:TARGET.UID3=0x20031E90
