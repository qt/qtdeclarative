# Generated from qqmlpromise.pro.

#####################################################################
## tst_qqmlpromise Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmlpromise
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlpromise.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmlpromise.pro:<TRUE>:
# CONFIG = "testcase"
# DISTFILES = "data/then-fulfilled-non-callable.qml" "data/then-reject-non-callable.qml" "data/then-resolve-multiple-then.qml" "data/then-resolve-chaining.qml" "data/then-reject-chaining.qml" "data/promise-resolve-with-value.qml" "data/promise-resolve-with-promise.qml" "data/promise-reject-with-value.qml" "data/promise-executor-resolve.qml" "data/promise-get-length.qml" "data/promise-executor-reject.qml" "data/promise-reject-catch.qml" "data/promise-async-resolve-with-value.qml" "data/promise-async-reject-with-value.qml" "data/promise-all-resolve.qml" "data/promise-all-empty-input.qml" "data/promise-resolve-with-array.qml" "data/promise-all-reject-reject-is-last.qml" "data/promise-all-reject-reject-is-mid.qml" "data/promise-race-resolve-1st.qml" "data/promise-race-empty-input.qml" "data/promise-race-resolve-2nd.qml" "data/promise-race-resolve-1st-in-executor-function.qml" "data/promise-resolve-is-a-function.qml" "data/promise-resolve-function-length.qml" "data/promise-all-invoke-then-method.qml" "data/promise-resolve-with-empty.qml" "data/promise-executor-throw-exception.qml" "data/promise-executor-function-extensible.qml" "data/promise-all-noniterable-input.qml"
# OTHER_FILES = <EMPTY>

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmlpromise.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlpromise CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlpromise CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
