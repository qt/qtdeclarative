TEMPLATE=subdirs
SUBDIRS=\
    qml \
    quick \
    particles \
    qmldevtools

contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

!cross_compile:                             SUBDIRS += host.pro
