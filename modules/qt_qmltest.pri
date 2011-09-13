QT.qmltest.VERSION = 5.0.0
QT.qmltest.MAJOR_VERSION = 5
QT.qmltest.MINOR_VERSION = 0
QT.qmltest.PATCH_VERSION = 0

QT.qmltest.name = QtQuickTest
QT.qmltest.bins = $$QT_MODULE_BIN_BASE
QT.qmltest.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtQuickTest
QT.qmltest.private_includes = $$QT_MODULE_INCLUDE_BASE/QtQuickTest/$$QT.qmltest.VERSION
QT.qmltest.sources = $$QT_MODULE_BASE/src/qmltest
QT.qmltest.libs = $$QT_MODULE_LIB_BASE
QT.qmltest.plugins = $$QT_MODULE_PLUGIN_BASE
QT.qmltest.imports = $$QT_MODULE_IMPORT_BASE
QT.qmltest.depends = declarative testlib
QT.qmltest.DEFINES = QT_QMLTEST_LIB

QT_CONFIG += qmltest
