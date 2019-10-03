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

macos {
    SUBDIRS += \
        metalunderqml \
        metaltextureimport
}

win32 {
    SUBDIRS += d3d11underqml
}

qtConfig(vulkan) {
    SUBDIRS += vulkanunderqml
}

EXAMPLE_FILES += \
    shared
