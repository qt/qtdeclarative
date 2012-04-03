TEMPLATE=subdirs
SUBDIRS=\
    qml \
    quick \
    headersclean \
    particles \
    qmltest \
    qmldevtools

testcocoon: SUBDIRS -= headersclean
