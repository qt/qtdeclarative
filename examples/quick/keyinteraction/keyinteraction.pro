TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/keyinteraction
qml.files = keyinteraction.qml focus
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/keyinteraction
INSTALLS += target qml

