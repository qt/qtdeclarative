# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(deployment_api)
enable_testing()

find_package(Qt6 COMPONENTS REQUIRED Quick Test QuickTestUtilsPrivate)

qt6_standard_project_setup()

function(create_test_executable target)
    cmake_parse_arguments(arg "" "" "" ${ARGN})

    if(CMAKE_VERSION VERSION_LESS "3.19")
        qt_add_executable(${target} MANUAL_FINALIZATION main.cpp)
    else()
        qt_add_executable(${target} main.cpp)
    endif()

    qt_add_qml_module(${target}
        URI EntryModule
        VERSION 1.0
        QML_FILES main.qml
        NO_RESOURCE_TARGET_PATH
    )

    set_target_properties(${target} PROPERTIES
        # We explicitly don't set WIN32_EXECUTABLE to ensure we see errors from stderr when
        # something fails and not having to use DebugView.

        MACOSX_BUNDLE TRUE
    )
    target_link_libraries(${target} PRIVATE
        Qt::Quick
        Qt::Test
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    )

    # Skip installation on iOS, because ctest's --build-and-test only builds the Debug
    # configuration by default, whereas installation installs both Release and Debug, and
    # thus fails (due to missing Release libraries)
    if(NOT IOS)
        install(TARGETS ${target}
            BUNDLE  DESTINATION .
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()

    # Need to manually install the backing libraries on Windows because windeployqt does not do it.
    # On macOS, macdeployqt takes care of installing the backing library because it is detected
    # as a dependency of the qml plugin, which is specified via the -executable option
    # of macdeployqt.
    # Note we install all the binaries and backing libraries before calling the deploy tool.
    if(WIN32)
        install(TARGETS ShapesWrapper EllipseShape FunkyShape
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()

    qt_generate_deploy_qml_app_script(
        TARGET ${target}
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
        # Just to ensure that running the test from the build dir also works
        MACOS_BUNDLE_POST_BUILD
    )
    install(SCRIPT ${deploy_script})

    if(CMAKE_VERSION VERSION_LESS "3.19")
        qt_finalize_target(${target})
    endif()

    if(APPLE AND NOT IOS)
        set(installed_app_location "${CMAKE_INSTALL_PREFIX}/${target}.app/Contents/MacOS/${target}")
    elseif(WIN32)
        set(installed_app_location "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${target}.exe")
    elseif(UNIX AND NOT APPLE AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING)
        set(installed_app_location "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${target}")
    endif()

    # There's no nice way to get the location of an installed binary, so we need to construct
    # the binary install path by hand, somewhat similar to how it's done in
    # the implementation of qt_deploy_runtime_dependencies.
    # On unsupported deployment platforms, either the install_ test will fail not finding
    # the location of the app (because we do not set a installed_app_location value)
    # or the run_deployed_ test will fail because we didn't deploy the runtime dependencies.
    #
    # On iOS we don't want to install nor run the application. Installing won't work due to the
    # ctest limitation described above, and running won't work because we'd have to do it via
    # a simulator. More importantly installing would fail with
    #  Test command: NOT_AVAILABLE
    # because using the add_test(NAME) signature fails to run when using ctest without a -C
    # option.
    # We still want to build the iOS app, to confirm that building works.
    #
    # When support for additional platforms is added, these locations will have to be augmented.
    if(NOT IOS)
        add_test(install_${target} "${CMAKE_COMMAND}" --install .)
        set_tests_properties(install_${target} PROPERTIES FIXTURES_SETUP deploy_step)
        add_test(NAME run_deployed_${target}
                 COMMAND "${installed_app_location}"
                 # Make sure that we don't use the default working directory which is
                 # CMAKE_CURRENT_BINARY_DIR because on Windows the loader might pick up dlls
                 # from the working directory instead of the installed app dir, if the dll is
                 # missing in the app dir.
                 WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
    endif()
endfunction()

# Create the backing targets before the app, so that the backing library
# install commands don't error out.
add_subdirectory(Shapes)

create_test_executable(UserQmlApp)
