QT       += core gui quick widgets quickwidgets

TARGET = quickwidget
TEMPLATE = app

SOURCES += main.cpp

RESOURCES += quickwidget.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/quickwidgets/quickwidget
INSTALLS += target
