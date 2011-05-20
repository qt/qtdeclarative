TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += declarative plugins imports
contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

