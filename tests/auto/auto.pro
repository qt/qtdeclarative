TEMPLATE=subdirs
SUBDIRS=\
    qml \
    headersclean \
    particles \
    qmltest \
    qmldevtools \
    cmake \
    installed_cmake

!mac:SUBDIRS += quick

installed_cmake.depends = cmake

testcocoon: SUBDIRS -= headersclean
