TEMPLATE = subdirs
SUBDIRS +=  qmlviewer qmlscene qmlplugindump

contains(QT_CONFIG, qmltest): SUBDIRS += qmltestrunner

