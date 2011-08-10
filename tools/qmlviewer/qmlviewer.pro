TEMPLATE = app
CONFIG += qt uic declarative_debug
DESTDIR = $$QT.declarative.bins
QT += declarative qtquick1 qtquick1-private

include(qml.pri)

SOURCES += main.cpp

INCLUDEPATH += ../../include/QtDeclarative
INCLUDEPATH += ../../src/declarative/util
INCLUDEPATH += ../../src/declarative/graphicsitems

DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

maemo5 {
    QT += maemo5
}
symbian {
    TARGET.UID3 = 0x20021317
    CONFIG += qt_demo
    TARGET.EPOCHEAPSIZE = 0x20000 0x4000000
    TARGET.CAPABILITY = NetworkServices ReadUserData

    # Deploy plugin for remote debugging
    qmldebuggingplugin.sources = $$QT_BUILD_TREE/plugins/qmltooling/qmldbg_tcp$${QT_LIBINFIX}.dll  $$QT_BUILD_TREE/plugins/qmltooling/qmldbg_ost$${QT_LIBINFIX}.dll 
    qmldebuggingplugin.path = c:$$QT_PLUGINS_BASE_DIR/qmltooling
    DEPLOYMENT +=  qmldebuggingplugin
}
mac {
    QMAKE_INFO_PLIST=Info_mac.plist
    TARGET=QMLViewer
    ICON=qml.icns
} else {
    TARGET=qmlviewer
}
