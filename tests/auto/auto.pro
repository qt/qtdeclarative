TEMPLATE=subdirs
SUBDIRS=\
    qml \
    quick \
    headersclean \
    particles \
    qmldevtools

contains(QT_CONFIG, qmltest): SUBDIRS += qmltest
