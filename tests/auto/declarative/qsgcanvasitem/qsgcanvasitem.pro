QT += core-private gui-private declarative-private widgets
TEMPLATE=app
TARGET=tst_qsgcanvasitem

CONFIG += warn_on qmltestcase
SOURCES += tst_qsgcanvasitem.cpp

importFiles.files = data
importFiles.path = .
DEPLOYMENT += importFiles
