TEMPLATE = subdirs

SUBDIRS += \
    folderlistmodel \
    localstorage

!isEmpty(QT.quick.name) {
    SUBDIRS += \
        qtquick2 \
        particles \
        window \
        testlib
}

contains(QT_CONFIG, xmlpatterns) : SUBDIRS += xmllistmodel
