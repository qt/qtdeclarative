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
