// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyle.h"
#include "qquickstyle_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>
#if QT_CONFIG(settings)
#include <QtCore/qsettings.h>
#endif
#include <QtCore/qfileselector.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmetaobject.h>
#include <QtGui/qcolor.h>
#include <QtGui/qfont.h>
#include <QtGui/qpalette.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfile.h>

#include <functional>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQtQuickControlsStyle, "qt.quick.controls.style")

/*!
    \class QQuickStyle
    \brief The QQuickStyle class allows configuring the application style.
    \inmodule QtQuickControls2
    \since 5.7

    QQuickStyle provides API for querying and configuring the application
    \l {Styling Qt Quick Controls}{styles} of Qt Quick Controls.

    \code
    #include <QGuiApplication>
    #include <QQmlApplicationEngine>
    #include <QQuickStyle>

    int main(int argc, char *argv[])
    {
        QGuiApplication app(argc, argv);

        QQuickStyle::setStyle("Material");

        QQmlApplicationEngine engine;
        engine.load(QUrl("qrc:/main.qml"));

        return app.exec();
    }
    \endcode

    \note The style must be configured \b before loading QML that imports
    Qt Quick Controls. It is not possible to change the style after the QML
    types have been registered.

    \note QQuickStyle is not supported when using
    \l {Compile-Time Style Selection}{compile-time style selection}.

    To create your own custom style, see \l {Creating a Custom Style}. Custom
    styles do not need to implement all controls. By default, the styling
    system uses the \l {Basic style} as a fallback for controls that a custom
    style does not provide. It is possible to specify a different fallback
    style to customize or extend one of the built-in styles.

    \code
    QQuickStyle::setStyle("MyStyle");
    QQuickStyle::setFallbackStyle("Material");
    \endcode

    \sa {Styling Qt Quick Controls}
*/

struct QQuickStyleSpec
{
    QQuickStyleSpec() { }

    QString name()
    {
        if (!resolved)
            resolve();
        return style.mid(style.lastIndexOf(QLatin1Char('/')) + 1);
    }

    QString path()
    {
        if (!resolved)
            resolve();
        QString s = style;
        if (QQmlFile::isLocalFile(s))
            s = QQmlFile::urlToLocalFileOrQrc(s);
        return s.left(s.lastIndexOf(QLatin1Char('/')) + 1);
    }

    void setStyle(const QString &s)
    {
        qCDebug(lcQtQuickControlsStyle) << "style" << s << "set on QQuickStyleSpec";
        if (s.contains(QLatin1Char('/'))) {
            qWarning() << "Style names must not contain paths; see the \"Definition of a Style\" documentation for more information";
            return;
        }

        qCDebug(lcQtQuickControlsStyle) << "clearing resolved flag and resolving";
        style = s;
        resolved = false;
        resolve();
    }

    void setFallbackStyle(const QString &fallback, const QByteArray &method)
    {
        if (!fallback.isEmpty())
            qCDebug(lcQtQuickControlsStyle) << "fallback style" << fallback << "set on QQuickStyleSpec via" << method;

        fallbackStyle = fallback;
        fallbackMethod = method;
    }

    void resolve()
    {
        qCDebug(lcQtQuickControlsStyle) << "resolving style";

        if (style.isEmpty())
            style = QGuiApplicationPrivate::styleOverride;
        if (style.isEmpty())
            style = QString::fromLocal8Bit(qgetenv("QT_QUICK_CONTROLS_STYLE"));
        if (fallbackStyle.isEmpty())
            setFallbackStyle(QString::fromLocal8Bit(qgetenv("QT_QUICK_CONTROLS_FALLBACK_STYLE")), "QT_QUICK_CONTROLS_FALLBACK_STYLE");
#if QT_CONFIG(settings)
        if (style.isEmpty() || fallbackStyle.isEmpty()) {
            QSharedPointer<QSettings> settings = QQuickStylePrivate::settings(QStringLiteral("Controls"));
            if (settings) {
                if (style.isEmpty())
                    style = settings->value(QStringLiteral("Style")).toString();
                if (fallbackStyle.isEmpty())
                    setFallbackStyle(settings->value(QStringLiteral("FallbackStyle")).toString(), ":/qtquickcontrols2.conf");
            }
        }
#endif

        auto builtInStyleList = QQuickStylePrivate::builtInStyles();
        if (!fallbackStyle.isEmpty() && !builtInStyleList.contains(fallbackStyle)) {
            qWarning().nospace().noquote() << fallbackMethod << ": the specified fallback style \"" <<
                fallbackStyle << "\" is not one of the built-in Qt Quick Controls 2 styles";
            fallbackStyle.clear();
        }

        // Find the config file.
        resolveConfigFilePath();

        usingDefaultStyle = false;

        if (style.isEmpty() || style.toLower() == QStringLiteral("default")) {
            usingDefaultStyle = true;
            style.clear();

            qCDebug(lcQtQuickControlsStyle) << "no style (or Default) was specified;"
                << "checking if we have an appropriate style for this platform";

            // If these defaults are changed, ensure that the "Using Styles in Qt Quick Controls"
            // section of qtquickcontrols-styles.qdoc is updated.
#if defined(Q_OS_MACOS)
            style = QLatin1String("macOS");
#elif defined(Q_OS_WINDOWS)
            style = QLatin1String("Windows");
#elif defined(Q_OS_ANDROID)
            style = QLatin1String("Material");
#elif defined(Q_OS_LINUX)
            style = QLatin1String("Fusion");
#elif defined(Q_OS_IOS)
            style = QLatin1String("iOS");
#endif
            if (!style.isEmpty())
                qCDebug(lcQtQuickControlsStyle) << "using" << style << "as a default";
            else
                qCDebug(lcQtQuickControlsStyle) << "no appropriate style found; using Basic as a default";
        }

        // If it's still empty by this point, then it means we have no native style available for this platform,
        // as is the case on e.g. embedded. In that case, we want to default to the Basic style,
        // which is what effectiveStyleName() returns when "style" is empty.
        custom = !builtInStyleList.contains(QQuickStylePrivate::effectiveStyleName(style));

        resolved = true;

        qCDebug(lcQtQuickControlsStyle).nospace() << "done resolving:"
            << "\n    style=" << style
            << "\n    custom=" << custom
            << "\n    resolved=" << resolved
            << "\n    fallbackStyle=" << fallbackStyle
            << "\n    fallbackMethod=" << fallbackMethod
            << "\n    configFilePath=" << configFilePath;
    }

    void reset()
    {
        qCDebug(lcQtQuickControlsStyle) << "resetting values to their defaults";

        custom = false;
        resolved = false;
        usingDefaultStyle = false;
        style.clear();
        fallbackStyle.clear();
        fallbackMethod.clear();
        configFilePath.clear();
    }

    QString resolveConfigFilePath()
    {
        if (configFilePath.isEmpty()) {
            configFilePath = QFile::decodeName(qgetenv("QT_QUICK_CONTROLS_CONF"));
            if (configFilePath.isEmpty() || !QFile::exists(configFilePath)) {
                if (!configFilePath.isEmpty())
                    qWarning("QT_QUICK_CONTROLS_CONF=%s: No such file", qPrintable(configFilePath));

                configFilePath = QStringLiteral(":/qtquickcontrols2.conf");
            }
        }
        return configFilePath;
    }

    // Is this a custom style defined by the user and not "built-in" style?
    bool custom = false;
    // Have we resolved the style yet?
    bool resolved = false;
    // Are we using the default style for this platform (because no style was specified)?
    bool usingDefaultStyle = false;
    // The name of the style.
    QString style;
    // The built-in style to use if the requested style cannot be found.
    QString fallbackStyle;
    // A description of the way in which fallbackStyle was set, used in e.g. warning messages shown to the user.
    QByteArray fallbackMethod;
    // The path to the qtquickcontrols2.conf file.
    QString configFilePath;
};

Q_GLOBAL_STATIC(QQuickStyleSpec, styleSpec)

/*
    Note that most of these functions (with the exception of e.g. isResolved())
    should not be called before the style has been resolved, as it's only after
    that happens that they will have been set.
*/
QString QQuickStylePrivate::style()
{
    return styleSpec()->style;
}

QString QQuickStylePrivate::effectiveStyleName(const QString &styleName)
{
    return !styleName.isEmpty() ? styleName : QLatin1String("Basic");
}

QString QQuickStylePrivate::fallbackStyle()
{
    return styleSpec()->fallbackStyle;
}

bool QQuickStylePrivate::isCustomStyle()
{
    return styleSpec()->custom;
}

bool QQuickStylePrivate::isResolved()
{
    return styleSpec()->resolved;
}

bool QQuickStylePrivate::isUsingDefaultStyle()
{
    return styleSpec()->usingDefaultStyle;
}

void QQuickStylePrivate::init()
{
    QQuickStyleSpec *spec = styleSpec();
    spec->resolve();
}

void QQuickStylePrivate::reset()
{
    if (styleSpec())
        styleSpec()->reset();
}

QString QQuickStylePrivate::configFilePath()
{
    return styleSpec()->resolveConfigFilePath();
}

QSharedPointer<QSettings> QQuickStylePrivate::settings(const QString &group)
{
#ifndef QT_NO_SETTINGS
    const QString filePath = QQuickStylePrivate::configFilePath();
    if (QFile::exists(filePath)) {
        QFileSelector selector;
        QSettings *settings = new QSettings(selector.select(filePath), QSettings::IniFormat);
        if (!group.isEmpty())
            settings->beginGroup(group);
        return QSharedPointer<QSettings>(settings);
    }
#endif // QT_NO_SETTINGS
    return QSharedPointer<QSettings>();
}

#if QT_CONFIG(settings)
static void readValue(const QSharedPointer<QSettings> &settings, const QString &name, std::function<void(const QVariant &)> setValue)
{
    const QVariant var = settings->value(name);
    if (var.isValid())
        setValue(var);
}

template <typename Enum>
static Enum toEnumValue(const QVariant &var)
{
    // ### TODO: expose QFont enums to the meta object system using Q_ENUM
    //QMetaEnum enumeration = QMetaEnum::fromType<Enum>();
    //bool ok = false;
    //int value = enumeration.keyToValue(var.toByteArray(), &ok);
    //if (!ok)
    //    value = var.toInt();
    //return static_cast<Enum>(value);

    return static_cast<Enum>(var.toInt());
}

const QFont *QQuickStylePrivate::readFont(const QSharedPointer<QSettings> &settings)
{
    const QVariant var = settings->value(QStringLiteral("Font"));
    if (var.isValid())
        return new QFont(var.value<QFont>());

    QFont f;
    settings->beginGroup(QStringLiteral("Font"));
    readValue(settings, QStringLiteral("Family"), [&f](const QVariant &var) { f.setFamilies(QStringList{var.toString()}); });
    readValue(settings, QStringLiteral("PointSize"), [&f](const QVariant &var) { f.setPointSizeF(var.toReal()); });
    readValue(settings, QStringLiteral("PixelSize"), [&f](const QVariant &var) { f.setPixelSize(var.toInt()); });
    readValue(settings, QStringLiteral("StyleHint"), [&f](const QVariant &var) { f.setStyleHint(toEnumValue<QFont::StyleHint>(var.toInt())); });
    readValue(settings, QStringLiteral("Weight"), [&f](const QVariant &var) { f.setWeight(toEnumValue<QFont::Weight>(var)); });
    readValue(settings, QStringLiteral("Style"), [&f](const QVariant &var) { f.setStyle(toEnumValue<QFont::Style>(var.toInt())); });
    settings->endGroup();
    return new QFont(f);
}

static void readColorGroup(const QSharedPointer<QSettings> &settings, QPalette::ColorGroup group, QPalette *palette)
{
    const QStringList keys = settings->childKeys();
    if (keys.isEmpty())
        return;

    static const int index = QPalette::staticMetaObject.indexOfEnumerator("ColorRole");
    Q_ASSERT(index != -1);
    QMetaEnum metaEnum = QPalette::staticMetaObject.enumerator(index);

    for (const QString &key : keys) {
        bool ok = false;
        int role = metaEnum.keyToValue(key.toUtf8(), &ok);
        if (ok)
            palette->setColor(group, static_cast<QPalette::ColorRole>(role), settings->value(key).value<QColor>());
    }
}

const QPalette *QQuickStylePrivate::readPalette(const QSharedPointer<QSettings> &settings)
{
    QPalette p;
    settings->beginGroup(QStringLiteral("Palette"));
    readColorGroup(settings, QPalette::All, &p);

    settings->beginGroup(QStringLiteral("Normal"));
    readColorGroup(settings, QPalette::Normal, &p);
    settings->endGroup();

    settings->beginGroup(QStringLiteral("Disabled"));
    readColorGroup(settings, QPalette::Disabled, &p);
    settings->endGroup();
    return new QPalette(p);
}
#endif // QT_CONFIG(settings)

bool QQuickStylePrivate::isDarkSystemTheme()
{
    const bool dark = [](){
        if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
            return theme->colorScheme() == Qt::ColorScheme::Dark;
        return false;
    }();
    return dark;
}

QStringList QQuickStylePrivate::builtInStyles()
{
    return {
        QLatin1String("Basic"),
        QLatin1String("Fusion"),
        QLatin1String("Imagine"),
#ifdef Q_OS_MACOS
        QLatin1String("macOS"),
        QLatin1String("iOS"),
#endif
#ifdef Q_OS_IOS
        QLatin1String("iOS"),
#endif
        QLatin1String("Material"),
        QLatin1String("Universal"),
#ifdef Q_OS_WINDOWS
        QLatin1String("Windows")
#endif
    };
}

/*!
    Returns the name of the application style.

    \note The application style can be specified by passing a \c -style command
          line argument. Therefore \c name() may not return a fully resolved
          value if called before constructing a QGuiApplication.
*/
QString QQuickStyle::name()
{
    return styleSpec()->name();
}

/*!
    Sets the application style to \a style.

    \note The style must be configured \b before loading QML that imports Qt Quick Controls.
          It is not possible to change the style after the QML types have been registered.

    \sa setFallbackStyle(), {Using Styles in Qt Quick Controls}
*/
void QQuickStyle::setStyle(const QString &style)
{
    qCDebug(lcQtQuickControlsStyle) << "setStyle called with" << style;

    if (QQmlMetaType::matchingModuleVersion(
                QStringLiteral("QtQuick.Controls"), QTypeRevision::fromVersion(2, 0)).isValid()) {
        qWarning() << "ERROR: QQuickStyle::setStyle() must be called before loading QML that imports Qt Quick Controls 2.";
        return;
    }

    styleSpec()->setStyle(style);
}

/*!
    \since 5.8
    Sets the application fallback style to \a style.

    \note The fallback style must be the name of one of the built-in Qt Quick Controls styles, e.g. "Material".

    \note The style must be configured \b before loading QML that imports Qt Quick Controls.
          It is not possible to change the style after the QML types have been registered.

    The fallback style can be also specified by setting the \c QT_QUICK_CONTROLS_FALLBACK_STYLE
    \l {Supported Environment Variables in Qt Quick Controls}{environment variable}.

    \sa setStyle(), {Using Styles in Qt Quick Controls}
*/
void QQuickStyle::setFallbackStyle(const QString &style)
{
    if (QQmlMetaType::matchingModuleVersion(
                QStringLiteral("QtQuick.Controls"), QTypeRevision::fromVersion(2, 0)).isValid()) {
        qWarning() << "ERROR: QQuickStyle::setFallbackStyle() must be called before loading QML that imports Qt Quick Controls 2.";
        return;
    }

    styleSpec()->setFallbackStyle(style, "QQuickStyle::setFallbackStyle()");
}

QT_END_NAMESPACE
