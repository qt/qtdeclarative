TEMPLATE = subdirs

SUBDIRS += \
    folderlistmodel \
    localstorage \
    models

qtHaveModule(quick) {
    SUBDIRS += \
        qtquick2 \
        particles \
        window \
        dialogs \
        testlib
}

qtHaveModule(xmlpatterns) : SUBDIRS += xmllistmodel

qtHaveModule(widgets) : SUBDIRS += widgets
