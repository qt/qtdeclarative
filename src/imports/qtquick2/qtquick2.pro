TARGET  = qtquick2plugin
TARGETPATH = QtQuick.2
include(../qimportbase.pri)

SOURCES += \
    plugin.cpp

QT += quick-private declarative-private

OTHER_FILES += \
    qmldir

DESTDIR = $$QT.declarative.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir
