TEMPLATE=subdirs
SUBDIRS=\
    qml \
    quick \
    qmltest \
    qmldevtools \
    cmake \
    installed_cmake \
    toolsupport

qtHaveModule(gui):qtConfig(opengl(es1|es2)?) {
    SUBDIRS += particles
    qtHaveModule(widgets): SUBDIRS += quickwidgets

}

qmldevtools.CONFIG = host_build

installed_cmake.depends = cmake
