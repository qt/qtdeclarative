TEMPLATE = subdirs
SUBDIRS +=  qmlviewer qmlscene qmlplugindump qmlmin qmleasing

contains(QT_CONFIG, qmltest): SUBDIRS += qmltestrunner


