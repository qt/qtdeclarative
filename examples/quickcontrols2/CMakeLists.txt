# Generated from quickcontrols2.pro.

qt_internal_add_example(gallery)
add_subdirectory(chattutorial)
qt_internal_add_example(texteditor)
qt_internal_add_example(contactlist)
qt_internal_add_example(sidepanel)
qt_internal_add_example(swipetoremove)
qt_internal_add_example(wearable)
qt_internal_add_example(imagine/automotive)
qt_internal_add_example(imagine/musicplayer)
if(TARGET Qt::Widgets)
    qt_internal_add_example(flatstyle)
endif()
