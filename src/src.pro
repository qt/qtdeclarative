TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += v8 declarative plugins
contains(QT_CONFIG, qmltest): SUBDIRS += qmltest
SUBDIRS += imports
