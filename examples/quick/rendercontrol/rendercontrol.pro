TEMPLATE = subdirs

SUBDIRS += \
    rendercontrol_opengl

win32 {
    SUBDIRS += \
        rendercontrol_d3d11
}
