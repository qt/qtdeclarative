TEMPLATE = app

QT += quick qml

CONFIG += qmltypes
QML_IMPORT_NAME = GameOfLifeModel
QML_IMPORT_MAJOR_VERSION = 1

SOURCES += \
        main.cpp \
    gameoflifemodel.cpp

RESOURCES += \
    main.qml \
    gosperglidergun.cells

target.path = $$[QT_INSTALL_EXAMPLES]/quick/tableview/gameoflife
INSTALLS += target

HEADERS += \
    gameoflifemodel.h
