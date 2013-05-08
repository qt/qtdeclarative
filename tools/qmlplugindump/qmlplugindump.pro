QT += qml qml-private quick-private core-private

#
# qmlplugindump is an applicaton bundle on the mac
# so that we can include an Info.plist, which is needed
# to surpress qmlplugindump popping up in the dock
# when launched.
#

CONFIG += qpa_minimal_plugin

SOURCES += \
    main.cpp \
    qmlstreamwriter.cpp

HEADERS += \
    qmlstreamwriter.h

OTHER_FILES += Info.plist
macx: QMAKE_INFO_PLIST = Info.plist

load(qt_tool)
