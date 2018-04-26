TEMPLATE = subdirs

SUBDIRS += \
    builtins \
    qtqml \
    folderlistmodel \
    models

qtHaveModule(sql): SUBDIRS += localstorage
qtConfig(settings): SUBDIRS += settings
qtConfig(statemachine): SUBDIRS += statemachine

qtHaveModule(quick) {
    QT_FOR_CONFIG += quick-private

    SUBDIRS += \
        handlers \
        layouts \
        qtquick2 \
        window \
        tableview

    qtHaveModule(testlib): SUBDIRS += testlib
    qtConfig(systemsemaphore): SUBDIRS += sharedimage
    qtConfig(quick-particles): \
        SUBDIRS += particles

    SUBDIRS += shapes
}

qtHaveModule(xmlpatterns) : SUBDIRS += xmllistmodel
