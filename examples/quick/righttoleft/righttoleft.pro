TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/righttoleft
qml.files = righttoleft.qml layoutdirection layoutmirroring textalignment
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/righttoleft
INSTALLS += target qml

