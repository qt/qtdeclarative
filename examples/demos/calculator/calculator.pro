TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/demos/calculator
qml.files = calculator-desktop.qml calculator-mobile.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/demos/calculator
INSTALLS += target qml
