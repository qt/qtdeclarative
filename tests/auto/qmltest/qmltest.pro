TEMPLATE=app
TARGET=tst_qmltest
CONFIG += warn_on qmltestcase
SOURCES += tst_qmltest.cpp

OTHER_FILES += \
    selftests/tst_selftests.qml \
    qdecarativebinding/tst_binding2.qml \
    qdecarativebinding/tst_binding.qml \
    selftests/tst_compare.qml \
    selftests/tst_compare_quickobjects.qml

CONFIG+=insignificant_test
