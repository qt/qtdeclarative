TEMPLATE = lib
CONFIG += qt plugin
QT += qml quick

DESTDIR = ImageProviderCore
TARGET  = qmlimageproviderplugin

SOURCES += imageprovider.cpp

EXAMPLE_FILES = imageprovider-example.qml

target.path = $$[QT_INSTALL_EXAMPLES]/qml/imageprovider/ImageProviderCore
qml.files = ImageProviderCore/qmldir
qml.path = $$[QT_INSTALL_EXAMPLES]/qml/imageprovider/ImageProviderCore
INSTALLS = target qml
