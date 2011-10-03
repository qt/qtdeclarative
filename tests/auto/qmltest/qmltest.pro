TEMPLATE=app
TARGET=tst_qmltest
CONFIG += warn_on qmltestcase
SOURCES += tst_qmltest.cpp


importFiles.files = borderimage  buttonclick  createbenchmark  events  qdeclarativebinding selftests

importFiles.path = .
DEPLOYMENT += importFiles

CONFIG+=insignificant_test