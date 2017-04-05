TEMPLATE = subdirs

QT_FOR_CONFIG += quick-private

SUBDIRS += \
    builtins \
    qtqml \
    folderlistmodel \
    localstorage \
    models

qtConfig(settings): SUBDIRS += settings
qtConfig(statemachine): SUBDIRS += statemachine

qtHaveModule(quick) {
    SUBDIRS += \
        layouts \
        qtquick2 \
        window \
        testlib

    qtConfig(systemsemaphore): SUBDIRS += sharedimage
    qtConfig(quick-particles): \
        SUBDIRS += particles
}

qtHaveModule(xmlpatterns) : SUBDIRS += xmllistmodel
