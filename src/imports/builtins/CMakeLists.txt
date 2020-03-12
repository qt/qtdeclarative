# Generated from builtins.pro.

# special case begin

set(qml_type_file builtins.qmltypes)

qt_copy_or_install(FILES ${qml_type_file}
    DESTINATION ${INSTALL_QMLDIR}
)

if (NOT CMAKE_CROSSCOMPILING)

    add_custom_target(builtins_qmltypes
        COMMAND qmlplugindump -builtins > ${CMAKE_CURRENT_SOURCE_DIR}/${qml_type_file}
    )
endif()

# special case end
