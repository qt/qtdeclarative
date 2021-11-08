# Generated from chattutorial.pro.

add_subdirectory(chapter1)
add_subdirectory(chapter2)
add_subdirectory(chapter3)
if (TARGET Qt::Sql)
    add_subdirectory(chapter4)
    add_subdirectory(chapter5)
endif()

