# Generated from controls.pro.

add_subdirectory(basic)
add_subdirectory(fusion)
add_subdirectory(imagine)
add_subdirectory(material)
add_subdirectory(universal)
if(MACOS)
    add_subdirectory(macos)
    add_subdirectory(ios)
endif()
if(WIN32)
    add_subdirectory(windows)
endif()
