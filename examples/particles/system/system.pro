TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/particles/system
qml.files = system.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/particles/system
INSTALLS += target qml

