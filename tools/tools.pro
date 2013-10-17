TEMPLATE = subdirs
qtHaveModule(quick): !android|android_app: SUBDIRS += qmlscene qmlplugindump
qtHaveModule(qmltest): !android|android_app: SUBDIRS += qmltestrunner
SUBDIRS += \
    qmlmin \
    qmlprofiler
!android|android_app: SUBDIRS += \
                          qml \
                          qmlbundle \
                          v4
qtHaveModule(quick):qtHaveModule(widgets): SUBDIRS += qmleasing

# qmlmin & qmlbundle are build tools.
# qmlscene is needed by the autotests.
# qmltestrunner may be useful for manual testing.
# qmlplugindump cannot be a build tool, because it loads target plugins.
# The other apps are mostly "desktop" tools and are thus excluded.
qtNomakeTools( \
    qmlprofiler \
    qmlplugindump \
    qmleasing \
)

qtHaveModule(quick) {
    for(subdir, SUBDIRS): $${subdir}.depends += qmlimportscanner
    SUBDIRS += qmlimportscanner
}
