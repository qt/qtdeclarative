#! [0]
find_package(Qt6 REQUIRED COMPONENTS Qml)
target_link_libraries(mytarget PRIVATE Qt6::Qml)
#! [0]

#! [1]
find_package(Qt6 REQUIRED COMPONENTS QmlIntegration)
target_link_libraries(mytarget PRIVATE Qt6::QmlIntegration)
#! [1]

