TEMPLATE = subdirs
QT_FOR_CONFIG += qml-private

SUBDIRS += \
    builtins \
    qtqml \
    models \
    labsmodels \
    tooling

qtConfig(qml-itemmodel): SUBDIRS += folderlistmodel
qtConfig(qml-worker-script): SUBDIRS += workerscript
qtHaveModule(sql): SUBDIRS += localstorage
qtConfig(settings): SUBDIRS += settings

qtHaveModule(quick) {
    QT_FOR_CONFIG += quick-private

    SUBDIRS += \
        labsanimation \
        layouts \
        qtquick2

    qtConfig(quick-shadereffect): SUBDIRS += wavefrontmesh

    qtHaveModule(testlib): SUBDIRS += testlib
    qtConfig(systemsemaphore): SUBDIRS += sharedimage
    qtConfig(quick-particles): \
        SUBDIRS += particles

    qtConfig(quick-path): SUBDIRS += shapes
}

