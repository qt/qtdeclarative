TEMPLATE=subdirs
SUBDIRS=\
    qml \
    quick \
    headersclean \
    particles \
    qmldevtools

testcocoon: SUBDIRS -= headersclean

contains(QT_CONFIG, qmltest): SUBDIRS += qmltest
