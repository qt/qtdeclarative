# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(QmltcTests)
add_subdirectory(NamespaceTest/Subfolder)
add_subdirectory(QmltcExportedTests)

set(test_sources
    nameconflict.h nameconflict.cpp
    tst_qmltc.h tst_qmltc.cpp
)

set(qmltc_module_libs
    qmltc_test_module
    # We need to link against a <module>plugin here for the following:
    # - Properly see `import QmltcTests` (that could be resolved differently
    #   but this works as well)
    # - Properly see C++ types exposed to QML in the engine (we need C++
    #   automatic type registration that comes from the plugin)
    qmltc_test_moduleplugin
    qmltc_namespace_test_module
    qmltc_namespace_test_moduleplugin
    qmltc_exported_tests_module
    qmltc_exported_tests_moduleplugin
)
qt_internal_add_test(tst_qmltc_diskcache
    SOURCES ${test_sources}
    LIBRARIES ${qmltc_module_libs}
)
target_compile_definitions(tst_qmltc_diskcache PRIVATE
    QMLTC_TESTS_DISABLE_CACHE=0
)

qt_internal_add_test(tst_qmltc_nodiskcache
    SOURCES ${test_sources}
    LIBRARIES ${qmltc_module_libs}
)
target_compile_definitions(tst_qmltc_nodiskcache PRIVATE
    QMLTC_TESTS_DISABLE_CACHE=1
)

# Add qmltc documentation example to the tests. This is not beautiful but allows
# to nicely test the documentation snippets automatically (and so making sure
# the code there is up-to-date)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/qml/doc/snippets/qmltc snippets)
