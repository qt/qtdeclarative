TARGET  = qmlgesturesplugin
TARGETPATH = Qt/labs/gestures
include(../qimportbase.pri)

QT += core-private gui-private declarative-private qtquick1 qtquick1-private widgets-private v8-private

SOURCES += qdeclarativegesturearea.cpp plugin.cpp
HEADERS += qdeclarativegesturearea_p.h

DESTDIR = $$QT.declarative.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir
