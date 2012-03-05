TEMPLATE = subdirs
SUBDIRS += demos shared localstorage particles qml quick tutorials window
contains(QT_CONFIG, qmltest): SUBDIRS += qmltest
