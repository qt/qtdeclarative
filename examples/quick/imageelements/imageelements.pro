TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/imageelements
qml.files = *.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/imageelements
sources.files = $$SOURCES imageelements.pro
sources.path = $$qml.path
INSTALLS += sources target qml
