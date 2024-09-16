# What is this?

This project is for manual testing of embedding QML into Android Services. It
loads a QML view and a regular Android view side by side, both hosted by a
Service, and wires them together.

This application is meant to be built using Android Studio, with the Qt Gradle
plugin. There is no need to manually build the Qt project or edit it, only this
Android project.

# How to sign the application
In order to sign the application, you must have a keystore file and list it in
a 'keystore.properties' file in the project root.

1) Create 'keystore.properties' file in the same folder as this README
2) Add the following information to the file:
    ```
    storePassword=somePassword
    keyPassword=someOtherPassword
    keyAlias=someKeyAlias
    storeFile=/full/path/to/your/keystore.keystore
    ```

After this, the app build.gradle will read that file and extract the required
information from it, and use that to sign the app before it is deployed.

# How to configure QtBuild Gradle plugin
The app-level build.gradle already includes and configures the plugin, but it requires some information about the environment it's running in: The Qt installation directory, and the Qt for Android kit directory.

1) Create 'qtbuild.properties' file in the same folder as this README
2) Add the following information to the file:
    ```
    qtKitDir=/path/to/your/android/kit/
    qtPath=/path/to/your/Qt/installation // e.g. /etc/Qt/6.8.0
    ```
