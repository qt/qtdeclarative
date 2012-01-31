TEMPLATE = subdirs
SUBDIRS +=  qmlscene qmlplugindump qmlmin qmleasing

contains(QT_CONFIG, qmltest): SUBDIRS += qmltestrunner


