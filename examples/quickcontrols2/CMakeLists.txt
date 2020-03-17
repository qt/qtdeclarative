# Generated from quickcontrols2.pro.

add_subdirectory(gallery)
add_subdirectory(chattutorial)
add_subdirectory(texteditor)
add_subdirectory(contactlist)
add_subdirectory(sidepanel)
add_subdirectory(swipetoremove)
add_subdirectory(wearable)
add_subdirectory(imagine/automotive)
add_subdirectory(imagine/musicplayer)
if(TARGET Qt::Widgets)
    add_subdirectory(flatstyle)
endif()
