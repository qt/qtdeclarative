TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += declarative quick qtquick1 plugins

contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

SUBDIRS += imports
SUBDIRS += qmldevtools

