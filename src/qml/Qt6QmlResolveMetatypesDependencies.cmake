#
# Resolve dependencies
#

if (NOT MAIN_DEP_FILE)
    message(FATAL_ERROR "MAIN_DEP_FILE file not specified")
endif()

if (NOT QT_INSTALL_DIR)
    message(FATAL_ERROR "QT_INSTALL_DIR was not specified")
endif()

if (NOT OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE not specified")
endif()

# Read main dep file
if (NOT EXISTS "${MAIN_DEP_FILE}")
    message(FATAL_ERROR "${MAIN_DEP_FILE} does not exist")
endif()

file(STRINGS "${MAIN_DEP_FILE}" remaining)
set(metatypes_list)
set(dep_list)
while(remaining)

    # Pop element off the list
    list(POP_FRONT remaining current)

    string(REPLACE "=" ";" file_list "${current}")
    list(GET file_list 0 metatypes_file)
    list(GET file_list 1 metatypes_dep_file)

    if (NOT IS_ABSOLUTE "${metatypes_file}")
        set(metatypes_file "${QT_INSTALL_DIR}/${metatypes_file}")
        set(metatypes_dep_file "${QT_INSTALL_DIR}/${metatypes_dep_file}")
    endif()

    if (NOT EXISTS "${metatypes_file}")
        message(FATAL_ERROR "${metatypes_file} does not exist")
    endif()
    if (NOT EXISTS "${metatypes_dep_file}")
        message(FATAL_ERROR "${metatypes_dep_file} does not exist")
    endif()

    list(APPEND metatypes_list "${metatypes_file}")

    file(STRINGS ${metatypes_dep_file} dep_string)
    if (dep_string)
        list(APPEND remaining "${dep_string}")
    endif()
    list(APPEND dep_list ${metatypes_dep_file})
endwhile()

list(REMOVE_DUPLICATES metatypes_list)
list(REMOVE_DUPLICATES dep_list)
list(JOIN metatypes_list "," metatypes)

if (metatypes)
    file(WRITE ${OUTPUT_FILE} "--foreign-types=${metatypes}\n")
else()
    file(WRITE ${OUTPUT_FILE} "\n")
endif()


function(gen_dep_file output_file dep_file list)
    file(TO_NATIVE_PATH "${dep_file}" dep_file_native)
    set(contents "${dep_file_native}: ")
    set(native_list)
    foreach(file IN LISTS ${list})
        file(TO_NATIVE_PATH "${file}" file_native)
        list(APPEND native_list "  ${file_native} ")
    endforeach()
    list(APPEND contents ${native_list})
    list(JOIN contents "\\\n" contents_str)
    file(WRITE "${output_file}" "${contents_str}")
endfunction()

if(FOREIGN_TYPES_DEP_FILE AND FOREIGN_TYPES_FILE_NAME)
    gen_dep_file(${FOREIGN_TYPES_DEP_FILE} ${FOREIGN_TYPES_FILE_NAME} dep_list)
endif()

if(CPP_DEP_FILE AND CPP_FILE_NAME)
    gen_dep_file(${CPP_DEP_FILE} ${CPP_FILE_NAME} metatypes_list)
endif()
