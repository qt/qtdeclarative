!qtConfig(vulkan): error("This example requires Qt built with Vulkan support")

QT += qml quick
CONFIG += qmltypes
QML_IMPORT_NAME = VulkanTextureImport
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += vulkantextureimport.h
SOURCES += vulkantextureimport.cpp main.cpp
RESOURCES += vulkantextureimport.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/vulkantextureimport
INSTALLS += target
