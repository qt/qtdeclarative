TEMPLATE = subdirs
SUBDIRS += \
    qmlscene \
    qmlplugindump \
    qmlmin \
    qmlprofiler \
    qmlbundle \
    qmltestrunner
qtHaveModule(widgets): SUBDIRS += qmleasing


