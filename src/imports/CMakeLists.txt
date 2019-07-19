# Generated from imports.pro.

add_subdirectory(controls)
add_subdirectory(calendar)
add_subdirectory(platform)
add_subdirectory(templates)

# special case begin
# NOTE: The pro2cmake project will emebed the projects from controls/fusion,
# controls/imagine, controls/material and controls/universal in this
# CMakeLists.txt. This is not the correct behavior and should be removed.
# A special case has been added in controls' CMakeLists.txt to address this.
# special case end
