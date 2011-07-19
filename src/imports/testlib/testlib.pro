TARGET  = qmltestplugin
TARGETPATH = QtTest
include(../qimportbase.pri)


CONFIG += qt plugin

symbian {
    CONFIG += epocallowdlldata
    contains(QT_EDITION, OpenSource) {
        TARGET.CAPABILITY = LocalServices NetworkServices ReadUserData UserEnvironment WriteUserData
    } else {
        TARGET.CAPABILITY = All -Tcb
    }

    isEmpty(DESTDIR):importFiles.files = qmltestplugin$${QT_LIBINFIX}.dll qmldir
    else:importFiles.files = $$DESTDIR/qmltestplugin$${QT_LIBINFIX}.dll qmldir
    importFiles.path = $$QT_IMPORTS_BASE_DIR/$$TARGETPATH

    DEPLOYMENT = importFiles

}

QT += declarative script qmltest qmltest-private declarative-private script-private core-private testlib

SOURCES += main.cpp
HEADERS +=

DESTDIR = $$QT.declarative.imports/$$TARGETPATH

target.path += $$[QT_INSTALL_IMPORTS]/QtTest
OTHER_IMPORT_FILES = \
    qmldir \
    TestCase.qml \
    SignalSpy.qml \
    testlogger.js

otherImportFiles.files += $$OTHER_IMPORT_FILES

otherImportFiles.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

copy2build.input = OTHER_IMPORT_FILES
copy2build.output = $$QT.declarative.imports/$$TARGETPATH/${QMAKE_FILE_BASE}${QMAKE_FILE_EXT}


INSTALLS += target otherImportFiles
