TEMPLATE = subdirs

SUBDIRS += \
    folderlistmodel \
    localstorage

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
