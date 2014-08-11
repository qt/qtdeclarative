TEMPLATE=lib
TARGET=softwarecontext

CONFIG += plugin

QT += gui-private core-private quick-private qml-private

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
    softwarelayer.cpp

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
    softwarelayer.h

OTHER_FILES += softwarecontext.json

target.path +=  $$[QT_INSTALL_PLUGINS]/scenegraph

files.path += $$[QT_INSTALL_PLUGINS]/scenegraph

INSTALLS += target files

