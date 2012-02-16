TEMPLATE = subdirs
SUBDIRS += qml tutorials
contains(QT_CONFIG, qmltest): SUBDIRS += qmltest
