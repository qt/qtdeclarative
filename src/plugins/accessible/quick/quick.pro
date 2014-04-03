TARGET  = qtaccessiblequick

PLUGIN_TYPE = accessible
PLUGIN_EXTENDS = quick
PLUGIN_CLASS_NAME = AccessibleQuickFactory
load(qt_plugin)

include ($$PWD/../shared/qaccessiblebase.pri)

QT += core-private gui-private  qml-private quick-private

#DEFINES+=Q_ACCESSIBLE_QUICK_ITEM_ENABLE_DEBUG_DESCRIPTION

SOURCES  += \
    main.cpp \
    qaccessiblequickview.cpp \
    qaccessiblequickitem.cpp

HEADERS  += \
    qaccessiblequickview.h \
    qaccessiblequickitem.h

OTHERFILES += accessible.json
