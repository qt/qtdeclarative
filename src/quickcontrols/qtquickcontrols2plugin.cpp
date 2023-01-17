// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qfileselector_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTemplates2/private/qquicktheme_p_p.h>
#include <QtQuickControls2/private/qquickstyle_p.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/qtquickcontrols2global.h>

QT_BEGIN_NAMESPACE

Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls);

Q_LOGGING_CATEGORY(lcQtQuickControls2Plugin, "qt.quick.controls.qtquickcontrols2plugin")

class QtQuickControls2Plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2Plugin(QObject *parent = nullptr);
    ~QtQuickControls2Plugin();

    void registerTypes(const char *uri) override;
    void unregisterTypes() override;

private:
    // We store these because the style plugins can be unregistered before
    // QtQuickControls2Plugin, and since QQuickStylePlugin calls QQuickStylePrivate::reset(),
    // the style information can be lost when it comes time to call qmlUnregisterModuleImport().
    // It also avoids unnecessarily resolving the style after resetting it just to get the style
    // name in unregisterTypes().
    bool customStyle = false;
    QString registeredStyleUri;
    QString registeredFallbackStyleUri;
    QString rawFallbackStyleName;
};

static const char *qtQuickControlsUri = "QtQuick.Controls";

QString styleUri()
{
    const QString style = QQuickStyle::name();
    if (!QQuickStylePrivate::isCustomStyle()) {
        // The style set is a built-in style.
        const QString styleName = QQuickStylePrivate::effectiveStyleName(style);
        return QString::fromLatin1("QtQuick.Controls.%1").arg(styleName);
    }

    // This is a custom style, so just use the name as the import uri.
    QString styleName = style;
    if (styleName.startsWith(QLatin1String(":/")))
        styleName.remove(0, 2);
    return styleName;
}

QString fallbackStyleUri()
{
    // The fallback style must be a built-in style, so we don't need to check for custom styles here.
    const QString fallbackStyle = QQuickStylePrivate::fallbackStyle();
    const QString fallbackStyleName = QQuickStylePrivate::effectiveStyleName(fallbackStyle);
    return QString::fromLatin1("QtQuick.Controls.%1").arg(fallbackStyleName);
}

QtQuickControls2Plugin::QtQuickControls2Plugin(QObject *parent) : QQmlExtensionPlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_Controls;
    Q_UNUSED(registration);
}

QtQuickControls2Plugin::~QtQuickControls2Plugin()
{
    // Intentionally empty: we use register/unregisterTypes() to do
    // initialization and cleanup, as plugins are not unloaded on macOS.
}

/*!
    \internal

    If this function is called, it means QtQuick.Controls was imported,
    and we're doing runtime style selection.

    For example, where:
    \list
    \li styleName="Material"
    \li rawFallbackStyleName=""
    \li fallbackStyleName="Basic"
    \li registeredStyleUri="QtQuick.Controls.Material"
    \li rawFallbackStyleName is empty => parentModule="QtQuick.Controls.Material"
    \li registeredFallbackStyleUri="QtQuick.Controls.Basic"
    \endlist

    The following registrations would be made:

    qmlRegisterModuleImport("QtQuick.Controls.Material", "QtQuick.Controls.Basic")
    qmlRegisterModuleImport("QtQuick.Controls", "QtQuick.Controls.Material")

    As another example, where:
    \list
    \li styleName="Material"
    \li rawFallbackStyleName="Fusion"
    \li fallbackStyleName="Fusion"
    \li registeredStyleUri="QtQuick.Controls.Material"
    \li rawFallbackStyleName is not empty => parentModule="QtQuick.Controls"
    \li registeredFallbackStyleUri="QtQuick.Controls.Fusion"
    \endlist

    The following registrations would be made:

    qmlRegisterModuleImport("QtQuick.Controls", "QtQuick.Controls.Fusion")
    qmlRegisterModuleImport("QtQuick.Controls", "QtQuick.Controls.Material")

    In this case, the Material style imports a fallback (Basic) via the IMPORTS
    section in its CMakeLists.txt, \e and the user specifies a different fallback
    using an env var/.conf/C++. We want the user's fallback to take priority,
    which means we have to place the user-specified fallback at a more immediate place,
    and that place is as an import of QtQuick.Controls itself rather than as an
    import of the current style, Material (as we did in the first example).

    If the style to be imported is a custom style and no specific fallback was
    selected, we need to indirectly import Basic, but we cannot import Basic through
    the custom style since the versions don't match. For that case we have a
    "QtQuick.Controls.IndirectBasic" which does nothing but import
    QtQuick.Controls.Basic. Instead of QtQuick.Controls.Basic we import that one:

    qmlRegisterModuleImport("QtQuick.Controls", "Some.Custom.Style")
    qmlRegisterModuleImport("QtQuick.Controls", "QtQuick.Controls.IndirectBasic")
*/
void QtQuickControls2Plugin::registerTypes(const char *uri)
{
    qCDebug(lcQtQuickControls2Plugin) << "registerTypes() called with uri" << uri;

    // It's OK that the style is resolved more than once; some accessors like name() cause it to be called, for example.
    QQuickStylePrivate::init();

    // The fallback style that was set via env var/.conf/C++.
    rawFallbackStyleName = QQuickStylePrivate::fallbackStyle();
    // The style that was set via env var/.conf/C++, or Basic if none was set.
    const QString styleName = QQuickStylePrivate::effectiveStyleName(QQuickStyle::name());
    // The effective fallback style: rawFallbackStyleName, or Basic if empty.
    const QString fallbackStyleName = QQuickStylePrivate::effectiveStyleName(rawFallbackStyleName);
    qCDebug(lcQtQuickControls2Plugin) << "style:" << QQuickStyle::name() << "effective style:" << styleName
        << "fallback style:" << rawFallbackStyleName << "effective fallback style:" << fallbackStyleName;

    customStyle = QQuickStylePrivate::isCustomStyle();
    // The URI of the current style. For built-in styles, the style name is appended to "QtQuick.Controls.".
    // For custom styles that are embedded in resources, we need to remove the ":/" prefix.
    registeredStyleUri = ::styleUri();

    // If the style is Basic, we don't need to register the fallback because the Basic style
    // provides all controls. Also, if we didn't return early here, we can get an infinite import loop
    // when the style is set to Basic.
    if (styleName != fallbackStyleName && styleName != QLatin1String("Basic")) {
        // If no specific fallback is given, the fallback is of lower precedence than recursive
        // imports of the main style (i.e. IMPORTS in a style's CMakeLists.txt).
        // If a specific fallback is given, it is of higher precedence.

        QString parentModule;
        QString fallbackModule;

        // The fallback style has to be a built-in style, so it will become "QtQuick.Controls.<fallback>".
        registeredFallbackStyleUri = ::fallbackStyleUri();

        if (!rawFallbackStyleName.isEmpty()) {
            parentModule = qtQuickControlsUri;
            fallbackModule = registeredFallbackStyleUri;
        } else if (customStyle) {
            // Since we don't know the versioning scheme of custom styles, but we want the
            // version of QtQuick.Controls to be propagated, we need to do our own indirection.
            // QtQuick.Controls.IndirectBasic indirectly imports QtQuick.Controls.Basic
            Q_ASSERT(registeredFallbackStyleUri == QLatin1String("QtQuick.Controls.Basic"));
            parentModule = qtQuickControlsUri;
            fallbackModule = QLatin1String("QtQuick.Controls.IndirectBasic");
        } else {
            parentModule = registeredStyleUri;
            fallbackModule = registeredFallbackStyleUri;
        }

        qCDebug(lcQtQuickControls2Plugin)
                << "calling qmlRegisterModuleImport() to register fallback style with"
                << " uri \"" << parentModule << "\" moduleMajor" << QQmlModuleImportModuleAny
                << "import" << fallbackModule << "importMajor" << QQmlModuleImportAuto;
        // Whenever parentModule is imported, registeredFallbackStyleUri will be imported too.
        // The fallback style must be a built-in style, so we match the version number.
        qmlRegisterModuleImport(parentModule.toUtf8().constData(), QQmlModuleImportModuleAny,
                                fallbackModule.toUtf8().constData(),
                                QQmlModuleImportAuto, QQmlModuleImportAuto);
    }

    // If the user imports QtQuick.Controls 2.15, and they're using the Material style, we should import version 2.15.
    // However, if they import QtQuick.Controls 2.15, but are using a custom style, we want to use the latest version
    // number of their style.
    const int importMajor = customStyle ? QQmlModuleImportLatest : QQmlModuleImportAuto;
    qCDebug(lcQtQuickControls2Plugin).nospace()
            << "calling qmlRegisterModuleImport() to register primary style with"
            << " uri \"" << qtQuickControlsUri << "\" moduleMajor " << importMajor
            << " import " << registeredStyleUri << " importMajor " << importMajor;
    // When QtQuick.Controls is imported, the selected style will be imported too.
    qmlRegisterModuleImport(qtQuickControlsUri, QQmlModuleImportModuleAny,
                            registeredStyleUri.toUtf8().constData(), importMajor);

    if (customStyle)
        QFileSelectorPrivate::addStatics(QStringList() << styleName);
}

void QtQuickControls2Plugin::unregisterTypes()
{
    qCDebug(lcQtQuickControls2Plugin) << "unregisterTypes() called";

    const int importMajor = customStyle ? QQmlModuleImportLatest : QQmlModuleImportAuto;
    qCDebug(lcQtQuickControls2Plugin).nospace()
            << "calling qmlUnregisterModuleImport() to unregister primary style with"
            << " uri \"" << qtQuickControlsUri << "\" moduleMajor " << importMajor
            << " import " << registeredStyleUri << " importMajor " << importMajor;
    qmlUnregisterModuleImport(qtQuickControlsUri, QQmlModuleImportModuleAny,
                              registeredStyleUri.toUtf8().constData(), importMajor);

    if (!registeredFallbackStyleUri.isEmpty()) {
        QString parentModule;
        QString fallbackModule;

        if (!rawFallbackStyleName.isEmpty()) {
            parentModule = qtQuickControlsUri;
            fallbackModule = registeredFallbackStyleUri;
            rawFallbackStyleName.clear();
        } else if (customStyle) {
            parentModule = qtQuickControlsUri;
            fallbackModule = QLatin1String("QtQuick.Controls.IndirectBasic");
        } else {
            parentModule = registeredStyleUri;
            fallbackModule = registeredFallbackStyleUri;
        }

        qCDebug(lcQtQuickControls2Plugin)
                << "calling qmlUnregisterModuleImport() to unregister fallback style with"
                << " uri \"" << parentModule << "\" moduleMajor" << QQmlModuleImportModuleAny
                << "import" << fallbackModule << "importMajor" << QQmlModuleImportAuto;
        qmlUnregisterModuleImport(parentModule.toUtf8().constData(), QQmlModuleImportModuleAny,
                                  fallbackModule.toUtf8().constData(),
                                  QQmlModuleImportAuto, QQmlModuleImportAuto);

        registeredFallbackStyleUri.clear();
    }

    customStyle = false;
    registeredStyleUri.clear();
}

QT_END_NAMESPACE

#include "qtquickcontrols2plugin.moc"
