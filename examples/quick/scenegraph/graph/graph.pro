QT       += core gui quick

TARGET = graph

TEMPLATE = app

CONFIG += qmltypes
QML_IMPORT_NAME = Graph
QML_IMPORT_MAJOR_VERSION = 1

SOURCES += main.cpp \
    graph.cpp \
    noisynode.cpp \
    gridnode.cpp \
    linenode.cpp

HEADERS += \
    graph.h \
    noisynode.h \
    gridnode.h \
    linenode.h

RESOURCES += \
    graph.qrc

OTHER_FILES += \
    main.qml \
    shaders/noisy.vsh \
    shaders/noisy.fsh \
    shaders/line.fsh \
    shaders/line.vsh

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/graph
INSTALLS += target
