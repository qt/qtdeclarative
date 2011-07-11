TEMPLATE=subdirs
SUBDIRS=\
    declarative \
    qtquick1 \

contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

!cross_compile:                             SUBDIRS += host.pro
