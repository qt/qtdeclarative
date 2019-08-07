# Generated from highdpi.pro.

#####################################################################
## qmlimageproviderplugin Plugin:
#####################################################################

add_qt_plugin(qmlimageproviderplugin
    OUTPUT_DIRECTORY "ImageProvider"
    SOURCES
        imageprovider.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:highdpi.pro:<TRUE>:
# CONFIG = "plugin"
# INSTALLS = "target" "qml"
# TEMPLATE = "lib"
# qml.files = "ImageProvider/qmldir"
# qml.path = "$$[QT_INSTALL_EXAMPLES]/quick/imageprovider/ImageProvider"
# target.path = "$$[QT_INSTALL_EXAMPLES]/quick/imageprovider/ImageProvider"
