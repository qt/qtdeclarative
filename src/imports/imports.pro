TEMPLATE = subdirs

SUBDIRS += qtquick1 qt47 folderlistmodel particles gestures etcprovider
contains(QT_CONFIG, qmltest): SUBDIRS += testlib

