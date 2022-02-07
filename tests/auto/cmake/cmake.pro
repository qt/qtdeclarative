
# Cause make to do nothing.
TEMPLATE = subdirs

CMAKE_QT_MODULES_UNDER_TEST = quick qml

CONFIG += ctest_testcase

macos:contains(QT_ARCHS, arm64) {
    CMAKE_MODULE_DEFINES += -DQT_SKIP_MACOS_ARM_TESTS=1
}
