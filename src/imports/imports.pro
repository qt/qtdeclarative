TEMPLATE = subdirs

SUBDIRS += \
    builtins \
    qtqml \
    folderlistmodel \
    localstorage \
    models \
    settings \
    statemachine

qtHaveModule(quick) {
    SUBDIRS += \
        layouts \
        qtquick2 \
        window \
        testlib

    contains(QT_CONFIG, opengl(es1|es2)?) {
        SUBDIRS += \
            particles
    }
}

qtHaveModule(xmlpatterns) : SUBDIRS += xmllistmodel
