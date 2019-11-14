# Generated from scenegraph.pro.

#####################################################################
## tst_scenegraph Test:
#####################################################################

# Collect test data
list(APPEND test_data "../data")

qt_add_test(tst_scenegraph
    OUTPUT_DIRECTORY ".."
    SOURCES
        ../../../../../qtbase/tests/baselineserver/shared/baselineprotocol.cpp ../../../../../qtbase/tests/baselineserver/shared/baselineprotocol.h
        ../../../../../qtbase/tests/baselineserver/shared/lookup3.cpp
        ../../../../../qtbase/tests/baselineserver/shared/qbaselinetest.cpp ../../../../../qtbase/tests/baselineserver/shared/qbaselinetest.h
        tst_scenegraph.cpp
    DEFINES
        QMAKESPEC=\\\"\\\"
    INCLUDE_DIRECTORIES
        ../../../../../qtbase/tests/baselineserver/shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:scenegraph.pro:<TRUE>:
# TEST_HELPER_INSTALLS = ".././qmlscenegrabber"

## Scopes:
#####################################################################

#### Keys ignored in scope 4:.:../../../../../qtbase/tests/baselineserver/shared:../../../../../qtbase/tests/baselineserver/shared/qbaselinetest.pri:WIN32:
# MKSPEC = "$$replace(QMAKESPEC, \\\\, /)"

#### Keys ignored in scope 5:.:../../../../../qtbase/tests/baselineserver/shared:../../../../../qtbase/tests/baselineserver/shared/qbaselinetest.pri:else:
# MKSPEC = "$$QMAKESPEC"
