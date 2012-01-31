TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += declarative quick plugins

contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

SUBDIRS += imports
SUBDIRS += qmldevtools

