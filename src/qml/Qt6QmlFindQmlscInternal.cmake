if (QT_NO_FIND_QMLSC)
    return()
endif()

set(QT_NO_FIND_QMLSC TRUE)

# FIXME: Make this work with cross-builds
find_package(Qt6QmlCompilerPlusPrivate QUIET)
