TEMPLATE = subdirs
SUBDIRS +=  qmlviewer qmlscene qmlplugindump qmlmin

contains(QT_CONFIG, qmltest): SUBDIRS += qmltestrunner


