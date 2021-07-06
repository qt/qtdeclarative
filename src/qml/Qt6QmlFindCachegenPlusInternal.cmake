if (QT_NO_FIND_QML_CACHEGEN_PLUS)
    return()
endif()

set(QT_NO_FIND_QML_CACHEGEN_PLUS TRUE)

# FIXME: Make this work with cross-builds
find_package(Qt6QmlCompilerPlus QUIET)
