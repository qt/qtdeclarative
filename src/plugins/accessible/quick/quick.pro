TARGET  = qtaccessiblequick
DESTDIR = $$QT.gui.plugins/accessible

PLUGIN_TYPE = accessible
load(qt_plugin)

include ($$PWD/../shared/qaccessiblebase.pri)

QT += core-private gui-private v8-private qml-private quick-private

#DEFINES+=Q_ACCESSIBLE_QUICK_ITEM_ENABLE_DEBUG_DESCRIPTION

SOURCES  += \
    main.cpp \
    qaccessiblequickview.cpp \
    qaccessiblequickitem.cpp

HEADERS  += \
    qaccessiblequickview.h \
    qaccessiblequickitem.h

OTHERFILES += accessible.json
