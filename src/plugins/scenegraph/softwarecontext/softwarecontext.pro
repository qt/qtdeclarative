TARGET=softwarecontext

QT += gui-private core-private quick-private qml-private

PLUGIN_TYPE = scenegraph
PLUGIN_CLASS_NAME = ContextPlugin
load(qt_plugin)

QMAKE_TARGET_PRODUCT = "Qt Quick 2D Renderer (Qt $$QT_VERSION)"
QMAKE_TARGET_DESCRIPTION = "Quick 2D Renderer for Qt."


#DEFINES += QTQUICK2D_DEBUG_FLUSH

SOURCES += \
    qsgsoftwarecontext.cpp \
    qsgabstractsoftwarerenderer.cpp \
    qsgsoftwareglyphnode.cpp \
    qsgsoftwareimagenode.cpp \
    qsgsoftwareninepatchnode.cpp \
    qsgsoftwarepainternode.cpp \
    qsgsoftwarerectanglenode.cpp \
    qsgsoftwarepixmaprenderer.cpp \
    qsgsoftwarepixmaptexture.cpp \
    qsgsoftwarerenderablenode.cpp \
    qsgsoftwarerenderablenodeupdater.cpp \
    qsgsoftwarerenderer.cpp \
    qsgsoftwarerenderlistbuilder.cpp \
    qsgsoftwarerenderloop.cpp \
    qsgsoftwarelayer.cpp \
    qsgsoftwarecontextplugin.cpp

HEADERS += \
    qsgsoftwarecontext_p.h \
    qsgabstractsoftwarerenderer_p.h \
    qsgsoftwareglyphnode_p.h \
    qsgsoftwareimagenode_p.h \
    qsgsoftwareninepatchnode_p.h \
    qsgsoftwarepainternode_p.h \
    qsgsoftwarepixmaprenderer_p.h \
    qsgsoftwarepixmaptexture_p.h \
    qsgsoftwarerectanglenode_p.h \
    qsgsoftwarerenderablenode_p.h \
    qsgsoftwarerenderablenodeupdater_p.h \
    qsgsoftwarerenderer_p.h \
    qsgsoftwarerenderlistbuilder_p.h \
    qsgsoftwarerenderloop_p.h \
    qsgsoftwarelayer_p.h \
    qsgsoftwarecontextplugin_p.h

OTHER_FILES += softwarecontext.json

