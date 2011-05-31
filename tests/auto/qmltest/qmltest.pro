TEMPLATE=app
TARGET=tst_qmltest
CONFIG += warn_on qmltestcase
SOURCES += tst_qmltest.cpp

OTHER_FILES += \
    selftests/tst_selftests.qml \
    qdeclarativebinding/tst_binding2.qml \
    qdeclarativebinding/tst_binding.qml \
    selftests/tst_compare.qml \
    selftests/tst_compare_quickobjects.qml

CONFIG+=insignificant_test
