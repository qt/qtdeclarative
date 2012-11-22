TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/system
qml.files = system.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/system
sources.files = $$SOURCES system.pro
sources.path = $$qml.path
INSTALLS += sources target qml
