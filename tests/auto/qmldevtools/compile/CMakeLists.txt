# Generated from compile.pro.

#####################################################################
## tst_compile Binary:
#####################################################################

qt_internal_add_executable(tst_compile
    GUI
    SOURCES
        tst_compile.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::QmlDevToolsPrivate
)

#### Keys ignored in scope 1:.:.:compile.pro:<TRUE>:
# _OPTION = "host_build"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_compile CONDITION force_bootstrap
    PUBLIC_LIBRARIES
        Qt::BootstrapPrivate
)

qt_internal_extend_target(tst_compile CONDITION NOT force_bootstrap
    PUBLIC_LIBRARIES
        Qt::Core
)
