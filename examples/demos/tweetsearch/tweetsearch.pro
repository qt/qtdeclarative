TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/demos/tweetsearch
qml.files = tweetsearch.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/demos/tweetsearch
INSTALLS += target qml
