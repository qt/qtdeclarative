TEMPLATE = app

QT += quick declarative
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/demos/calculator
qml.files = calculator-desktop.qml calculator-mobile.qml CalculatorCore
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/demos/calculator
INSTALLS += target qml
