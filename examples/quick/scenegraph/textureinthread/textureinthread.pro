QT += quick

# To make threaded gl check...
QT += core-private gui-private

CONFIG += qmltypes
QML_IMPORT_NAME = SceneGraphRendering
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += threadrenderer.h
SOURCES += threadrenderer.cpp main.cpp

INCLUDEPATH += ../shared
HEADERS += ../shared/logorenderer.h
SOURCES += ../shared/logorenderer.cpp

RESOURCES += textureinthread.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/textureinthread
INSTALLS += target

OTHER_FILES += \
    main.qml \
    error.qml
