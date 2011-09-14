TEMPLATE = subdirs

SUBDIRS += qtquick1 qt47 folderlistmodel particles gestures inputcontext etcprovider
contains(QT_CONFIG, qmltest): SUBDIRS += testlib

