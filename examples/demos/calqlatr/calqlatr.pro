TEMPLATE = app

QT += qml quick
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/demos/calqlatr
qml.files = calqlatr.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/demos/calqlatr
sources.files = $$SOURCES calqlatr.pro
sources.path = $$qml.path
INSTALLS += target sources qml
