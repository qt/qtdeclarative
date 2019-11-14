# Generated from quickshapes.pro.

#####################################################################
## QuickShapes Module:
#####################################################################

qt_add_module(QuickShapes
    INTERNAL_MODULE
    SOURCES
        qquickshape.cpp qquickshape_p.h
        qquickshape_p_p.h
        qquickshapegenericrenderer.cpp qquickshapegenericrenderer_p.h
        qquickshapesglobal.h qquickshapesglobal_p.h
        qquickshapesoftwarerenderer.cpp qquickshapesoftwarerenderer_p.h
    LIBRARIES
        Qt::GuiPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qtquickshapes_resource_files
    "shaders/blit.frag"
    "shaders/blit.vert"
    "shaders/blit_core.frag"
    "shaders/blit_core.vert"
    "shaders/conicalgradient.frag"
    "shaders/conicalgradient.vert"
    "shaders/conicalgradient_core.frag"
    "shaders/conicalgradient_core.vert"
    "shaders/lineargradient.frag"
    "shaders/lineargradient.vert"
    "shaders/lineargradient_core.frag"
    "shaders/lineargradient_core.vert"
    "shaders/radialgradient.frag"
    "shaders/radialgradient.vert"
    "shaders/radialgradient_core.frag"
    "shaders/radialgradient_core.vert"
    "shaders_ng/conicalgradient.frag.qsb"
    "shaders_ng/conicalgradient.vert.qsb"
    "shaders_ng/lineargradient.frag.qsb"
    "shaders_ng/lineargradient.vert.qsb"
    "shaders_ng/radialgradient.frag.qsb"
    "shaders_ng/radialgradient.vert.qsb"
)

qt_add_resource(QuickShapes "qtquickshapes"
    PREFIX
        "/qt-project.org/shapes"
    FILES
        ${qtquickshapes_resource_files}
)


## Scopes:
#####################################################################

qt_extend_target(QuickShapes CONDITION QT_FEATURE_opengl
    SOURCES
        qquicknvprfunctions.cpp qquicknvprfunctions_p.h
        qquicknvprfunctions_p_p.h
        qquickshapenvprrenderer.cpp qquickshapenvprrenderer_p.h
)
