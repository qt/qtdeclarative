TARGET  = qmltestplugin
TARGETPATH = QtTest
include(../qimportbase.pri)

CONFIG += qt plugin

QT += qml quick qmltest qmltest-private v8-private qml-private core-private testlib

SOURCES += main.cpp

OTHER_FILES += testlib.json

DESTDIR = $$QT.qml.imports/$$TARGETPATH

target.path += $$[QT_INSTALL_IMPORTS]/QtTest
OTHER_IMPORT_FILES = \
    qmldir \
    TestCase.qml \
    SignalSpy.qml \
    testlogger.js

otherImportFiles.files += $$OTHER_IMPORT_FILES

otherImportFiles.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

copy2build.input = OTHER_IMPORT_FILES
copy2build.output = $$QT.qml.imports/$$TARGETPATH/${QMAKE_FILE_BASE}${QMAKE_FILE_EXT}


INSTALLS += target otherImportFiles
