TEMPLATE=app
TARGET=tst_qmltestexample
CONFIG += qmltestcase
SOURCES += tst_qmltest.cpp

# Note: Normally, tests are auto-installed to a test-specific directory. Overwritten here
# so this one will end up in the examples tree.
target.path = $$[QT_INSTALL_EXAMPLES]/qmltest
qml.files = tst_basic.qml tst_item.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qmltest
INSTALLS += target qml
