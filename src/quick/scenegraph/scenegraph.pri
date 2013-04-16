!contains(QT_CONFIG, egl):DEFINES += QT_NO_EGL

# Core API
HEADERS += \
    $$PWD/coreapi/qsgdefaultrenderer_p.h \
    $$PWD/coreapi/qsggeometry.h \
    $$PWD/coreapi/qsgmaterial.h \
    $$PWD/coreapi/qsgnode.h \
    $$PWD/coreapi/qsgnodeupdater_p.h \
    $$PWD/coreapi/qsgrenderer_p.h \
    $$PWD/coreapi/qsgrendernode_p.h \
    $$PWD/coreapi/qsggeometry_p.h

SOURCES += \
    $$PWD/coreapi/qsgdefaultrenderer.cpp \
    $$PWD/coreapi/qsggeometry.cpp \
    $$PWD/coreapi/qsgmaterial.cpp \
    $$PWD/coreapi/qsgnode.cpp \
    $$PWD/coreapi/qsgnodeupdater.cpp \
    $$PWD/coreapi/qsgrenderer.cpp \
    $$PWD/coreapi/qsgrendernode.cpp \
    scenegraph/util/qsgsimplematerial.cpp

# Util API
HEADERS += \
    $$PWD/util/qsgareaallocator_p.h \
    $$PWD/util/qsgdepthstencilbuffer_p.h \
    $$PWD/util/qsgflatcolormaterial.h \
    $$PWD/util/qsgsimplematerial.h \
    $$PWD/util/qsgsimplerectnode.h \
    $$PWD/util/qsgsimpletexturenode.h \
    $$PWD/util/qsgtexturematerial.h \
    $$PWD/util/qsgtexturematerial_p.h \
    $$PWD/util/qsgvertexcolormaterial.h \
    $$PWD/util/qsgtexture.h \
    $$PWD/util/qsgtexture_p.h \
    $$PWD/util/qsgtextureprovider.h \
    $$PWD/util/qsgpainternode_p.h \
    $$PWD/util/qsgdistancefieldutil_p.h

SOURCES += \
    $$PWD/util/qsgareaallocator.cpp \
    $$PWD/util/qsgdepthstencilbuffer.cpp \
    $$PWD/util/qsgflatcolormaterial.cpp \
    $$PWD/util/qsgsimplerectnode.cpp \
    $$PWD/util/qsgsimpletexturenode.cpp \
    $$PWD/util/qsgtexturematerial.cpp \
    $$PWD/util/qsgvertexcolormaterial.cpp \
    $$PWD/util/qsgtexture.cpp \
    $$PWD/util/qsgtextureprovider.cpp \
    $$PWD/util/qsgpainternode.cpp \
    $$PWD/util/qsgdistancefieldutil.cpp

# QML / Adaptations API
HEADERS += \
    $$PWD/qsgadaptationlayer_p.h \
    $$PWD/qsgcontext_p.h \
    $$PWD/qsgcontextplugin_p.h \
    $$PWD/qsgdefaultglyphnode_p.h \
    $$PWD/qsgdefaultdistancefieldglyphcache_p.h \
    $$PWD/qsgdistancefieldglyphnode_p.h \
    $$PWD/qsgdistancefieldglyphnode_p_p.h \
    $$PWD/qsgdefaultglyphnode_p_p.h \
    $$PWD/qsgdefaultimagenode_p.h \
    $$PWD/qsgdefaultrectanglenode_p.h \
    $$PWD/qsgflashnode_p.h \
    $$PWD/qsgshareddistancefieldglyphcache_p.h \
    $$PWD/qsgrenderloop_p.h \
    $$PWD/qsgthreadedrenderloop_p.h \
    $$PWD/qsgwindowsrenderloop_p.h

SOURCES += \
    $$PWD/qsgadaptationlayer.cpp \
    $$PWD/qsgcontext.cpp \
    $$PWD/qsgcontextplugin.cpp \
    $$PWD/qsgdefaultglyphnode.cpp \
    $$PWD/qsgdefaultglyphnode_p.cpp \
    $$PWD/qsgdefaultdistancefieldglyphcache.cpp \
    $$PWD/qsgdistancefieldglyphnode.cpp \
    $$PWD/qsgdistancefieldglyphnode_p.cpp \
    $$PWD/qsgdefaultimagenode.cpp \
    $$PWD/qsgdefaultrectanglenode.cpp \
    $$PWD/qsgflashnode.cpp \
    $$PWD/qsgshareddistancefieldglyphcache.cpp \
    $$PWD/qsgrenderloop.cpp \
    $$PWD/qsgthreadedrenderloop.cpp \
    $$PWD/qsgwindowsrenderloop.cpp
