TEMPLATE = subdirs

SUBDIRS += \
    rendercontrol_opengl

win32:!mingw {
    SUBDIRS += \
        rendercontrol_d3d11
}
