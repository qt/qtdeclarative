TARGET=softwarecontext

QT += gui-private core-private quick-private qml-private

PLUGIN_TYPE = scenegraph
PLUGIN_CLASS_NAME = ContextPlugin
load(qt_plugin)

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

