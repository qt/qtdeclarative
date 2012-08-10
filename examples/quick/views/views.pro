TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/views
qml.files = \
        views.qml \
        gridview \
        listview \
        package \
        parallax \
        pathview \
        visualdatamodel \
        visualitemmodel
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/views
INSTALLS += target qml
