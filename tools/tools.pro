TEMPLATE = subdirs
SUBDIRS +=  qmlscene qmlplugindump qmlmin qmleasing qmlprofiler
!contains(QT_CONFIG, no-widgets):SUBDIRS += easingcurveeditor
contains(QT_CONFIG, qmltest): SUBDIRS += qmltestrunner


