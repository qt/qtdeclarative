TEMPLATE=subdirs
SUBDIRS=\
    qml \
    quick \
    quicktest \
    qmltest \
    qmldevtools \
    cmake \
    installed_cmake \
    toolsupport

qtHaveModule(gui):qtConfig(opengl(es1|es2)?) {
    SUBDIRS += particles
# Disabled for Qt 6 until a conclusion on QQuickWidget is reached
#    qtHaveModule(widgets): SUBDIRS += quickwidgets

}

# console applications not supported
uikit: SUBDIRS -= qmltest

installed_cmake.depends = cmake
