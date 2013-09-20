TEMPLATE = subdirs

SUBDIRS += \
    folderlistmodel \
    localstorage \
    models \
    settings

qtHaveModule(quick) {
    SUBDIRS += \
        qtquick2 \
        particles \
        window \
        dialogs-private \
        dialogs \
        testlib
}

qtHaveModule(xmlpatterns) : SUBDIRS += xmllistmodel

qtHaveModule(quick):qtHaveModule(widgets): SUBDIRS += widgets
