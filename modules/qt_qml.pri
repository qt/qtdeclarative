QT.qml.VERSION = 5.0.0
QT.qml.MAJOR_VERSION = 5
QT.qml.MINOR_VERSION = 0
QT.qml.PATCH_VERSION = 0

QT.qml.name = QtQml
QT.qml.bins = $$QT_MODULE_BIN_BASE
QT.qml.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtQml
QT.qml.private_includes = $$QT_MODULE_INCLUDE_BASE/QtQml/$$QT.qml.VERSION
QT.qml.sources = $$QT_MODULE_BASE/src/qml
QT.qml.libs = $$QT_MODULE_LIB_BASE
QT.qml.plugins = $$QT_MODULE_PLUGIN_BASE
QT.qml.imports = $$QT_MODULE_IMPORT_BASE
QT.qml.depends = gui network
QT.qml.DEFINES = QT_QML_LIB QQmlImageProvider=QQuickImageProvider

QT_CONFIG += qml
