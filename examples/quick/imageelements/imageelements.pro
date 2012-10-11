TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/imageelements
qml.files = *.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/imageelements
INSTALLS += target qml

