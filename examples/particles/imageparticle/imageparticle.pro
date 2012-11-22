TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/imageparticle
qml.files = imageparticle.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/imageparticle
sources.files = $$SOURCES imageparticle.pro
sources.path = $$qml.path
INSTALLS += sources target qml
