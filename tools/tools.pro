TEMPLATE = subdirs
qtHaveModule(quick): SUBDIRS += qmlscene qmlplugindump
qtHaveModule(qmltest): SUBDIRS += qmltestrunner
SUBDIRS += \
    qmlmin \
    qmlprofiler \
    qmlbundle
qtHaveModule(quick):qtHaveModule(widgets): SUBDIRS += qmleasing
