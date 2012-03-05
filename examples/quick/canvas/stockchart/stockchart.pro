TEMPLATE = lib
CONFIG += qt plugin
QT += qml network

DESTDIR = com/nokia/StockChartExample
TARGET  = qmlstockchartexampleplugin

SOURCES += model.cpp  plugin.cpp
HEADERS += model.h
qdeclarativesources.files += \
    com/nokia/StockChartExample/qmldir \
    stock.qml

qdeclarativesources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/plugins/com/nokia/StockChartExample

sources.files += stockchart.pro model.h model.cpp plugin.cpp README
sources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/plugins
target.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/plugins/com/nokia/StockChartExample

INSTALLS += qdeclarativesources sources target
