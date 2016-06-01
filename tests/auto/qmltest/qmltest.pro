TEMPLATE=app
TARGET=tst_qmltest
CONFIG += qmltestcase
CONFIG += console
SOURCES += tst_qmltest.cpp


importFiles.files = borderimage  buttonclick  createbenchmark  events  qqmlbinding selftests

importFiles.path = .
DEPLOYMENT += importFiles

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
# Please do not make this test insignificant again, thanks.
# Just skip those unstable ones. See also QTBUG-33723.
