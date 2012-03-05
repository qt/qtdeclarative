TEMPLATE = lib
CONFIG += qt plugin
QT += qml

DESTDIR = ImageProviderCore
TARGET  = qmlimageproviderplugin

SOURCES += imageprovider.cpp

OTHER_FILES += imageprovider.json

sources.files = $$SOURCES imageprovider.qml imageprovider.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/imageprovider

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/imageprovider/ImageProviderCore

ImageProviderCore_sources.files = \
    ImageProviderCore/qmldir 
ImageProviderCore_sources.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/imageprovider/ImageProviderCore

INSTALLS = sources ImageProviderCore_sources target

