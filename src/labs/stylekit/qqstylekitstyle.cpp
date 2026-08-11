// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekit_p.h"
#include "qqstylekitstyle_p.h"
#include "qqstylekittheme_p.h"
#include "qqstylekitvariation_p.h"
#include "qqstylekitcustomtheme_p.h"
#include "qqstylekitcontrolproperties_p.h"
#include "qqstylekitpropertyresolver_p.h"

#include <QtQuickTemplates2/private/qquickdeferredexecute_p_p.h>
#include <QtQml/private/qqmllist_p.h>

#include <QtGui/QGuiApplication>
#include <QtGui/QStyleHints>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Style
    \inqmlmodule Qt.labs.StyleKit
    \inherits AbstractStyle
    \brief The root type for a style definition.

    \l Style is the root type in StyleKit for defining a complete visual style for
    \l [QtQuickControls] {Qt Quick Controls}. A style lets you customize
    the appearance of \l {AbstractStylableControls}{every control type}
    — \l {ControlStateStyle::background}{backgrounds}, \l {ControlStateStyle::indicator}{indicators},
    \l {ControlStateStyle::handle}{handles}, \l {ControlStateStyle::}{text},
    \l {ControlStateStyle::}{padding}, and more
    — as well as how controls respond to states
    such as \l {ControlStyle::}{hovered},
    \l {ControlStyle::}{pressed}, and
    \l {ControlStyle::}{disabled}, including animated
    \l {ControlStyle::transition}{transitions} between them.

    Styles support \l light and \l dark color schemes through \l {Theme}
    {themes}, and you can add any number of \l {CustomTheme}{custom themes}
    as well. \l {StyleVariation}{Style variations} allow you to define
    alternative styling that can be applied to specific control instances
    or entire control types. You can also define \l {CustomControl}
    {custom controls} to extend the style beyond the built-in control set.

    The following example shows a minimal style that defines some structural
    properties shared by all themes, with separate light and dark themes for colors:

    \snippet PlainStyle.qml 1

    For a more complete example, see the \l{StyleKit Quick Controls Example}.

    \labs

    \sa Theme, CustomTheme, StyleVariation, ControlStyle, DelegateStyle,
        CustomControl, {StyleKit Property Resolution}
*/

/*!
    \qmlproperty list<string> Style::customThemeNames
    \readonly

    The names of all the \l {CustomTheme}{custom themes} defined in the style. This does not
    include the \l{availableThemeNames}{built-in themes.}

    \sa availableThemeNames, themeName, CustomTheme
*/

/*!
    \qmlproperty Component Style::dark

    The dark theme component. It's instantiated and applied when the system is in
    dark mode and \l themeName is \c "System", or when \l themeName is
    explicitly set to \c "Dark".

    \snippet StyleSnippets.qml dark

    \sa light, themeName
*/

/*!
    \qmlproperty Style Style::fallbackStyle

    The fallback style used to resolve properties that are not explicitly
    set in this style. When a property is not found in the style or its
    active theme, StyleKit looks it up in the fallback style.

    By default, the fallback style is set to an internal style that provides
    a basic appearance similar to the \l {Basic Style}{Basic} style.

    You can set this to a custom Style, or to \c null to disable
    fallback resolution entirely. Note that setting it to \c null
    means starting from a completely clean slate, which requires
    you to set many more properties than otherwise needed. A
    reference implementation of a fallback style can be found
    \l {qtlabsstylekit-fallbackstyle.html}{here.}
*/

/*!
    \qmlproperty Component Style::light

    The light theme component. It's instantiated and applied when the system is in
    light mode and \l themeName is \c "System", or when \l themeName is
    explicitly set to \c "Light".

    \snippet StyleSnippets.qml light

    \sa dark, themeName
*/

/*!
    \qmlproperty palette Style::palette
    \readonly

    The palette of the control being styled.

    Use this palette to bind colors in the style to the
    \l {StyleReader::palette}{palette} of the control being styled.
    If the application assigns a different palette to a control, the
    style will adapt, and the control will repaint.

    \snippet StyleSnippets.qml palette

    \sa StyleReader::palette
*/

/*!
    \qmlproperty Theme Style::theme
    \readonly

    The currently active theme instance.
    It's instantiated from either the \l {light}{light theme component}, the
    \l {dark}{dark theme component}, or one of the \l {CustomTheme}{custom themes},
    depending on \l themeName.

    When resolving a style property, StyleKit first looks for it in this
    theme (\l {StyleVariation}{StyleVariations} aside). If the property is not found, it
    falls back to search for it in the \l Style.

    \sa themeName, light, dark
*/

/*!
    \qmlproperty string Style::themeName

    The name of the currently active theme. The default value is \c "System",
    which automatically choose between \l light or \l dark depending on the
    color scheme reported by \l QStyleHints::colorScheme.

    You can set this property to change the current theme of this style.

    \snippet StyleSnippets.qml themeName

    Supported values:
    \list
        \li \c "System" \mdash follows \l QStyleHints::colorScheme (default)
        \li \c "Light" \mdash forces the \l light theme
        \li \c "Dark" \mdash forces the \l dark theme
        \li \l {customThemeNames}{Any custom theme name}
    \endlist

    \note Themes are local to the \l Style where they are defined, and can only
    be set as the current theme for that style. For the current theme to take
    effect, the style it belongs to must also be the \l{StyleKit::style}{current style}
    in the application.

    \sa availableThemeNames, theme
*/

/*!
    \qmlproperty list<string> Style::availableThemeNames
    \readonly

    The names of all available themes, including \c "System", \c "Light",
    \c "Dark", and any \l {customThemeNames}{custom themes.}

    \sa themeName, customThemeNames
*/

static const QString kSystem = "System"_L1;
static const QString kLight = "Light"_L1;
static const QString kDark = "Dark"_L1;

QQStyleKitStyle::QQStyleKitStyle(QObject *parent)
    : QQStyleKitStyleAndThemeBase(parent)
    , m_paletteProxy(new QQuickPalette(this))
    , m_themeName(kSystem)
{
}

QQStyleKitStyle::~QQStyleKitStyle()
{
    if (m_theme)
        m_theme->deleteLater();
}

QQuickPalette *QQStyleKitStyle::palette() const
{
    return m_paletteProxy;
}

QQmlComponent *QQStyleKitStyle::light() const
{
    return m_light;
}

QQStyleKitStyle *QQStyleKitStyle::fallbackStyle() const
{
    if (!m_fallbackStyle) {
        auto *self = const_cast<QQStyleKitStyle *>(this);
        self->executeFallbackStyle();
    }
    return m_fallbackStyle;
}

void QQStyleKitStyle::setFallbackStyle(QQStyleKitStyle *fallbackStyle)
{
    if (m_fallbackStyle == fallbackStyle)
        return;

    m_fallbackStyle = fallbackStyle;
    emit fallbackStyleChanged();

    if (palettes())
        palettes()->setFallbackPalette(m_fallbackStyle ? m_fallbackStyle->palettes() : nullptr);

    if (fonts())
        fonts()->setFallbackFont(m_fallbackStyle ? m_fallbackStyle->fonts() : nullptr);

}

void QQStyleKitStyle::setLight(QQmlComponent *lightTheme)
{
    if (m_light == lightTheme)
        return;

    m_light = lightTheme;

    emit lightChanged();
}

QQmlComponent *QQStyleKitStyle::dark() const
{
    return m_dark;
}

void QQStyleKitStyle::setDark(QQmlComponent *darkTheme)
{
    if (m_dark == darkTheme)
        return;

    m_dark = darkTheme;

    emit darkChanged();
}

QQStyleKitTheme *QQStyleKitStyle::theme() const
{
    return m_theme;
}

QList<QQStyleKitCustomTheme *> QQStyleKitStyle::customThemes() const
{
    QList<QQStyleKitCustomTheme *> list;
    for (auto *obj : children()) {
        if (auto *customTheme = qobject_cast<QQStyleKitCustomTheme *>(obj))
            list.append(customTheme);
    }
    return list;
}

void QQStyleKitStyle::parseThemes()
{
    m_availableThemeNames = QStringList({kSystem});

    if (m_light)
        m_availableThemeNames << kLight;
    if (m_dark)
        m_availableThemeNames << kDark;

    for (auto *customTheme : customThemes()) {
        const QString name = customTheme->name();
        if (name.isEmpty())
            continue;
        m_availableThemeNames << name;
        m_customThemeNames << name;
    }

    emit availableThemeNamesChanged();
    emit customThemeNamesChanged();
}

QString QQStyleKitStyle::themeName() const
{
    return m_themeName;
}

QStringList QQStyleKitStyle::availableThemeNames() const
{
    return m_availableThemeNames;
}

QStringList QQStyleKitStyle::customThemeNames() const
{
    return m_customThemeNames;
}

void QQStyleKitStyle::setThemeName(const QString &themeName)
{
    if (m_themeName == themeName)
        return;

    m_themeName = themeName;
    if (m_completed)
        recreateTheme();

    emit themeNameChanged();
}

void QQStyleKitStyle::recreateTheme()
{
    QString effectiveThemeName;
    QQmlComponent *effectiveThemeComponent = nullptr;

    if (QString::compare(m_themeName, kSystem, Qt::CaseInsensitive) == 0) {
        switch (QGuiApplication::styleHints()->colorScheme()) {
        case Qt::ColorScheme::Dark:
            effectiveThemeName = kDark;
            effectiveThemeComponent = m_dark;
            break;
        case Qt::ColorScheme::Light:
        case Qt::ColorScheme::Unknown:
            effectiveThemeName = kLight;
            effectiveThemeComponent = m_light;
            break;
        // Intentionally skipping a default case here, so that if a new scheme is added
        // in the future, we get a warning about an unhandled enum value, which will prompt
        // us to consider if we need to add explicit support for it in StyleKit as well.
        }
    } else if (QString::compare(m_themeName, kLight, Qt::CaseInsensitive) == 0) {
        effectiveThemeName = kLight;
        effectiveThemeComponent = m_light;
    } else if (QString::compare(m_themeName,kDark, Qt::CaseInsensitive) == 0) {
        effectiveThemeName =kDark;
        effectiveThemeComponent = m_dark;
    } else if (!m_themeName.isEmpty()){
        for (auto *customTheme : customThemes()) {
            if (QString::compare(m_themeName, customTheme->name(), Qt::CaseInsensitive) == 0) {
                effectiveThemeName = customTheme->name();
                effectiveThemeComponent = customTheme->theme();
                break;
            }
        }
        if (effectiveThemeName.isEmpty())
            qmlWarning(this) << "No theme found with themeName '" << m_themeName << "'";
        else if (!effectiveThemeComponent)
            qmlWarning(this) << "Custom theme '" << effectiveThemeName << "' has no theme component set";
    }

    if (m_theme) {
        if (m_effectiveThemeName == effectiveThemeName)
            return;

        m_theme->deleteLater();
        m_theme = nullptr;
    }

    m_effectiveThemeName = effectiveThemeName;
    m_currentThemeComponent = effectiveThemeComponent;

    if (effectiveThemeComponent) {
        if (effectiveThemeComponent->status() != QQmlComponent::Ready) {
            qmlWarning(this) << "failed to create theme '" << effectiveThemeName << "': " << effectiveThemeComponent->errorString();
        } else {
            /* The 'createThemeInsideStyle' JS function is a work-around since we haven't found
             * a way to instantiate a Theme inside the context of a Style from c++. Doing so is
             * needed in order to allow custom style properties to be added as children of a
             * Style, and at the same time, be able to access them from within a Theme. For
             * this to work, the Style also needs to set 'pragma ComponentBehavior: Bound'. */
            QVariant themeAsVariant;
            QMetaObject::invokeMethod(this, "createThemeInsideStyle", Qt::DirectConnection,
                qReturnArg(themeAsVariant), QVariant::fromValue(effectiveThemeComponent));
            m_theme = qvariant_cast<QQStyleKitTheme *>(themeAsVariant);

            if (!m_theme || !effectiveThemeComponent->errorString().isEmpty()) {
                qmlWarning(this) << "failed to create theme '" << effectiveThemeName << "': " << effectiveThemeComponent->errorString();
            } else {
                m_theme->setParent(this);
            }
        }
    }

    if (!m_theme) {
        // We always require a theme, even if it's empty
        m_theme = new QQStyleKitTheme(this);
        m_theme->setObjectName("<empty theme>"_L1);
        m_theme->m_completed = true;
    } else {
        m_theme->setParent(this);
    }

    if (m_theme->fonts())
        m_theme->fonts()->setFallbackFont(fonts());
    if (m_theme->palettes())
        m_theme->palettes()->setFallbackPalette(palettes());

    reapplyStyle();

    emit themeChanged();
}

void QQStyleKitStyle::reapplyStyle()
{
    if (!loaded())
        return;

    m_theme->updateThemePalettes();
    m_theme->updateThemeFonts();
    QQStyleKitVariation::resetVariationsForStyle(this);
    QQStyleKitReader::resetReadersForStyle(this);
}

QQStyleKitStyle* QQStyleKitStyle::current()
{
    return QQStyleKit::qmlAttachedProperties()->style();
}

QPalette QQStyleKitStyle::paletteForControlType(QQStyleKitExtendableControlType type) const
{
    return m_theme->paletteForControlType(type);
}

QFont QQStyleKitStyle::fontForControlType(QQStyleKitExtendableControlType type) const
{
    return m_theme->fontForControlType(type);
}

bool QQStyleKitStyle::loaded() const
{
    /* Before both the style and theme has completed loading
     * we return false. This can be used to avoid unnecessary
     * property reads when we anyway have to do a full update
     * in the end. */
    if (!m_completed)
        return false;
    if (!m_theme || !m_theme->m_completed)
        return false;

    return true;
}

void QQStyleKitStyle::executeFallbackStyle(bool complete)
{
    if (m_fallbackStyle.wasExecuted())
        return;

    const QString name = "fallbackStyle"_L1;
    if (!m_fallbackStyle || complete)
        quickBeginDeferred(this, name, m_fallbackStyle);
    if (complete)
        quickCompleteDeferred(this, name, m_fallbackStyle);
}

void QQStyleKitStyle::syncFromQPalette(const QPalette &palette)
{
    if (m_isUpdatingPalette)
        return;
    QScopedValueRollback<bool> rb(m_isUpdatingPalette, true);
    if (palette == m_effectivePalette)
        return;
    m_effectivePalette = palette;
    m_paletteProxy->fromQPalette(m_effectivePalette);
    emit paletteChanged();
}

QPalette QQStyleKitStyle::effectivePalette() const
{
    return m_effectivePalette;
}

void QQStyleKitStyle::componentComplete()
{
    QQStyleKitControls::componentComplete();

    executeFallbackStyle(true);

    /* If the style is top-level (i.e not a fallback style), we instantiate its theme and
     * tell all relevant StyleReaders to update.
     * Note: as an optimization, we currently don't support fallback styles to have their own
     * themes, simply because it reduces the number of layers that need to be searched during
     * property resolution. But we might need to reconsider implementing support for this later. */
    const bool isTopLevelStyle = !qobject_cast<QQStyleKitStyle *>(parent());
    if (isTopLevelStyle) {
        parseThemes();
        recreateTheme();
        /* It's important to set m_completed to true before resetting the readers, otherwise
         * QQStyleKitStyle::loaded() will still be false, which will cause property reads to
         * trigger the "skip read" optimization from QQStyleKitPropertyResolver */
        m_completed = true;
        reapplyStyle();
    }

    m_completed = true;
}

QT_END_NAMESPACE

#include "moc_qqstylekitstyle_p.cpp"
