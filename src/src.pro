TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += declarative qtquick1 plugins

contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

SUBDIRS += imports
SUBDIRS += qmldevtools

