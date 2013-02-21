TEMPLATE = subdirs

SUBDIRS += \
    folderlistmodel \
    localstorage

qtHaveModule(quick) {
    SUBDIRS += \
        qtquick2 \
        particles \
        window \
        testlib
}

qtHaveModule(xmlpatterns) : SUBDIRS += xmllistmodel
