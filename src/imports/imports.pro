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
        qtquick2 \
        particles \
        window \
        testlib
}

qtHaveModule(xmlpatterns) : SUBDIRS += xmllistmodel
