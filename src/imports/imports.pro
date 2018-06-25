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
        layouts \
        qtquick2 \
        window \
        tableview

    qtHaveModule(testlib): SUBDIRS += testlib
    qtConfig(systemsemaphore): SUBDIRS += sharedimage
    qtConfig(quick-particles): \
        SUBDIRS += particles

    qtConfig(quick-path): SUBDIRS += shapes
}

qtHaveModule(xmlpatterns) : SUBDIRS += xmllistmodel
