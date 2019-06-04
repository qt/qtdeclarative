# Generated from openvg.pro.

#####################################################################
## qsgopenvgbackend Plugin:
#####################################################################

add_qt_plugin(qsgopenvgbackend
    TYPE scenegraph
    CLASS_NAME QSGOpenVGAdaptation
    SOURCES
        qopenvgcontext.cpp qopenvgcontext_p.h
        qopenvgmatrix.cpp qopenvgmatrix.h
        qopenvgoffscreensurface.cpp qopenvgoffscreensurface.h
        qsgopenvgadaptation.cpp qsgopenvgadaptation_p.h
        qsgopenvgcontext.cpp qsgopenvgcontext_p.h
        qsgopenvgfontglyphcache.cpp qsgopenvgfontglyphcache.h
        qsgopenvgglyphnode.cpp qsgopenvgglyphnode_p.h
        qsgopenvghelpers.cpp qsgopenvghelpers.h
        qsgopenvginternalimagenode.cpp qsgopenvginternalimagenode.h
        qsgopenvginternalrectanglenode.cpp qsgopenvginternalrectanglenode.h
        qsgopenvglayer.cpp qsgopenvglayer.h
        qsgopenvgnodevisitor.cpp qsgopenvgnodevisitor.h
        qsgopenvgpainternode.cpp qsgopenvgpainternode.h
        qsgopenvgpublicnodes.cpp qsgopenvgpublicnodes.h
        qsgopenvgrenderable.cpp qsgopenvgrenderable.h
        qsgopenvgrenderer.cpp qsgopenvgrenderer_p.h
        qsgopenvgrenderloop.cpp qsgopenvgrenderloop_p.h
        qsgopenvgtexture.cpp qsgopenvgtexture.h
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        openvg
)

#### Keys ignored in scope 1:.:.:openvg.pro:<TRUE>:
# CONFIG = "egl"
# OTHER_FILES = "$$PWD/openvg.json"
# PLUGIN_CLASS_NAME = "QSGOpenVGAdaptation"
# PLUGIN_TYPE = "scenegraph"
# QMAKE_TARGET_DESCRIPTION = "Quick OpenVG Renderer for Qt."
# QMAKE_TARGET_PRODUCT = "Qt Quick OpenVG Renderer (Qt $$QT_VERSION)"
# _LOADED = "qt_plugin"

## Scopes:
#####################################################################

extend_target(qsgopenvgbackend CONDITION QT_FEATURE_quick_sprite
    SOURCES
        qsgopenvgspritenode.cpp qsgopenvgspritenode.h
)
