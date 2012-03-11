TEMPLATE = app

QT += quick declarative
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/text
qml.files = fonts imgtag styledtext-layout.qml text.qml textselection
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/text
INSTALLS += target qml

