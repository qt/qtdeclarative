TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/modelviews
qml.files = \
        modelviews.qml \
        gridview \
        listview \
        package \
        parallax \
        pathview \
        visualdatamodel \
        visualitemmodel
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/modelviews
INSTALLS += target qml

