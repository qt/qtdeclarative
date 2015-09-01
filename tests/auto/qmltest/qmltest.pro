TEMPLATE=app
TARGET=tst_qmltest
CONFIG += qmltestcase
CONFIG += console
SOURCES += tst_qmltest.cpp


importFiles.files = borderimage  buttonclick  createbenchmark  events  qqmlbinding selftests

importFiles.path = .
DEPLOYMENT += importFiles

CONFIG+=insignificant_test # QTBUG-33723

