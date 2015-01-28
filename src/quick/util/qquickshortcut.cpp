/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickshortcut_p.h"

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Shortcut
    \instantiates QQuickShortcut
    \inqmlmodule QtQuick
    \since 5.5
    \ingroup qtquick-input
    \brief Provides keyboard shortcuts

    The Shortcut type provides a way of handling keyboard shortcuts. The shortcut can
    be set to one of the \l{QKeySequence::StandardKey}{standard keyboard shortcuts},
    or it can be described with a string containing a sequence of up to four key
    presses that are needed to \l{Shortcut::activated}{activate} the shortcut.

    \qml
    Item {
        id: view

        property int currentIndex

        Shortcut {
            sequence: StandardKey.NextChild
            onActivated: view.currentIndex++
        }
    }
    \endqml

    \sa Keys
*/

/*! \qmlsignal QtQuick::Shortcut::activated()

    This signal is emitted when the shortcut is activated.

    The corresponding handler is \c onActivated.
*/

/*! \qmlsignal QtQuick::Shortcut::activatedAmbiguously()

    This signal is emitted when the shortcut is activated ambigously,
    meaning that it matches the start of more than one shortcut.

    The corresponding handler is \c onActivatedAmbiguously.
*/

QQuickShortcut::QQuickShortcut(QObject *parent) : QObject(parent), m_id(0),
    m_enabled(true), m_completed(false), m_autorepeat(true), m_context(Qt::WindowShortcut)
{
}

QQuickShortcut::~QQuickShortcut()
{
    ungrabShortcut();
}

/*!
    \qmlproperty keysequence QtQuick::Shortcut::sequence

    This property holds the shortcut's key sequence. The key sequence can be set
    to one of the \l{QKeySequence::StandardKey}{standard keyboard shortcuts}, or
    it can be described with a string containing a sequence of up to four key
    presses that are needed to \l{Shortcut::activated}{activate} the shortcut.

    The default value is an empty key sequence.

    \qml
    Shortcut {
        sequence: "Ctrl+E,Ctrl+W"
        onActivated: edit.wrapMode = TextEdit.Wrap
    }
    \endqml
*/
QVariant QQuickShortcut::sequence() const
{
    return m_sequence;
}

void QQuickShortcut::setSequence(const QVariant &sequence)
{
    if (sequence == m_sequence)
        return;

    QKeySequence shortcut;
    if (sequence.type() == QVariant::Int)
        shortcut = QKeySequence(static_cast<QKeySequence::StandardKey>(sequence.toInt()));
    else
        shortcut = QKeySequence::fromString(sequence.toString());

    grabShortcut(shortcut, m_context);

    m_sequence = sequence;
    m_shortcut = shortcut;
    emit sequenceChanged();
}

/*!
    \qmlproperty bool QtQuick::Shortcut::enabled

    This property holds whether the shortcut is enabled.

    The default value is \c true.
*/
bool QQuickShortcut::isEnabled() const
{
    return m_enabled;
}

void QQuickShortcut::setEnabled(bool enabled)
{
    if (enabled == m_enabled)
        return;

    if (m_id)
        QGuiApplicationPrivate::instance()->shortcutMap.setShortcutEnabled(enabled, m_id, this);

    m_enabled = enabled;
    emit enabledChanged();
}

/*!
    \qmlproperty bool QtQuick::Shortcut::autoRepeat

    This property holds whether the shortcut can auto repeat.

    The default value is \c true.
*/
bool QQuickShortcut::autoRepeat() const
{
    return m_autorepeat;
}

void QQuickShortcut::setAutoRepeat(bool repeat)
{
    if (repeat == m_autorepeat)
        return;

    if (m_id)
        QGuiApplicationPrivate::instance()->shortcutMap.setShortcutAutoRepeat(repeat, m_id, this);

    m_autorepeat = repeat;
    emit autoRepeatChanged();
}

/*!
    \qmlproperty enumeration QtQuick::Shortcut::context

    This property holds the \l{Qt::ShortcutContext}{shortcut context}.

    Supported values are:
    \list
    \li \c Qt.WindowShortcut (default) - The shortcut is active when its parent item is in an active top-level window.
    \li \c Qt.ApplicationShortcut - The shortcut is active when one of the application's windows are active.
    \endlist

    \qml
    Shortcut {
        sequence: StandardKey.Quit
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }
    \endqml
*/
Qt::ShortcutContext QQuickShortcut::context() const
{
    return m_context;
}

void QQuickShortcut::setContext(Qt::ShortcutContext context)
{
    if (context == m_context)
        return;

    grabShortcut(m_shortcut, context);

    m_context = context;
    emit contextChanged();
}

void QQuickShortcut::classBegin()
{
}

void QQuickShortcut::componentComplete()
{
    m_completed = true;
    grabShortcut(m_shortcut, m_context);
}

bool QQuickShortcut::event(QEvent *event)
{
    if (m_enabled && event->type() == QEvent::Shortcut) {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(event);
        if (se->shortcutId() == m_id && se->key() == m_shortcut){
            if (se->isAmbiguous())
                emit activatedAmbiguously();
            else
                emit activated();
            return true;
        }
    }
    return false;
}

static bool qQuickShortcutContextMatcher(QObject *obj, Qt::ShortcutContext context)
{
    switch (context) {
    case Qt::ApplicationShortcut:
        return true;
    case Qt::WindowShortcut:
        while (obj && !obj->isWindowType()) {
            obj = obj->parent();
            if (QQuickItem *item = qobject_cast<QQuickItem *>(obj))
                obj = item->window();
        }
        return obj && obj == QGuiApplication::focusWindow();
    default:
        return false;
    }
}

void QQuickShortcut::grabShortcut(const QKeySequence &sequence, Qt::ShortcutContext context)
{
    ungrabShortcut();

    if (m_completed && !sequence.isEmpty()) {
        QGuiApplicationPrivate *pApp = QGuiApplicationPrivate::instance();
        m_id = pApp->shortcutMap.addShortcut(this, sequence, context, qQuickShortcutContextMatcher);
        if (!m_enabled)
            pApp->shortcutMap.setShortcutEnabled(false, m_id, this);
        if (!m_autorepeat)
            pApp->shortcutMap.setShortcutAutoRepeat(false, m_id, this);
    }
}

void QQuickShortcut::ungrabShortcut()
{
    if (m_id) {
        QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(m_id, this);
        m_id = 0;
    }
}

QT_END_NAMESPACE
