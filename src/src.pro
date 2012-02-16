TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += qml quick plugins

contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

SUBDIRS += imports
SUBDIRS += qmldevtools

