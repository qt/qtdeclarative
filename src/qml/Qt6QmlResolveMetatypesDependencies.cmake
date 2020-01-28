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
endwhile()

list(REMOVE_DUPLICATES metatypes_list)
list(JOIN metatypes_list "," metatypes)

if (metatypes)
    file(WRITE ${OUTPUT_FILE} "--foreign-types=${metatypes}\n")
else()
    file(WRITE ${OUTPUT_FILE} "\n")
endif()
