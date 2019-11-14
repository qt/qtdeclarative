# Generated from evaluate.pro.

#####################################################################
## evaluate Binary:
#####################################################################

qt_add_executable(evaluate
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        # Remove: fsanitize=fuzzer
        Qt::Qml
)
