TEMPLATE = subdirs
SUBDIRS += declarative 
contains(QT_CONFIG, qmltest): SUBDIRS += qmltest
