// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qsettings.h>
#include <QtGui/qguiapplication.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQuickControls2/qquickstyle.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName("dialogs-manual-test");
    QCoreApplication::setOrganizationName("QtProject");

    // Use native dialogs by default unless the user asked us not to.
    QSettings settings;
    const QVariant useNativeDialogs = settings.value("useNativeDialogs");
    if (useNativeDialogs.isValid()) {
        if (!useNativeDialogs.toBool())
            QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    } else {
        // Set the default here so that we can use an alias in QML.
        // Without this it defaults to CheckBox's checked default value, which is false.
        settings.setValue("useNativeDialogs", true);
    }

    QQmlApplicationEngine engine;
    engine.setInitialProperties({{ "style", QQuickStyle::name() }});
    engine.load(QUrl(QStringLiteral("qrc:/dialogs.qml")));

    return app.exec();
}
