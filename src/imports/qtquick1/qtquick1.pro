TARGET  = qtquick1plugin
TARGETPATH = QtQuick.1
include(../qimportbase.pri)

SOURCES += \
    plugin.cpp

QT += declarative qtquick1 opengl qtquick1-private


OTHER_FILES += \
    qmldir

DESTDIR = $$QT.declarative.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

symbian:{
#    TARGET.UID3 = 
    
    isEmpty(DESTDIR):importFiles.files = qtquick1plugin$${QT_LIBINFIX}.dll qmldir
    else:importFiles.files = $$DESTDIR/qtquick1plugin$${QT_LIBINFIX}.dll qmldir
    importFiles.path = $$QT_IMPORTS_BASE_DIR/$$TARGETPATH
    
    DEPLOYMENT = importFiles
}

INSTALLS += target qmldir
