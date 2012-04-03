TEMPLATE = subdirs

SUBDIRS += \
    qtquick2 \
    particles \
    window \
    folderlistmodel \
    localstorage \
    testlib
contains(QT_CONFIG, xmlpatterns) : SUBDIRS += xmllistmodel
