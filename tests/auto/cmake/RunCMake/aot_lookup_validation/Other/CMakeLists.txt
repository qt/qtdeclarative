qt_add_qml_module(other
    URI Other
    SOURCES
        other.h
)

target_link_libraries(other PRIVATE Qt6::Qml)
