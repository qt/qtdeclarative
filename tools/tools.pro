TEMPLATE = subdirs
qtHaveModule(quick): SUBDIRS += qmlscene qmlplugindump
qtHaveModule(qmltest): SUBDIRS += qmltestrunner
SUBDIRS += \
    qmlmin \
    qmlprofiler \
    qmlbundle
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
