TEMPLATE = app

QT += declarative

RESOURCES += stockchart.qrc

HEADERS = model.h
SOURCES = main.cpp model.cpp

macx: CONFIG -= app_bundle
CONFIG += console

OTHER_FILES += \
    stock.qml \
    contents/Stocks.qml \
    contents/ScrollBar.qml \
    contents/TitleBar.qml \
    contents/Button.qml \
    contents/ToolBar.qml

