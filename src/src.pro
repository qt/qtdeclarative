TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += declarative plugins
contains(QT_CONFIG, qmltest): SUBDIRS += qmltest
SUBDIRS += imports
