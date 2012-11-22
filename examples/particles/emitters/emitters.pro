TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/emitters
qml.files = emitters.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/emitters
sources.files = $$SOURCES emitters.pro
sources.path = $$qml.path
INSTALLS += sources target qml
