TEMPLATE = app

QT += quick declarative
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/draganddrop
qml.files = draganddrop.qml tiles views
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/draganddrop
INSTALLS += target qml

