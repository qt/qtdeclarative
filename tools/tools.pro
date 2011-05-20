TEMPLATE = subdirs
SUBDIRS +=  qmlviewer qmlscene qmlplugindump
contains(QT_CONFIG, quicktest): SUBDIRS += qmltestrunner

