TEMPLATE = app
TARGET = tableview_tablemodel
QT += qml quick
SOURCES += main.cpp
RESOURCES += main.qml RowForm.qml

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
