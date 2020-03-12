# Generated from examples.pro.

# special case begin
qt_examples_build_begin()
# special case end

add_subdirectory(qml)
if(TARGET Qt::QuickTest)
    add_subdirectory(qmltest)
endif()
if(TARGET Qt::Quick)
    add_subdirectory(quick)
endif()

# special case begin
qt_examples_build_end()
# special case end
