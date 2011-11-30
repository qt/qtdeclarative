TEMPLATE = subdirs
SUBDIRS += declarative tutorials
contains(QT_CONFIG, qmltest): SUBDIRS += qmltest
