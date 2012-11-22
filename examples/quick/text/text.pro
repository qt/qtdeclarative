TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/text
qml.files = fonts imgtag styledtext-layout.qml text.qml textselection
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/text
sources.files = $$SOURCES text.pro
sources.path = $$qml.path
INSTALLS += sources target qml
