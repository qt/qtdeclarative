QT += qml quick

HEADERS += customrenderitem.h \
           openglrenderer.h \
           softwarerenderer.h

SOURCES += customrenderitem.cpp \
           openglrenderer.cpp \
           softwarerenderer.cpp \
           main.cpp

RESOURCES += rendernode.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/rendernode
INSTALLS += target

OTHER_FILES += \
    main.qml \
    shader.hlsl

qtConfig(d3d12) {
    HEADERS += d3d12renderer.h
    SOURCES += d3d12renderer.cpp
    LIBS += -ld3d12

    VSPS = shader.hlsl
    vshader.input = VSPS
    vshader.header = vs_shader.hlslh
    vshader.entry = VS_Simple
    vshader.type = vs_5_0
    pshader.input = VSPS
    pshader.header = ps_shader.hlslh
    pshader.entry = PS_Simple
    pshader.type = ps_5_0

    HLSL_SHADERS = vshader pshader
    load(hlsl_bytecode_header)
}
