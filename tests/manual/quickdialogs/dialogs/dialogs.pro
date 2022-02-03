TEMPLATE = app
TARGET = dialogs
QT += qml quickcontrols2

SOURCES += dialogs.cpp
RESOURCES += \
    dialogs.qml \
    ColorDialogPage.qml \
    FileDialogPage.qml \
    FolderDialogPage.qml \
    FontDialogPage.qml \
    MessageBoxPage.qml \
    StringListView.qml \
    qmldir \
    Theme.qml
