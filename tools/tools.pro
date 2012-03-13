TEMPLATE = subdirs
SUBDIRS +=  qmlscene qmlplugindump qmlmin qmleasing qmlprofiler easingcurveeditor

contains(QT_CONFIG, qmltest): SUBDIRS += qmltestrunner


