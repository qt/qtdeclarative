TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/modelviews
qml.files = \
        modelviews.qml \
        gridview \
        listview \
        package \
        parallax \
        pathview \
        visualdatamodel \
        visualitemmodel
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/modelviews
INSTALLS += target qml

