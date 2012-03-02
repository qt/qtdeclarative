TEMPLATE = app

QT += quick declarative
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/positioners
qml.files = positioners.qml positioners-transitions.qml positioners-attachedproperties.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/positioners
INSTALLS += target qml

