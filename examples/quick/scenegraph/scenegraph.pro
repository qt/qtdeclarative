TEMPLATE = subdirs

qtConfig(opengl(es1|es2)?) {
    SUBDIRS += \
            graph \
            simplematerial \
            sgengine \
            fboitem \
            openglunderqml \
            textureinthread \
            twotextureproviders
}

SUBDIRS += \
        customgeometry \
        rendernode \
        threadedanimation

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
