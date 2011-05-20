TEMPLATE = subdirs

SUBDIRS += folderlistmodel particles gestures inputcontext etcprovider
contains(QT_CONFIG, qmltest): SUBDIRS += testlib

