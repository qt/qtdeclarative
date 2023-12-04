TEMPLATE = app
QT += widgets quick

SOURCES += main.cpp

OTHER_FILES += Main.qml

qml_resources.files = \
   qmldir \
   Main.qml

qml_resources.prefix = /qt/qml/embeddedinwidgets

RESOURCES += qml_resources

target.path = $$[QT_INSTALL_EXAMPLES]/quick/embeddedinwidgets
INSTALLS += target
