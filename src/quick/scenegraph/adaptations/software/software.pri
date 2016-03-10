QT += gui-private core-private qml-private

#DEFINES += QTQUICK2D_DEBUG_FLUSH

SOURCES += \
    $$PWD/qsgsoftwarecontext.cpp \
    $$PWD/qsgabstractsoftwarerenderer.cpp \
    $$PWD/qsgsoftwareglyphnode.cpp \
    $$PWD/qsgsoftwareimagenode.cpp \
    $$PWD/qsgsoftwareninepatchnode.cpp \
    $$PWD/qsgsoftwarepainternode.cpp \
    $$PWD/qsgsoftwarerectanglenode.cpp \
    $$PWD/qsgsoftwarepixmaprenderer.cpp \
    $$PWD/qsgsoftwarepixmaptexture.cpp \
    $$PWD/qsgsoftwarerenderablenode.cpp \
    $$PWD/qsgsoftwarerenderablenodeupdater.cpp \
    $$PWD/qsgsoftwarerenderer.cpp \
    $$PWD/qsgsoftwarerenderlistbuilder.cpp \
    $$PWD/qsgsoftwarerenderloop.cpp \
    $$PWD/qsgsoftwarelayer.cpp \
    $$PWD/qsgsoftwareadaptation.cpp

HEADERS += \
    $$PWD/qsgsoftwarecontext_p.h \
    $$PWD/qsgabstractsoftwarerenderer_p.h \
    $$PWD/qsgsoftwareglyphnode_p.h \
    $$PWD/qsgsoftwareimagenode_p.h \
    $$PWD/qsgsoftwareninepatchnode_p.h \
    $$PWD/qsgsoftwarepainternode_p.h \
    $$PWD/qsgsoftwarepixmaprenderer_p.h \
    $$PWD/qsgsoftwarepixmaptexture_p.h \
    $$PWD/qsgsoftwarerectanglenode_p.h \
    $$PWD/qsgsoftwarerenderablenode_p.h \
    $$PWD/qsgsoftwarerenderablenodeupdater_p.h \
    $$PWD/qsgsoftwarerenderer_p.h \
    $$PWD/qsgsoftwarerenderlistbuilder_p.h \
    $$PWD/qsgsoftwarerenderloop_p.h \
    $$PWD/qsgsoftwarelayer_p.h \
    $$PWD/qsgsoftwareadaptation_p.h
