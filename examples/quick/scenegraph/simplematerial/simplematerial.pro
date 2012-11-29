
QT += quick

SOURCES += \
    simplematerial.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/simplematerial
qml.files = main.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/simplematerial

INSTALLS += target qml
