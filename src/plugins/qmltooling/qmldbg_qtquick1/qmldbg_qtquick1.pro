load(qt_module)

TARGET = qmldbg_qtquick1
QT       += declarative-private core-private gui-private widgets-private qtquick1 opengl-private

load(qt_plugin)

DESTDIR  = $$QT.declarative.plugins/qmltooling

INCLUDEPATH *= $$PWD $$PWD/../shared

SOURCES += \
    abstractliveedittool.cpp \
    boundingrecthighlighter.cpp \
    colorpickertool.cpp \
    livelayeritem.cpp \
    liverubberbandselectionmanipulator.cpp \
    liveselectionindicator.cpp \
    liveselectionrectangle.cpp \
    liveselectiontool.cpp \
    livesingleselectionmanipulator.cpp \
    qdeclarativeviewinspector.cpp \
    qtquick1plugin.cpp \
    ../shared/abstracttool.cpp \
    ../shared/abstractviewinspector.cpp \
    subcomponentmasklayeritem.cpp \
    zoomtool.cpp

HEADERS += \
    abstractliveedittool.h \
    boundingrecthighlighter.h \
    colorpickertool.h \
    livelayeritem.h \
    liverubberbandselectionmanipulator.h \
    liveselectionindicator.h \
    liveselectionrectangle.h \
    liveselectiontool.h \
    livesingleselectionmanipulator.h \
    qdeclarativeviewinspector.h \
    qdeclarativeviewinspector_p.h \
    qtquick1plugin.h \
    ../shared/abstracttool.h \
    ../shared/abstractviewinspector.h \
    ../shared/qdeclarativeinspectorprotocol.h \
    ../shared/qmlinspectorconstants.h \
    subcomponentmasklayeritem.h \
    zoomtool.h

target.path += $$[QT_INSTALL_PLUGINS]/qmltooling
INSTALLS += target
