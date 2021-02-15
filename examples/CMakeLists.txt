qt_examples_build_begin(EXTERNAL_BUILD)

add_subdirectory(qml)
if(TARGET Qt::QuickTest)
    add_subdirectory(qmltest)
endif()
if(TARGET Qt::Quick)
    add_subdirectory(quick)
endif()

qt_examples_build_end()
