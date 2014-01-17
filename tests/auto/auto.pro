TEMPLATE=subdirs
SUBDIRS=\
    qml \
    headersclean \
    qmldevtools \
    cmake \
    installed_cmake

qmldevtools.CONFIG = host_build

installed_cmake.depends = cmake

testcocoon: SUBDIRS -= headersclean
