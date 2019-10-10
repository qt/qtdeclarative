!qtConfig(vulkan): error("This example requires Qt built with Vulkan support")

QT += qml quick

HEADERS += vulkansquircle.h
SOURCES += vulkansquircle.cpp main.cpp
RESOURCES += vulkanunderqml.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/vulkanunderqml
INSTALLS += target
