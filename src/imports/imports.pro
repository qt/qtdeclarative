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
# disabled to allow file dialog changes to use urls internally (qtbase)
#        dialogs \
        testlib
}

qtHaveModule(xmlpatterns) : SUBDIRS += xmllistmodel

qtHaveModule(quick):qtHaveModule(widgets): SUBDIRS += widgets
