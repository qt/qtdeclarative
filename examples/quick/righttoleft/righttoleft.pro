TEMPLATE = app

QT += quick declarative
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/righttoleft
qml.files = righttoleft.qml layoutdirection layoutmirroring textalignment
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/righttoleft
INSTALLS += target qml

