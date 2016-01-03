TARGET=softwarecontext

QT += gui-private core-private quick-private qml-private

PLUGIN_TYPE = scenegraph
PLUGIN_CLASS_NAME = ContextPlugin
load(qt_plugin)

QMAKE_TARGET_PRODUCT = "Qt Quick 2D Renderer (Qt $$QT_VERSION)"
QMAKE_TARGET_DESCRIPTION = "Quick 2D Renderer for Qt."


#DEFINES += QTQUICK2D_DEBUG_FLUSH

SOURCES += \
    context.cpp \
    pluginmain.cpp \
    renderloop.cpp \
    rectanglenode.cpp \
    imagenode.cpp \
    pixmaptexture.cpp \
    glyphnode.cpp \
    ninepatchnode.cpp \
    softwarelayer.cpp \
    painternode.cpp \
    renderablenode.cpp \
    renderer.cpp \
    pixmaprenderer.cpp \
    renderablenodeupdater.cpp \
    renderlistbuilder.cpp \
    abstractsoftwarerenderer.cpp

HEADERS += \
    context.h \
    pluginmain.h \
    renderloop.h \
    rectanglenode.h \
    imagenode.h \
    pixmaptexture.h \
    glyphnode.h \
    ninepatchnode.h \
    softwarelayer.h \
    painternode.h \
    renderablenode.h \
    renderer.h \
    pixmaprenderer.h \
    renderablenodeupdater.h \
    renderlistbuilder.h \
    abstractsoftwarerenderer.h

OTHER_FILES += softwarecontext.json

