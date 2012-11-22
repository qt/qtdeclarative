TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/affectors
qml.files = affectors.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/affectors
sources.files = $$SOURCES affectors.pro
sources.path = $$qml.path
INSTALLS += sources target qml
