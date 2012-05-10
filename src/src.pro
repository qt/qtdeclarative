TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += qml quick particles plugins

contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

SUBDIRS += imports
SUBDIRS += qmldevtools
