TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/emitters
qml.files = emitters.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/emitters
INSTALLS += target qml

