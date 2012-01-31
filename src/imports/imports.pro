TEMPLATE = subdirs

SUBDIRS += qtquick2 folderlistmodel etcprovider localstorage
contains(QT_CONFIG, qmltest): SUBDIRS += testlib
contains(QT_CONFIG, xmlpatterns) : SUBDIRS += xmllistmodel
