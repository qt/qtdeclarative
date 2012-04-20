TEMPLATE=app
TARGET=tst_qmltest
CONFIG += warn_on qmltestcase
SOURCES += tst_qmltest.cpp


importFiles.files = borderimage  buttonclick  createbenchmark  events  qqmlbinding selftests

importFiles.path = .
DEPLOYMENT += importFiles

mac:CONFIG+=insignificant_test # QTBUG-25306
