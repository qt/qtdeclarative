TARGET=softwarecontext

QT += gui-private core-private quick-private qml-private

PLUGIN_TYPE = scenegraph
PLUGIN_CLASS_NAME = ContextPlugin
load(qt_plugin)

QMAKE_TARGET_PRODUCT = "Qt Quick 2D Renderer (Qt $$QT_VERSION)"
QMAKE_TARGET_DESCRIPTION = "Quick 2D Renderer for Qt."

SOURCES += \
    context.cpp \
    pluginmain.cpp \
    renderloop.cpp \
    rectanglenode.cpp \
    imagenode.cpp \
    pixmaptexture.cpp \
    glyphnode.cpp \
    renderingvisitor.cpp \
    ninepatchnode.cpp \
    softwarelayer.cpp \
    threadedrenderloop.cpp \
    painternode.cpp

HEADERS += \
    context.h \
    pluginmain.h \
    renderloop.h \
    rectanglenode.h \
    imagenode.h \
    pixmaptexture.h \
    glyphnode.h \
    renderingvisitor.h \
    ninepatchnode.h \
    softwarelayer.h \
    threadedrenderloop.h \
    painternode.h

OTHER_FILES += softwarecontext.json

