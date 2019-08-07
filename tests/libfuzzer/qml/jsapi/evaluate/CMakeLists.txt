# Generated from evaluate.pro.

#####################################################################
## evaluate Binary:
#####################################################################

add_qt_executable(evaluate
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        # Remove: fsanitize=fuzzer
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:evaluate.pro:<TRUE>:
# CONFIG = "console" "-app_bundle"
