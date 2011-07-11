TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += v8 declarative qtquick1 plugins
contains(QT_CONFIG, qmltest): SUBDIRS += qmltest
SUBDIRS += imports
