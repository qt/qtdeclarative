TEMPLATE = subdirs
SUBDIRS += \
    qmlscene \
    qmlplugindump \
    qmlmin \
    qmleasing \
    qmlprofiler \
    qmlbundle \
    qmltestrunner
!contains(QT_CONFIG, no-widgets):SUBDIRS += easingcurveeditor


