# Generated from chattutorial.pro.

add_subdirectory(chapter1-settingup)
add_subdirectory(chapter2-lists)
add_subdirectory(chapter3-navigation)
if (TARGET Qt::Sql)
    add_subdirectory(chapter4-models)
    add_subdirectory(chapter5-styling)
endif()

