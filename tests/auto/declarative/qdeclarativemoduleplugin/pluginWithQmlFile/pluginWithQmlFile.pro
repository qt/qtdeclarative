TEMPLATE = lib
CONFIG += plugin
SOURCES = plugin.cpp
QT = core declarative
DESTDIR = ../imports/com/nokia/AutoTestPluginWithQmlFile

symbian: {
    TARGET.EPOCALLOWDLLDATA=1
}
QT += core-private gui-private declarative-private
