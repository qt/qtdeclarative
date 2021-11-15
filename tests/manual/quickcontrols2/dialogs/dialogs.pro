TEMPLATE = app
TARGET = dialogs
QT += qml quickcontrols2

SOURCES += dialogs.cpp
RESOURCES += \
    dialogs.qml \
    FileDialogPage.qml \
    FontDialogPage.qml \
    MessageBoxPage.qml \
    StringListView.qml \
    qmldir \
    Theme.qml
