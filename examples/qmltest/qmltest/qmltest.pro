TEMPLATE=app
TARGET=tst_qmltestexample
CONFIG += qmltestcase
SOURCES += tst_qmltest.cpp

# Note: Normally, tests are auto-installed to a test-specific directory. Overwritten here
# so this one will end up in the examples tree.
target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qmltest
sources.files = $$SOURCES qmltest.pro *.qml
sources.path = $$target.path
INSTALLS = sources target
