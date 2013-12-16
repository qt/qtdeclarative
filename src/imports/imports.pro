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
        testlib
}

qtHaveModule(xmlpatterns) : SUBDIRS += xmllistmodel
