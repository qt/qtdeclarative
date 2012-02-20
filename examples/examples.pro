TEMPLATE = subdirs
SUBDIRS += demos shared localstorage particles qml qtquick tutorials window
contains(QT_CONFIG, qmltest): SUBDIRS += qmltest
