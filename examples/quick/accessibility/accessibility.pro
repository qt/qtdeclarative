TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/accessibility
qml.files = accessibility.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/accessibility
sources.files = $$SOURCES accessibility.pro
sources.path = $$qml.path
INSTALLS += sources target qml
