if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "TextBalloon should be built as part of the 'painteditem' project, and not in isolation.")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/customitems/painteditem/TextBalloon")

qt_add_qml_module(qmltextballoon
    VERSION 1.0
    URI "TextBalloon"
    PLUGIN_TARGET qmltextballoon
    SOURCES
        textballoon.cpp textballoon.h
)

target_link_libraries(qmltextballoon PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

install(TARGETS qmltextballoon
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmldir
    DESTINATION "${INSTALL_EXAMPLEDIR}"
)
