set(cpp_sources
    tst_qmltc.h tst_qmltc.cpp

    # test files:
    nameconflict.h nameconflict.cpp
)

set(qml_sources
    data/HelloWorld.qml
    data/NameConflict.qml
    data/simpleQtQuickTypes.qml
    data/typeWithEnums.qml
    data/methods.qml
)

set_source_files_properties(data/NameConflict.qml PROPERTIES
    QT_QMLTC_FILE_BASENAME ResolvedNameConflict)

qt_internal_add_test(tst_qmltc_diskcache
    SOURCES
        ${cpp_sources}
    LIBRARIES
        Qt::Core
        Qt::QmlPrivate
        Qt::QuickPrivate
)
qt6_add_qml_module(tst_qmltc_diskcache
    VERSION 1.0
    URI QmltcTests
    QML_FILES
        ${qml_sources}
    OUTPUT_DIRECTORY diskcache/QmltcTests
)
target_compile_definitions(tst_qmltc_diskcache PRIVATE
    QMLTC_TESTS_DISABLE_CACHE=0
)
qt_internal_target_compile_qml_to_cpp(tst_qmltc_diskcache
    NAMESPACE QmltcTest
    FILES
        ${qml_sources}
)

qt_internal_add_test(tst_qmltc_nodiskcache
    SOURCES
        ${cpp_sources}
    LIBRARIES
        Qt::Core
        Qt::QmlPrivate
        Qt::QuickPrivate
)
qt6_add_qml_module(tst_qmltc_nodiskcache
    VERSION 1.0
    URI QmltcTests
    QML_FILES
        ${qml_sources}
    OUTPUT_DIRECTORY nodiskcache/QmltcTests
)
target_compile_definitions(tst_qmltc_nodiskcache PRIVATE
    QMLTC_TESTS_DISABLE_CACHE=1
)
qt_internal_target_compile_qml_to_cpp(tst_qmltc_nodiskcache
    # NAMESPACE QmltcTest # use QT_NAMESPACE instead
    FILES
        ${qml_sources}
)
