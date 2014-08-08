TEMPLATE=lib
TARGET=softwarecontext

CONFIG += plugin

QT += gui-private core-private quick-private qml-private

SOURCES += \
    context.cpp \
    pluginmain.cpp

HEADERS += \
    context.h \
    pluginmain.h

OTHER_FILES += softwarecontext.json

target.path +=  $$[QT_INSTALL_PLUGINS]/scenegraph

files.path += $$[QT_INSTALL_PLUGINS]/scenegraph

INSTALLS += target files

