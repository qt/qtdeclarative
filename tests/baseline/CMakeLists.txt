if(NOT TARGET Qt::qsb)
    return()
endif()

# Special case: test includes the QBaselineTest module sources from qtbase directly
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../qtbase/tests/baseline/shared")
    add_subdirectory(scenegraph)
    add_subdirectory(controls)
endif()
