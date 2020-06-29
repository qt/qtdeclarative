# DEFINES += QSG_DISTANCEFIELD_CACHE_DEBUG

# Core API
HEADERS += \
    $$PWD/coreapi/qsggeometry.h \
    $$PWD/coreapi/qsgmaterial.h \
    $$PWD/coreapi/qsgmaterialtype.h \
    $$PWD/coreapi/qsgmaterialshader.h \
    $$PWD/coreapi/qsgmaterialshader_p.h \
    $$PWD/coreapi/qsgnode.h \
    $$PWD/coreapi/qsgnode_p.h \
    $$PWD/coreapi/qsgnodeupdater_p.h \
    $$PWD/coreapi/qsgabstractrenderer_p.h \
    $$PWD/coreapi/qsgabstractrenderer_p_p.h \
    $$PWD/coreapi/qsgrenderer_p.h \
    $$PWD/coreapi/qsgrendernode.h \
    $$PWD/coreapi/qsgrendernode_p.h \
    $$PWD/coreapi/qsgrendererinterface.h \
    $$PWD/coreapi/qsggeometry_p.h \
    $$PWD/coreapi/qsgtexture.h \
    $$PWD/coreapi/qsgtexture_p.h \
    $$PWD/coreapi/qsgtexture_platform.h \
    $$PWD/coreapi/qsgbatchrenderer_p.h \
    $$PWD/coreapi/qsgrhivisualizer_p.h

SOURCES += \
    $$PWD/coreapi/qsgabstractrenderer.cpp \
    $$PWD/coreapi/qsggeometry.cpp \
    $$PWD/coreapi/qsgmaterial.cpp \
    $$PWD/coreapi/qsgmaterialshader.cpp \
    $$PWD/coreapi/qsgtexture.cpp \
    $$PWD/coreapi/qsgnode.cpp \
    $$PWD/coreapi/qsgnodeupdater.cpp \
    $$PWD/coreapi/qsgrenderer.cpp \
    $$PWD/coreapi/qsgrendernode.cpp \
    $$PWD/coreapi/qsgrendererinterface.cpp \
    $$PWD/coreapi/qsgbatchrenderer.cpp \
    $$PWD/coreapi/qsgrhivisualizer.cpp

macos|ios {
    SOURCES += \
        $$PWD/coreapi/qsgtexture_mac.mm
}

# Util API
HEADERS += \
    $$PWD/util/qsgareaallocator_p.h \
    $$PWD/util/qsgplaintexture_p.h \
    $$PWD/util/qsgsimplerectnode.h \
    $$PWD/util/qsgsimpletexturenode.h \
    $$PWD/util/qsgtextureprovider.h \
    $$PWD/util/qsgflatcolormaterial.h \
    $$PWD/util/qsgtexturematerial.h \
    $$PWD/util/qsgtexturematerial_p.h \
    $$PWD/util/qsgvertexcolormaterial.h \
    $$PWD/util/qsgrectanglenode.h \
    $$PWD/util/qsgimagenode.h \
    $$PWD/util/qsgninepatchnode.h

SOURCES += \
    $$PWD/util/qsgareaallocator.cpp \
    $$PWD/util/qsgplaintexture.cpp \
    $$PWD/util/qsgsimplerectnode.cpp \
    $$PWD/util/qsgsimpletexturenode.cpp \
    $$PWD/util/qsgtextureprovider.cpp \
    $$PWD/util/qsgflatcolormaterial.cpp \
    $$PWD/util/qsgtexturematerial.cpp \
    $$PWD/util/qsgvertexcolormaterial.cpp \
    $$PWD/util/qsgrectanglenode.cpp \
    $$PWD/util/qsgimagenode.cpp \
    $$PWD/util/qsgninepatchnode.cpp

# RHI
HEADERS += \
    $$PWD/qsgrhitextureglyphcache_p.h \
    $$PWD/util/qsgrhiatlastexture_p.h \
    $$PWD/qsgrhilayer_p.h \
    $$PWD/qsgrhishadereffectnode_p.h \
    $$PWD/qsgrhidistancefieldglyphcache_p.h

SOURCES += \
    $$PWD/qsgrhitextureglyphcache.cpp \
    $$PWD/qsgrhilayer.cpp \
    $$PWD/qsgrhishadereffectnode.cpp \
    $$PWD/util/qsgrhiatlastexture.cpp \
    $$PWD/qsgrhidistancefieldglyphcache.cpp

# QML / Adaptations API
HEADERS += \
    $$PWD/qsgadaptationlayer_p.h \
    $$PWD/qsgcontext_p.h \
    $$PWD/qsgcontextplugin_p.h \
    $$PWD/qsgbasicinternalrectanglenode_p.h \
    $$PWD/qsgbasicinternalimagenode_p.h \
    $$PWD/qsgbasicglyphnode_p.h \
    $$PWD/qsgrenderloop_p.h \
    $$PWD/qsgrhisupport_p.h

SOURCES += \
    $$PWD/qsgadaptationlayer.cpp \
    $$PWD/qsgcontext.cpp \
    $$PWD/qsgcontextplugin.cpp \
    $$PWD/qsgbasicinternalrectanglenode.cpp \
    $$PWD/qsgbasicinternalimagenode.cpp \
    $$PWD/qsgbasicglyphnode.cpp \
    $$PWD/qsgrenderloop.cpp \
    $$PWD/qsgrhisupport.cpp

SOURCES += \
    $$PWD/qsgdefaultglyphnode.cpp \
    $$PWD/qsgdefaultglyphnode_p.cpp \
    $$PWD/qsgdistancefieldglyphnode.cpp \
    $$PWD/qsgdistancefieldglyphnode_p.cpp \
    $$PWD/qsgdefaultinternalimagenode.cpp \
    $$PWD/qsgdefaultinternalrectanglenode.cpp \
    $$PWD/qsgdefaultrendercontext.cpp \
    $$PWD/qsgdefaultcontext.cpp \
    $$PWD/util/qsgdefaultpainternode.cpp \
    $$PWD/util/qsgdefaultrectanglenode.cpp \
    $$PWD/util/qsgdefaultimagenode.cpp \
    $$PWD/util/qsgdefaultninepatchnode.cpp
HEADERS += \
    $$PWD/qsgdefaultglyphnode_p.h \
    $$PWD/qsgdistancefieldglyphnode_p.h \
    $$PWD/qsgdistancefieldglyphnode_p_p.h \
    $$PWD/qsgdefaultglyphnode_p_p.h \
    $$PWD/qsgdefaultinternalimagenode_p.h \
    $$PWD/qsgdefaultinternalrectanglenode_p.h \
    $$PWD/qsgdefaultrendercontext_p.h \
    $$PWD/qsgdefaultcontext_p.h \
    $$PWD/util/qsgdefaultpainternode_p.h \
    $$PWD/util/qsgdefaultrectanglenode_p.h \
    $$PWD/util/qsgdefaultimagenode_p.h \
    $$PWD/util/qsgdefaultninepatchnode_p.h

qtConfig(thread) {
    SOURCES += \
        $$PWD/qsgthreadedrenderloop.cpp
    HEADERS += \
        $$PWD/qsgthreadedrenderloop_p.h
}

qtConfig(quick-sprite) {
    SOURCES += \
        $$PWD/qsgdefaultspritenode.cpp
    HEADERS += \
        $$PWD/qsgdefaultspritenode_p.h
}

# Built-in, non-plugin-based adaptations
include(adaptations/adaptations.pri)

RESOURCES += \
    $$PWD/scenegraph.qrc

# Compressed Texture API
HEADERS += \
    $$PWD/util/qsgtexturereader_p.h

SOURCES += \
    $$PWD/util/qsgtexturereader.cpp

    HEADERS += \
        $$PWD/compressedtexture/qsgcompressedatlastexture_p.h \
        $$PWD/compressedtexture/qsgcompressedtexture_p.h

    SOURCES += \
        $$PWD/compressedtexture/qsgcompressedatlastexture.cpp \
        $$PWD/compressedtexture/qsgcompressedtexture.cpp

