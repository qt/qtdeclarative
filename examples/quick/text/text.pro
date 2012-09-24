TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/text
qml.files = fonts imgtag styledtext-layout.qml text.qml textselection
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/text
INSTALLS += target qml

