TARGET  = qmltestplugin
TARGETPATH = QtTest
include(../qimportbase.pri)

CONFIG += qt plugin

QT += declarative quick qmltest qmltest-private v8-private declarative-private core-private testlib

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
