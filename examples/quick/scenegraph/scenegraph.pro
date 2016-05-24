TEMPLATE = subdirs

contains(QT_CONFIG, opengl(es1|es2)?) {
    SUBDIRS += \
            graph \
            simplematerial \
            sgengine \
            textureinsgnode \
            openglunderqml \
            textureinsgnode \
            textureinthread \
            twotextureproviders
}

SUBDIRS += \
        customgeometry \
        rendernode \
        threadedanimation

EXAMPLE_FILES += \
    shared
