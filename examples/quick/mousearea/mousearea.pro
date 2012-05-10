TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/mousearea
qml.files = mousearea.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/mousearea
INSTALLS += target qml

