# Generated from quicktest.pro.

add_subdirectory(polish)
add_subdirectory(signalspy)
add_subdirectory(quicktestmainwithsetup)
if(QT_FEATURE_process)
    add_subdirectory(testfiltering)
endif()
add_subdirectory(testswithcomponents)
