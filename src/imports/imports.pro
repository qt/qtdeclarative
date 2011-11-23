TEMPLATE = subdirs

SUBDIRS += qtquick2 qtquick1 qt47 folderlistmodel particles gestures etcprovider
contains(QT_CONFIG, qmltest): SUBDIRS += testlib
contains(QT_CONFIG, xmlpatterns) : SUBDIRS += xmllistmodel
