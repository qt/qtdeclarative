SOURCES += \
    $$PWD/qsgd3d12adaptation.cpp \
    $$PWD/qsgd3d12renderloop.cpp \
    $$PWD/qsgd3d12renderer.cpp \
    $$PWD/qsgd3d12context.cpp \
    $$PWD/qsgd3d12rendercontext.cpp \
    $$PWD/qsgd3d12rectanglenode.cpp \
    $$PWD/qsgd3d12material.cpp \
    $$PWD/qsgd3d12builtinmaterials.cpp \
    $$PWD/qsgd3d12texture.cpp \
    $$PWD/qsgd3d12imagenode.cpp \
    $$PWD/qsgd3d12glyphnode.cpp \
    $$PWD/qsgd3d12glyphcache.cpp

NO_PCH_SOURCES += \
    $$PWD/qsgd3d12engine.cpp

HEADERS += \
    $$PWD/qsgd3d12adaptation_p.h \
    $$PWD/qsgd3d12renderloop_p.h \
    $$PWD/qsgd3d12renderer_p.h \
    $$PWD/qsgd3d12context_p.h \
    $$PWD/qsgd3d12rendercontext_p.h \
    $$PWD/qsgd3d12engine_p.h \
    $$PWD/qsgd3d12engine_p_p.h \
    $$PWD/qsgd3d12rectanglenode_p.h \
    $$PWD/qsgd3d12material_p.h \
    $$PWD/qsgd3d12builtinmaterials_p.h \
    $$PWD/qsgd3d12texture_p.h \
    $$PWD/qsgd3d12imagenode_p.h \
    $$PWD/qsgd3d12glyphnode_p.h \
    $$PWD/qsgd3d12glyphcache_p.h

LIBS += -ldxgi -ld3d12

DEFINES += QSG_D3D12

include($$PWD/shaders/shaders.pri)
