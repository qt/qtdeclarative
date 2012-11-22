TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/shadereffects
qml.files = shadereffects.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/shadereffects
sources.files = $$SOURCES shadereffects.pro
sources.path = $$qml.path
INSTALLS += sources target qml
