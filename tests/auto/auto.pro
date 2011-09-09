TEMPLATE=subdirs
SUBDIRS=\
    declarative


contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

!cross_compile:                             SUBDIRS += host.pro
