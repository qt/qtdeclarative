TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/affectors
qml.files = affectors.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/affectors
INSTALLS += target qml

