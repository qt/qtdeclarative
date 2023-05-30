TEMPLATE = subdirs

qtConfig(opengl(es1|es2)?) {
    SUBDIRS += \
            openglunderqml
}

SUBDIRS += \
        customgeometry \
        custommaterial \
        graph \
        threadedanimation \
        twotextureproviders \
        rhiunderqml \
        rhitextureitem

macos|ios {
    SUBDIRS += \
        metalunderqml \
        metaltextureimport
}

win32 {
    SUBDIRS += d3d11underqml
}

qtConfig(vulkan) {
    SUBDIRS += \
        vulkanunderqml \
        vulkantextureimport
}

EXAMPLE_FILES += \
    shared
