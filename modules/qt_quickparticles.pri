QT.quickparticles.VERSION = 5.0.0
QT.quickparticles.MAJOR_VERSION = 5
QT.quickparticles.MINOR_VERSION = 0
QT.quickparticles.PATCH_VERSION = 0

QT.quickparticles.name = QtQuickParticles
QT.quickparticles.bins = $$QT_MODULE_BIN_BASE
QT.quickparticles.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtQuickParticles
QT.quickparticles.private_includes = $$QT_MODULE_INCLUDE_BASE/QtQuickParticles/$$QT.quickparticles.VERSION
QT.quickparticles.sources = $$QT_MODULE_BASE/src/particles
QT.quickparticles.libs = $$QT_MODULE_LIB_BASE
QT.quickparticles.plugins = $$QT_MODULE_PLUGIN_BASE
QT.quickparticles.imports = $$QT_MODULE_IMPORT_BASE
QT.quickparticles.depends = qml quick
QT.quickparticles.DEFINES = QT_QUICKPARTICLES_LIB

QT_CONFIG += quickparticles
