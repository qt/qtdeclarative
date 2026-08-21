find_package(Qt6 REQUIRED COMPONENTS Qml)

# Ask for the new argument forwarding, the values have to arrive unchanged.
qt_policy(SET QTP0008 NEW)

# Opt in to the quoted argument values.
qt_policy(SET QTP0007 NEW)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(app MACOSX_BUNDLE main.cpp)
qt_add_qml_module(app
    URI DeployQuoting
    VERSION 1.0
    QML_FILES Main.qml
)
target_link_libraries(app PRIVATE Qt6::Qml)

# Test behavior of the version-less command. It used to be a macro that forwards ${ARGV},
# which cmake re-parses and thus consumed one level of backslashes. Now that it is a
# function, the values reach the generated script unchanged, same as with the
# 'qt6_'-prefixed command.
qt_generate_deploy_qml_app_script(
    TARGET app
    OUTPUT_SCRIPT deploy_script
    DEPLOY_TOOL_OPTIONS
        "-codesign=Developer ID Application: ACME & Co. (ABC123)"
        "-quote=say \"hi\""
        "-backslash=a\\b"
        "-double-backslash=a\\\\b"
        "-backslash-quote=a\\\"b"
        "-genex=$<1:expanded>"
        "-trailing-backslash=a\\"
    POST_EXCLUDE_FILES
        "/some path/with spaces/lib.so"
)
