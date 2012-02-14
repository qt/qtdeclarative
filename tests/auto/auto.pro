TEMPLATE=subdirs
SUBDIRS=\
    declarative \
    qtquick2 \
    particles \
    qmldevtools

contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

!cross_compile:                             SUBDIRS += host.pro
