TEMPLATE = subdirs

contains(QT_CONFIG, opengl(es1|es2)?) {
    SUBDIRS += \
            simplematerial \
            sgengine \
            textureinsgnode \
            openglunderqml \
            textureinthread \
            twotextureproviders
}

SUBDIRS += \
        customgeometry \
        rendernode \
        threadedanimation
