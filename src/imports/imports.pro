TEMPLATE = subdirs

SUBDIRS += \
    builtins \
    qtqml \
    folderlistmodel \
    localstorage \
    models

qtConfig(settings): SUBDIRS += settings
qtConfig(statemachine): SUBDIRS += statemachine

qtHaveModule(quick) {
    SUBDIRS += \
        layouts \
        qtquick2 \
        window \
        testlib

    qtConfig(opengl(es1|es2)?): \
        SUBDIRS += particles
}

qtHaveModule(xmlpatterns) : SUBDIRS += xmllistmodel
