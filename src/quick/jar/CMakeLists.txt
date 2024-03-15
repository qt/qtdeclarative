qt_internal_add_jar(Qt${QtDeclarative_VERSION_MAJOR}AndroidQuick
    INCLUDE_JARS
        ${QT_ANDROID_JAR}
        ${QT6_INSTALL_PREFIX}/jar/Qt${QtDeclarative_VERSION_MAJOR}Android.jar
    SOURCES org/qtproject/qt/android/QtQuickView.java
    OUTPUT_DIR "${QT_BUILD_DIR}/jar")

qt_path_join(destination ${INSTALL_DATADIR} "jar")

install_jar(Qt${QtDeclarative_VERSION_MAJOR}AndroidQuick
        DESTINATION ${destination}
        COMPONENT Devel
)
