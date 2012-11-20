TEMPLATE = subdirs
SUBDIRS += \
    qmlscene \
    qmlplugindump \
    qmlmin \
    qmlprofiler \
    qmlbundle \
    qmltestrunner
!contains(QT_CONFIG, no-widgets):SUBDIRS += qmleasing


