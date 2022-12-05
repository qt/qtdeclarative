TEMPLATE = app
TARGET = todolist
QT += quick quickcontrols2

SOURCES += src/main.cpp

RESOURCES += \
        images/back.png \
        images/back@2x.png \
        images/back@3x.png \
        images/back-white.png \
        images/back-white@2x.png \
        images/back-white@3x.png \
        images/close.png \
        images/close@2x.png \
        images/close@3x.png \
        images/close-white.png \
        images/close-white@2x.png \
        images/close-white@3x.png \
        images/plus-math.png \
        images/plus-math@2x.png \
        images/plus-math@3x.png \
        images/settings.png \
        images/settings@2x.png \
        images/settings@3x.png \
        images/add-new.png \
        images/add-new@2x.png \
        images/add-new@3x.png \
        main.qml \
        AppSettings.qml \
        Database.qml \
        FontSizePage.qml \
        HomePage.qml \
        MaxTasksPage.qml \
        NavBar.qml \
        ProjectPage.qml \
        SettingsPage.qml \
        ToggleCompletedTasksPage.qml \
        qmldir

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols/ios/todolist
INSTALLS += target
