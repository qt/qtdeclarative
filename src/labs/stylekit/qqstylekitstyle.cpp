// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekit_p.h"
#include "qqstylekitstyle_p.h"
#include "qqstylekittheme_p.h"
#include "qqstylekitcustomtheme_p.h"
#include "qqstylekitcontrolproperties_p.h"
#include "qqstylekitpropertyresolver_p.h"

#include <QtQuickTemplates2/private/qquickdeferredexecute_p_p.h>
#include <QtQml/private/qqmllist_p.h>

#include <QtGui/QGuiApplication>
#include <QtGui/QStyleHints>

QT_BEGIN_NAMESPACE

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

QList<QObject *> QQStyleKitStyle::customThemesAsList()
{
    QList<QObject *> list;
    for (auto *customTheme : customThemes())
        list.append(customTheme);
    return list;
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
    m_themeNames = QStringList({kSystem, kLight, kDark});

    for (auto *customTheme : customThemes()) {
        const QString name = customTheme->name();
        if (name.isEmpty())
            continue;
        m_themeNames << name;
        m_customThemeNames << name;
    }

    emit themeNamesChanged();
    emit customThemeNamesChanged();
}

QString QQStyleKitStyle::themeName() const
{
    return m_themeName;
}

QStringList QQStyleKitStyle::themeNames() const
{
    return m_themeNames;
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
    recreateTheme();

    emit themeNameChanged();
}

void QQStyleKitStyle::recreateTheme()
{
    QString effectiveThemeName;
    QQmlComponent *effectiveThemeComponent = nullptr;

    if (QString::compare(m_themeName, kSystem, Qt::CaseInsensitive) == 0) {
        const auto scheme = QGuiApplication::styleHints()->colorScheme();
        if (scheme == Qt::ColorScheme::Light) {
            effectiveThemeName = kLight;
            effectiveThemeComponent = m_light;
        }
        else if (scheme == Qt::ColorScheme::Dark) {
            effectiveThemeName =kDark;
            effectiveThemeComponent = m_dark;
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
            qmlWarning(this) << "No theme found with name:" << m_themeName;
        else if (!effectiveThemeComponent)
            qmlWarning(this) << "Custom theme '" << effectiveThemeName << "' has no theme component set";
    }

    if (m_effectiveThemeName == effectiveThemeName) {
        // Switching theme name from e.g "System" to "Light" might not
        // actually change the currently effective theme.
        emit themeNameChanged();
        return;
    }

    if (m_theme) {
        m_theme->deleteLater();
        m_theme = nullptr;
    }

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
    if (this == current()) {
        m_theme->updateQuickTheme();
        QQStyleKitReader::resetAll();
    }

    emit themeChanged();
}

QQStyleKitStyle* QQStyleKitStyle::current()
{
    return QQStyleKit::qmlAttachedProperties()->style();
}

QFont QQStyleKitStyle::fontForReader(QQStyleKitReader *reader) const
{
    switch (reader->type()) {
        case QQStyleKitReader::ControlType::AbstractButton:
        case QQStyleKitReader::ControlType::Button:
        case QQStyleKitReader::ControlType::FlatButton:
            return m_theme->fonts()->button();
        case QQStyleKitReader::ControlType::CheckBox:
            return m_theme->fonts()->checkBox();
        case QQStyleKitReader::ControlType::ComboBox:
            return m_theme->fonts()->comboBox();
        case QQStyleKitReader::ControlType::GroupBox:
            return m_theme->fonts()->groupBox();
        case QQStyleKitReader::ControlType::RadioButton:
            return m_theme->fonts()->radioButton();
        case QQStyleKitReader::ControlType::SpinBox:
            return m_theme->fonts()->spinBox();
        case QQStyleKitReader::ControlType::SwitchControl:
            return m_theme->fonts()->switchControl();
        case QQStyleKitReader::ControlType::TabBar:
        case QQStyleKitReader::ControlType::TabButton:
            return m_theme->fonts()->tabBar();
        case QQStyleKitReader::ControlType::TextInput:
        case QQStyleKitReader::ControlType::TextField:
            return m_theme->fonts()->textField();
        case QQStyleKitReader::ControlType::TextArea:
            return m_theme->fonts()->textArea();
        case QQStyleKitReader::ControlType::ToolBar:
        case QQStyleKitReader::ControlType::ToolButton:
        case QQStyleKitReader::ControlType::ToolSeparator:
            return m_theme->fonts()->toolBar();
        case QQStyleKitReader::ControlType::ItemDelegate:
            return m_theme->fonts()->itemView();
        case QQStyleKitReader::ControlType::Label:
            return m_theme->fonts()->label();
        default:
            return m_theme->fonts()->system();
    }
}

QPalette QQStyleKitStyle::paletteForReader(QQStyleKitReader *reader) const
{
    return m_theme->paletteForReader(reader);
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

    /* It's important to set m_completed before creating the theme, otherwise
     * styleAndThemeFinishedLoading() will still be false, which will e.g cause
     * property reads to return early from QQStyleKitPropertyResolver */
    m_completed = true;

    executeFallbackStyle(true);
    parseThemes();
    recreateTheme();
}

QT_END_NAMESPACE

#include "moc_qqstylekitstyle_p.cpp"

