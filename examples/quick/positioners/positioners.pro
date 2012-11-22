TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/positioners
qml.files = positioners.qml positioners-transitions.qml positioners-attachedproperties.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/positioners
sources.files = $$SOURCES positioners.pro
sources.path = $$qml.path
INSTALLS += sources target qml
