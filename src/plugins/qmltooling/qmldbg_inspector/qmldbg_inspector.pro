load(qt_module)

TARGET = qmldbg_inspector
QT       += declarative-private core-private gui-private opengl-private qtquick1 widgets widgets-private

load(qt_plugin)

DESTDIR  = $$QT.declarative.plugins/qmltooling
QTDIR_build:REQUIRES += "contains(QT_CONFIG, declarative)"

SOURCES += \
    abstractviewinspector.cpp \
    qdeclarativeinspectorplugin.cpp \
    qdeclarativeviewinspector.cpp \
    editor/abstractliveedittool.cpp \
    editor/liveselectiontool.cpp \
    editor/livelayeritem.cpp \
    editor/livesingleselectionmanipulator.cpp \
    editor/liverubberbandselectionmanipulator.cpp \
    editor/liveselectionrectangle.cpp \
    editor/liveselectionindicator.cpp \
    editor/boundingrecthighlighter.cpp \
    editor/subcomponentmasklayeritem.cpp \
    editor/zoomtool.cpp \
    editor/colorpickertool.cpp \
    editor/qmltoolbar.cpp \
    editor/toolbarcolorbox.cpp \
    abstracttool.cpp \
    sgviewinspector.cpp \
    sgselectiontool.cpp \
    sghighlight.cpp

HEADERS += \
    abstractviewinspector.h \
    qdeclarativeinspectorplugin.h \
    qdeclarativeinspectorprotocol.h \
    qdeclarativeviewinspector.h \
    qdeclarativeviewinspector_p.h \
    qmlinspectorconstants.h \
    editor/abstractliveedittool.h \
    editor/liveselectiontool.h \
    editor/livelayeritem.h \
    editor/livesingleselectionmanipulator.h \
    editor/liverubberbandselectionmanipulator.h \
    editor/liveselectionrectangle.h \
    editor/liveselectionindicator.h \
    editor/boundingrecthighlighter.h \
    editor/subcomponentmasklayeritem.h \
    editor/zoomtool.h \
    editor/colorpickertool.h \
    editor/qmltoolbar.h \
    editor/toolbarcolorbox.h \
    abstracttool.h \
    sgviewinspector.h \
    sgselectiontool.h \
    sghighlight.h

RESOURCES += editor/editor.qrc

target.path += $$[QT_INSTALL_PLUGINS]/qmltooling
INSTALLS += target

symbian:TARGET.UID3=0x20031E90
