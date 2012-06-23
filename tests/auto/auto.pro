TEMPLATE=subdirs
SUBDIRS=\
    qml \
    quick \
    headersclean \
    particles \
    qmltest \
    qmldevtools \
    cmake

testcocoon: SUBDIRS -= headersclean
