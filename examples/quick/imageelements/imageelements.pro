TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/imageelements
qml.files = borderimage.qml content imageelements.qml image.qml shadows.qml simplesprite.qml spriteimage.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/imageelements
INSTALLS += target qml

