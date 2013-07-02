TEMPLATE=subdirs
SUBDIRS=\
    qml \
    quick \
    headersclean \
    particles \
    qmltest \
    qmldevtools \
    cmake \
    installed_cmake

installed_cmake.depends = cmake

testcocoon: SUBDIRS -= headersclean
