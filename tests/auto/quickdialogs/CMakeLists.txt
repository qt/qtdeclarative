if(NOT ANDROID) # temporarily disabled until QTBUG-100991 is fixed
add_subdirectory(qquickfiledialogimpl)
add_subdirectory(qquickfolderdialogimpl)
add_subdirectory(qquickfontdialogimpl)
add_subdirectory(qquickcolordialogimpl)
endif()
add_subdirectory(qquickmessagedialogimpl)
