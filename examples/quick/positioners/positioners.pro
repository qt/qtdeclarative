TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/positioners
qml.files = positioners.qml positioners-transitions.qml positioners-attachedproperties.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/positioners
INSTALLS += target qml
