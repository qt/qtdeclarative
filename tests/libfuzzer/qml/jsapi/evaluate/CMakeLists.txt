# Generated from evaluate.pro.

#####################################################################
## evaluate Binary:
#####################################################################

qt_internal_add_executable(evaluate
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:evaluate.pro:<TRUE>:
# FUZZ_ENGINE = "$$(LIB_FUZZING_ENGINE)"

## Scopes:
#####################################################################

qt_internal_extend_target(evaluate CONDITION FUZZ_ENGINE_ISEMPTY
    LINK_OPTIONS
        "-fsanitize=fuzzer"
)

qt_internal_extend_target(evaluate CONDITION NOT FUZZ_ENGINE_ISEMPTY
    PUBLIC_LIBRARIES
        $ENV{LIB_FUZZING_ENGINE}
)
