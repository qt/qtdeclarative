TEMPLATE=subdirs
SUBDIRS=\
    qml \
    quick \
    particles \
    qmltest \
    qmldevtools \
    cmake \
    installed_cmake \
    toolsupport

qtHaveModule(widgets): SUBDIRS += quickwidgets

qmldevtools.CONFIG = host_build

installed_cmake.depends = cmake

