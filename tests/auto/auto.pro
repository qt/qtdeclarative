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

qtHaveModule(gui):     SUBDIRS += particles
qtHaveModule(widgets): SUBDIRS += quickwidgets
qtHaveModule(qmldom):   SUBDIRS += qmldom

# console applications not supported
uikit: SUBDIRS -= qmltest

installed_cmake.depends = cmake
