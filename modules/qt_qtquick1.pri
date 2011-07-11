QT.qtquick1.VERSION = 5.0.0
QT.qtquick1.MAJOR_VERSION = 5
QT.qtquick1.MINOR_VERSION = 0
QT.qtquick1.PATCH_VERSION = 0

QT.qtquick1.name = QtQuick1
QT.qtquick1.bins = $$QT_MODULE_BIN_BASE
QT.qtquick1.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtQuick1
QT.qtquick1.private_includes = $$QT_MODULE_INCLUDE_BASE/QtQuick1/$$QT.qtquick1.VERSION
QT.qtquick1.sources = $$QT_MODULE_BASE/src/qtquick1
QT.qtquick1.libs = $$QT_MODULE_LIB_BASE
QT.qtquick1.plugins = $$QT_MODULE_PLUGIN_BASE
QT.qtquick1.imports = $$QT_MODULE_IMPORT_BASE
QT.qtquick1.depends = declarative
QT.qtquick1.DEFINES = QT_DECLARATIVE_LIB

QT_CONFIG += qtquick1
