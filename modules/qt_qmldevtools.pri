QT.qmldevtools.VERSION = 5.0.0
QT.qmldevtools.MAJOR_VERSION = 5
QT.qmldevtools.MINOR_VERSION = 0
QT.qmldevtools.PATCH_VERSION = 0

QT.qmldevtools.name = QtQmlDevTools
QT.qmldevtools.bins = $$QT_MODULE_BIN_BASE
QT.qmldevtools.includes = $$QT_MODULE_INCLUDE_BASE/QtQmlDevTools
QT.qmldevtools.private_includes = $$QT_MODULE_INCLUDE_BASE/QtQmlDevTools/$$QT.qmldevtools.VERSION
QT.qmldevtools.sources = $$QT_MODULE_BASE/src/qmldevtools
QT.qmldevtools.libs = $$QT_MODULE_LIB_BASE
QT.qmldevtools.plugins = $$QT_MODULE_PLUGIN_BASE
QT.qmldevtools.imports = $$QT_MODULE_IMPORT_BASE
QT.qmldevtools.depends = core
QT.qmldevtools.module_config = staticlib
QT.qmldevtools.DEFINES = QT_QMLDEVTOOLS_LIB

QT_CONFIG += qmldevtools

