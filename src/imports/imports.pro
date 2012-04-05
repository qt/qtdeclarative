TEMPLATE = subdirs

SUBDIRS += qtquick2 particles window folderlistmodel localstorage
contains(QT_CONFIG, qmltest): SUBDIRS += testlib
contains(QT_CONFIG, xmlpatterns) : SUBDIRS += xmllistmodel
