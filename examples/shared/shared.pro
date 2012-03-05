#just install the files, all QML for now
TEMPLATE = aux

qml.files = images \
            LauncherList.qml \
            SimpleLauncherDelegate.qml \
            Button.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/shared
INSTALLS = qml
