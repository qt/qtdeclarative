TEMPLATE = app

QT += qml quick
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/demos/samegame
qml.files = samegame-desktop.qml samegame-mobile.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/demos/samegame
INSTALLS += target qml
