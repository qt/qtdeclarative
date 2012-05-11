TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/demos/tweetsearch
qml.files = tweetsearch.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/demos/tweetsearch
INSTALLS += target qml
