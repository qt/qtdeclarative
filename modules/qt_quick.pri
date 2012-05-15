QT.quick.VERSION = 5.0.0
QT.quick.MAJOR_VERSION = 5
QT.quick.MINOR_VERSION = 0
QT.quick.PATCH_VERSION = 0

QT.quick.name = QtQuick
QT.quick.bins = $$QT_MODULE_BIN_BASE
QT.quick.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtQuick
QT.quick.private_includes = $$QT_MODULE_INCLUDE_BASE/QtQuick/$$QT.quick.VERSION
QT.quick.sources = $$QT_MODULE_BASE/src/quick
QT.quick.libs = $$QT_MODULE_LIB_BASE
QT.quick.plugins = $$QT_MODULE_PLUGIN_BASE
QT.quick.imports = $$QT_MODULE_IMPORT_BASE
QT.quick.depends = qml gui
QT.quick.DEFINES = QT_QUICK_LIB

QT_CONFIG += quick
