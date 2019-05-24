# Generated from examples.pro.

add_subdirectory(qml)

if(TARGET Qt::QuickTest)
    add_subdirectory(qmltest)
endif()

#if(TARGET Qt::Quick)
#add_subdirectory(quick)
#endif()
