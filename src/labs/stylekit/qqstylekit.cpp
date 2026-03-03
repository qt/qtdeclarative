// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekit_p.h"
#include "qqstylekitstyle_p.h"
#include "qqstylekittheme_p.h"
#include "qqstylekitreader_p.h"

#include <QtGui/QStyleHints>

QT_BEGIN_NAMESPACE

QPointer<QQStyleKitAttached> QQStyleKitAttached::s_instance;
bool QQStyleKitAttached::s_transitionsEnabled = true;

/*!
    \qmltype StyleKit
    \inqmlmodule Qt.labs.StyleKit
    \brief A singleton for setting and accessing the current style.

    \c StyleKit is a singleton for setting the application \l Style.

    \snippet PlainStyleMain.qml 1

    It also provides properties for global StyleKit configuration and debugging.

    \labs

    \sa Style, StyleVariation
*/

/*!
    \qmlproperty Style StyleKit::style

    The active style for the application.

    Set this to a \l Style instance to apply it to all \l {Qt Quick Controls}.

    \snippet PlainStyleMain.qml 1

    \note For best startup performance, set the style on the root \l ApplicationWindow, before
    any child controls.

    \sa styleUrl, Style
*/

/*!
    \qmlproperty string StyleKit::styleUrl

    An alternative to \l style that loads a \l Style from a URL.

    \sa style
*/

/*!
    \qmlproperty bool StyleKit::transitionsEnabled

    Controls whether transitions are enabled globally.

    Set this to \c false to disable all transitions in the
    current style.

    The default value is \c true.

    \sa {ControlStyle::transition}
*/

/*!
    \qmlproperty StyleKitDebug StyleKit::debug
    \readonly

    A run-time debugging tool that can be used to trace
    style properties for a specific \l Control.
*/

/*!
    \qmlmethod bool StyleKit::styleLoaded()

    Returns \c true when the active \l style and its current \l {Style::}{theme} have both
    finished loading. This can be used to defer initialization logic that
    depends on resolved style properties.

    \sa style, {Style::}{theme}
*/

QQStyleKit::QQStyleKit(QObject *parent)
    : QObject(parent)
{
}

QQStyleKitAttached *QQStyleKit::qmlAttachedProperties(QObject *obj)
{
    /* QQStyleKitAttached is a singleton. It doesn't matter where it's
     * used from in the application, it will always represent the same
     * application global style and theme. */
    if (!QQStyleKitAttached::s_instance)
        QQStyleKitAttached::s_instance = new QQStyleKitAttached(QGuiApplication::instance());
    if (!QQStyleKitAttached::s_instance->m_engine && obj)
        QQStyleKitAttached::s_instance->m_engine = qmlEngine(obj);
    return QQStyleKitAttached::s_instance.get();
}

QQStyleKitAttached::QQStyleKitAttached(QObject *parent)
    : QObject(parent)
{
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this](){
        if (m_style)
            m_style->recreateTheme();
    });
}

QQStyleKitAttached::~QQStyleKitAttached()
{
    QQStyleKitAttached::s_instance = nullptr;
    if (m_ownsStyle) {
        delete m_style;
        m_style = nullptr;
    }
}

QQStyleKitStyle *QQStyleKitAttached::style() const
{
    return m_style;
}

void QQStyleKitAttached::setStyle(QQStyleKitStyle *style)
{
    if (m_style == style)
        return;

    if (m_ownsStyle)
        m_style->deleteLater();

    m_style = style;

    if (m_style && m_style->m_theme) {
        m_style->m_theme->updateThemePalettes();
        m_style->m_theme->updateThemeFonts();
    }
    if (m_style->loaded())
        QQStyleKitReader::resetAll();

    emit styleChanged();
}

QString QQStyleKitAttached::styleUrl() const
{
    return m_styleUrl;
}

void QQStyleKitAttached::setStyleUrl(const QString &styleUrl)
{
    if (m_styleUrl == styleUrl)
        return;

    m_styleUrl = styleUrl;

    Q_ASSERT(m_engine);
    QQmlComponent comp(m_engine, QUrl(styleUrl), this);
    if (!comp.errors().isEmpty()) {
        qmlWarning(this) << "Could not create a StyleKit style: " << comp.errorString();
        return;
    }
    auto *style = qobject_cast<QQStyleKitStyle *>(comp.create());
    if (!style) {
        qmlWarning(this) << "Could not create a StyleKit style from url: " << styleUrl;
        return;
    }

    style->componentComplete();
    setStyle(style);
    m_ownsStyle = true;
    emit styleUrlChanged();
}

bool QQStyleKitAttached::transitionsEnabled() const
{
    return s_transitionsEnabled;
}

void QQStyleKitAttached::setTransitionsEnabled(bool enabled)
{
    if (enabled == s_transitionsEnabled)
        return;

    s_transitionsEnabled = enabled;
    emit transitionsEnabledChanged();
}

QQStyleKitDebug *QQStyleKitAttached::debug() const
{
    return &const_cast<QQStyleKitAttached *>(this)->m_debug;
}

bool QQStyleKitAttached::styleLoaded() const
{
    return m_style && m_style->loaded();
}

QT_END_NAMESPACE

#include "moc_qqstylekit_p.cpp"

