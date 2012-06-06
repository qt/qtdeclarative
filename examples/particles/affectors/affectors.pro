TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/particles/affectors
qml.files = affectors.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/particles/affectors
INSTALLS += target qml

