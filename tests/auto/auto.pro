TEMPLATE=subdirs

SUBDIRS = \
    cmake \
    installed_cmake \
    qmldevtools

qtHaveModule(quick):contains(QT_CONFIG, opengl(es1|es2)?) {
    SUBDIRS += \
        qml \
        quick \
        particles \
        qmltest \
        toolsupport

    qtHaveModule(widgets): SUBDIRS += quickwidgets
}

qmldevtools.CONFIG = host_build

installed_cmake.depends = cmake

