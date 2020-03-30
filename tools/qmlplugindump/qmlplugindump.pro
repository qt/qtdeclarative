QT += qml qml-private quick-private core-private
qtHaveModule(widgets): QT += widgets

CONFIG += no_import_scan

QTPLUGIN.platforms = qminimal

include(../shared/shared.pri)

SOURCES += \
    $$QMLSTREAMWRITER_SOURCES \
    main.cpp \
    qmltypereader.cpp

HEADERS += \
    $$QMLSTREAMWRITER_HEADERS \
    qmltypereader.h

macx {
    # Prevent qmlplugindump from popping up in the dock when launched.
    # We embed the Info.plist file, so the application doesn't need to
    # be a bundle.
    QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,$$shell_quote($$PWD/Info.plist)
    CONFIG -= app_bundle
}

QMAKE_TARGET_DESCRIPTION = QML Plugin Metadata Dumper

load(qt_tool)
