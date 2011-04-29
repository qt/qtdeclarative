QT_DECLARATIVE_VERSION = $$QT_VERSION
QT_DECLARATIVE_MAJOR_VERSION = $$QT_MAJOR_VERSION
QT_DECLARATIVE_MINOR_VERSION = $$QT_MINOR_VERSION
QT_DECLARATIVE_PATCH_VERSION = $$QT_PATCH_VERSION

QT.declarative.name = QtDeclarative
QT.declarative.bins = $$QT_MODULE_BIN_BASE
QT.declarative.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtDeclarative
QT.declarative.private_includes = $$QT_MODULE_INCLUDE_BASE/QtDeclarative/private
QT.declarative.sources = $$QT_MODULE_BASE/src/declarative
QT.declarative.libs = $$QT_MODULE_LIB_BASE
QT.declarative.plugins = $$QT_MODULE_PLUGIN_BASE
QT.declarative.imports = $$QT_MODULE_IMPORT_BASE
QT.declarative.depends = gui script network opengl xmlpatterns
QT.declarative.DEFINES = QT_DECLARATIVE_LIB

QT_CONFIG += declarative
