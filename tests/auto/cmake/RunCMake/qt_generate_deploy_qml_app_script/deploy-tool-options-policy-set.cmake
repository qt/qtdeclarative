find_package(Qt6 REQUIRED COMPONENTS Qml)

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

# Use the 'qt6_'-prefixed command, because the version-less one might forward through a macro
# which re-parses ${ARGV}, and discards one level of backslashes.
qt6_generate_deploy_qml_app_script(
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
