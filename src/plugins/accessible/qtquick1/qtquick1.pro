contains(QT_CONFIG, accessibility) {

TARGET  = qtaccessibleqtquick1
load(qt_plugin)
include ($$PWD/../shared/qaccessiblebase.pri)

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private
DESTDIR = $$QT.gui.plugins/accessible

QTDIR_build:REQUIRES += "contains(QT_CONFIG, accessibility)"

DEFINES+=Q_ACCESSIBLE_QUICK_ITEM_ENABLE_DEBUG_DESCRIPTION

SOURCES  += \
    main.cpp \
    qaccessibledeclarativeview.cpp \
    qaccessibledeclarativeitem.cpp

HEADERS  += \
    qaccessibledeclarativeview.h \
    qaccessibledeclarativeitem.h
}
