TEMPLATE = lib
CONFIG += qt plugin
QT += declarative qtquick1

DESTDIR = ImageProviderCore
TARGET  = qmlimageproviderplugin

SOURCES += imageprovider.cpp

sources.files = $$SOURCES imageprovider.qml imageprovider.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/declarative/imageprovider

target.path = $$[QT_INSTALL_EXAMPLES]/declarative/imageprovider/ImageProviderCore

ImageProviderCore_sources.files = \
    ImageProviderCore/qmldir
ImageProviderCore_sources.path = $$[QT_INSTALL_EXAMPLES]/declarative/imageprovider/ImageProviderCore

INSTALLS = sources ImageProviderCore_sources target
