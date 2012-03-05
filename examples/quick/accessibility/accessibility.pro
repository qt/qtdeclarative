TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/accessibility
qml.files = accessibility.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/accessibility
INSTALLS += target qml

