!build_pass {
    # Create a header containing a hash that describes this library.  For a
    # released version of Qt, we'll use the .tag file that is updated by git
    # archive with the commit hash. For unreleased versions, we'll ask git
    # describe. Note that it won't update unless qmake is run again, even if
    # the commit change also changed something in this library.
    tagFile = $$PWD/../../.tag
    tag =
    exists($$tagFile) {
        tag = $$cat($$tagFile, singleline)
        QMAKE_INTERNAL_INCLUDED_FILES += $$tagFile
    }
    !equals(tag, "$${LITERAL_DOLLAR}Format:%H$${LITERAL_DOLLAR}") {
        QML_COMPILE_HASH = $$tag
    } else:exists($$PWD/../../.git) {
        commit = $$system(git rev-parse HEAD)
        QML_COMPILE_HASH = $$commit
    }
    compile_hash_contents = \
        "// Generated file, DO NOT EDIT" \
        "$${LITERAL_HASH}define QML_COMPILE_HASH \"$$QML_COMPILE_HASH\"" \
        "$${LITERAL_HASH}define QML_COMPILE_HASH_LENGTH $$str_size($$QML_COMPILE_HASH)"
    write_file("$$OUT_PWD/qml_compile_hash_p.h", compile_hash_contents)|error()
}

HEADERS += \
    $$PWD/qqmlapiversion_p.h \
    $$PWD/qqmljsdiagnosticmessage_p.h \
    $$PWD/qqmljsfixedpoolarray_p.h \
    $$PWD/qqmljsmemorypool_p.h \
    $$PWD/qqmljssourcelocation_p.h \
    $$PWD/qv4alloca_p.h \
    $$PWD/qv4calldata_p.h \
    $$PWD/qv4compileddata_p.h \
    $$PWD/qv4staticvalue_p.h \
    $$PWD/qv4stringtoarrayindex_p.h
