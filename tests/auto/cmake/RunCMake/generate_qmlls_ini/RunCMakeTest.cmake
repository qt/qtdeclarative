# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtRunCMake)

function(check_qmlls_ini expectedBuild)
    set(sourceQmllsIni "${RunCMake_SOURCE_DIR}/.qmlls.ini")

    file(READ "${sourceQmllsIni}" qmllsIniContent)

    string(FIND "${qmllsIniContent}" "${expectedBuild}" result)
    if(result EQUAL -1)
        message(FATAL_ERROR "Test failed: qmlls.ini does not contain expected ${expectedBuild} "
            "in its content:\n${qmllsIniContent}")
    endif()
endfunction()

function(buildProject name build)
    set(RunCMake_TEST_BINARY_DIR "${build}")
    run_cmake_command(${name} ${CMAKE_COMMAND} --build ${build})
endfunction()

set(build1 "${RunCMake_BINARY_DIR}/MyApplication-build1/generate_qmlls_ini")
set(build2 "${RunCMake_BINARY_DIR}/MyApplication-build2/generate_qmlls_ini")
set(expectedBuild1 "${RunCMake_BINARY_DIR}/MyApplication-build1")
set(expectedBuild2 "${RunCMake_BINARY_DIR}/MyApplication-build2")

# configure and build the project into build1
set(RunCMake_TEST_BINARY_DIR "${build1}")
run_cmake_with_options(
    configuration1
    "-DQT_QML_GENERATE_QMLLS_INI=ON"
    "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
)
set(RunCMake_TEST_NO_CLEAN 1)
set(RunCMake_TEST_OUTPUT_MERGE 1)
buildProject(build1 "${build1}")

# check qmlls.ini
check_qmlls_ini("${expectedBuild1}")

# configure the project again into build2
set(RunCMake_TEST_BINARY_DIR "${build2}")
run_cmake_with_options(
    configuration2
    "-DQT_QML_GENERATE_QMLLS_INI=ON"
    "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
)
buildProject(build2 "${build2}")
check_qmlls_ini("${expectedBuild2}")

#rebuild in previous build folder and check that qmlls gets updated:
buildProject(build1again "${build1}")
check_qmlls_ini("${expectedBuild1}")

buildProject(build2again "${build2}")
check_qmlls_ini("${expectedBuild2}")
