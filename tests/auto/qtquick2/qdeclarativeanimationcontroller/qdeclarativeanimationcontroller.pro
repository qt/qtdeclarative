QT += core-private gui-private declarative-private
TEMPLATE=app
TARGET=tst_qdeclarativeanimationcontroller

CONFIG += warn_on qmltestcase
SOURCES += tst_qdeclarativeanimationcontroller.cpp

importFiles.files = data
importFiles.path = .
DEPLOYMENT += importFiles
