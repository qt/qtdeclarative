qt_internal_add_jar(Qt${QtDeclarative_VERSION_MAJOR}AndroidQuick
    INCLUDE_JARS
        ${QT_ANDROID_JAR}
        ${QT6_INSTALL_PREFIX}/jar/Qt${QtDeclarative_VERSION_MAJOR}Android.jar
    SOURCES
        src/org/qtproject/qt/android/QtQuickView.java
        src/org/qtproject/qt/android/QtQmlStatus.java
        src/org/qtproject/qt/android/QtQmlStatusChangeListener.java
        src/org/qtproject/qt/android/QtQuickViewContent.java
    OUTPUT_DIR "${QT_BUILD_DIR}/jar")

qt_path_join(destination ${INSTALL_DATADIR} "jar")

install_jar(Qt${QtDeclarative_VERSION_MAJOR}AndroidQuick
        DESTINATION ${destination}
        COMPONENT Devel
)
