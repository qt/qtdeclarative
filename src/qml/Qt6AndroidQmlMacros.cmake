# The function collects qml root paths and sets the QT_QML_ROOT_PATH property to the ${target}
# based on the provided qml source files.
function(_qt_internal_collect_qml_root_paths target)
    get_target_property(qml_root_paths ${target} QT_QML_ROOT_PATH)
    if(NOT qml_root_paths)
        set(qml_root_paths "")
    endif()
    foreach(file IN LISTS ARGN)
        get_filename_component(extension "${file}" LAST_EXT)
        if(NOT extension STREQUAL ".qml")
            continue()
        endif()

        get_filename_component(dir "${file}" DIRECTORY)
        get_filename_component(absolute_dir "${dir}" ABSOLUTE)
        list(APPEND qml_root_paths "${absolute_dir}")
    endforeach()

    list(REMOVE_DUPLICATES qml_root_paths)
    set_target_properties(${target} PROPERTIES QT_QML_ROOT_PATH "${qml_root_paths}")
endfunction()
