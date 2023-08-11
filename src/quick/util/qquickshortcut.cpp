// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickshortcut_p.h"

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickrendercontrol.h>
#include <QtQuick/private/qtquickglobal_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/qqmlinfo.h>

/*!
    \qmltype Shortcut
    \instantiates QQuickShortcut
    \inqmlmodule QtQuick
    \since 5.5
    \ingroup qtquick-input
    \brief Provides keyboard shortcuts.

    The Shortcut type lets you handle keyboard shortcuts. The shortcut can
    be set to one of the
    \l{QKeySequence::StandardKey}{standard keyboard shortcuts},
    or it can be described with a string containing a sequence of up to four key
    presses that are needed to \l{Shortcut::activated}{activate} the shortcut.

    \qml
    Item {
        id: view

        property int currentIndex

        Shortcut {
            sequences: [StandardKey.NextChild]
            onActivated: view.currentIndex++
        }
    }
    \endqml

    It is also possible to set multiple shortcut \l sequences, so that the shortcut
    can be \l activated via several different sequences of key presses.

    \sa Keys, {Keys::}{shortcutOverride()}
*/

/*! \qmlsignal QtQuick::Shortcut::activated()

    This signal is emitted when the shortcut is activated.
*/

/*! \qmlsignal QtQuick::Shortcut::activatedAmbiguously()

    This signal is emitted when the shortcut is activated ambigously,
    meaning that it matches the start of more than one shortcut.
*/

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
        if (QWindow *renderWindow = QQuickRenderControl::renderWindowFor(qobject_cast<QQuickWindow *>(obj)))
            obj = renderWindow;
        return obj && obj == QGuiApplication::focusWindow();
    default:
        return false;
    }
}

typedef bool (*ContextMatcher)(QObject *, Qt::ShortcutContext);

Q_GLOBAL_STATIC_WITH_ARGS(ContextMatcher, ctxMatcher, (qQuickShortcutContextMatcher))

Q_QUICK_PRIVATE_EXPORT ContextMatcher qt_quick_shortcut_context_matcher()
{
    return *ctxMatcher();
}

Q_QUICK_PRIVATE_EXPORT void qt_quick_set_shortcut_context_matcher(ContextMatcher matcher)
{
    if (!ctxMatcher.isDestroyed())
        *ctxMatcher() = matcher;
}

QT_BEGIN_NAMESPACE

static QKeySequence valueToKeySequence(const QVariant &value, const QQuickShortcut *const shortcut)
{
    if (value.userType() == QMetaType::Int) {
        const QVector<QKeySequence> s =
                QKeySequence::keyBindings(static_cast<QKeySequence::StandardKey>(value.toInt()));
        if (s.size() > 1) {
            const QString templateString = QString::fromUtf16(
                    u"Shortcut: Only binding to one of multiple key bindings associated with %1. "
                    u"Use 'sequences: [ <key> ]' to bind to all of them.");
            qmlWarning(shortcut)
                    << templateString.arg(static_cast<QKeySequence::StandardKey>(value.toInt()));
        }
        return s.size() > 0 ? s[0] : QKeySequence {};
    }

    return QKeySequence::fromString(value.toString());
}

static QList<QKeySequence> valueToKeySequences(const QVariant &value)
{
    if (value.userType() == QMetaType::Int) {
        return QKeySequence::keyBindings(static_cast<QKeySequence::StandardKey>(value.toInt()));
    } else {
        QList<QKeySequence> result;
        result.push_back(QKeySequence::fromString(value.toString()));
        return result;
    }
}

QQuickShortcut::QQuickShortcut(QObject *parent) : QObject(parent),
    m_enabled(true), m_completed(false), m_autorepeat(true), m_context(Qt::WindowShortcut)
{
}

QQuickShortcut::~QQuickShortcut()
{
    ungrabShortcut(m_shortcut);
    for (Shortcut &shortcut : m_shortcuts)
        ungrabShortcut(shortcut);
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

    \note Given that standard keys can resolve to one shortcut on some
    platforms, but multiple shortcuts on other platforms, we recommend always
    using \l{Shortcut::}{sequences} for standard keys.

    \sa sequences
*/
QVariant QQuickShortcut::sequence() const
{
    return m_shortcut.userValue;
}

void QQuickShortcut::setSequence(const QVariant &value)
{
    if (value == m_shortcut.userValue)
        return;

    QKeySequence keySequence = valueToKeySequence(value, this);

    ungrabShortcut(m_shortcut);
    m_shortcut.userValue = value;
    m_shortcut.keySequence = keySequence;
    grabShortcut(m_shortcut, m_context);
    emit sequenceChanged();
}

/*!
    \qmlproperty list<keysequence> QtQuick::Shortcut::sequences
    \since 5.9

    This property holds multiple key sequences for the shortcut. The key sequences
    can be set to one of the \l{QKeySequence::StandardKey}{standard keyboard shortcuts},
    or they can be described with strings containing sequences of up to four key
    presses that are needed to \l{Shortcut::activated}{activate} the shortcut.

    \qml
    Shortcut {
        sequences: [StandardKey.Cut, "Ctrl+X", "Shift+Del"]
        onActivated: edit.cut()
    }
    \endqml
*/
QVariantList QQuickShortcut::sequences() const
{
    QVariantList values;
    for (const Shortcut &shortcut : m_shortcuts)
        values += shortcut.userValue;
    return values;
}

void QQuickShortcut::setSequences(const QVariantList &values)
{
    // convert QVariantList to QVector<QKeySequence>
    QVector<Shortcut> requestedShortcuts;
    for (const QVariant &v : values) {
        const QVector<QKeySequence> list = valueToKeySequences(v);
        for (const QKeySequence &s : list) {
            Shortcut sc;
            sc.userValue = v;
            sc.keySequence = s;
            requestedShortcuts.push_back(sc);
        }
    }

    // if nothing has changed, just return:
    if (m_shortcuts.size() == requestedShortcuts.size()) {
        bool changed = false;
        for (int i = 0; i < requestedShortcuts.size(); ++i) {
            const Shortcut &requestedShortcut = requestedShortcuts[i];
            const Shortcut &shortcut = m_shortcuts[i];
            if (!(requestedShortcut.userValue == shortcut.userValue
                  && requestedShortcut.keySequence == shortcut.keySequence)) {
                changed = true;
                break;
            }
        }
        if (!changed) {
            return;
        }
    }

    for (Shortcut &s : m_shortcuts)
        ungrabShortcut(s);
    m_shortcuts = requestedShortcuts;
    for (Shortcut &s : m_shortcuts)
        grabShortcut(s, m_context);

    emit sequencesChanged();
}

/*!
    \qmlproperty string QtQuick::Shortcut::nativeText
    \since 5.6

    This property provides the shortcut's key sequence as a platform specific
    string. This means that it will be shown translated, and on \macos it will
    resemble a key sequence from the menu bar. It is best to display this text
    to the user (for example, on a tooltip).

    \sa sequence, portableText
*/
QString QQuickShortcut::nativeText() const
{
    return m_shortcut.keySequence.toString(QKeySequence::NativeText);
}

/*!
    \qmlproperty string QtQuick::Shortcut::portableText
    \since 5.6

    This property provides the shortcut's key sequence as a string in a
    "portable" format, suitable for reading and writing to a file. In many
    cases, it will look similar to the native text on Windows and X11.

    \sa sequence, nativeText
*/
QString QQuickShortcut::portableText() const
{
    return m_shortcut.keySequence.toString(QKeySequence::PortableText);
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

    setEnabled(m_shortcut, enabled);
    for (Shortcut &shortcut : m_shortcuts)
        setEnabled(shortcut, enabled);

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

    setAutoRepeat(m_shortcut, repeat);
    for (Shortcut &shortcut : m_shortcuts)
        setAutoRepeat(shortcut, repeat);

    m_autorepeat = repeat;
    emit autoRepeatChanged();
}

/*!
    \qmlproperty enumeration QtQuick::Shortcut::context

    This property holds the \l{Qt::ShortcutContext}{shortcut context}.

    Supported values are:

    \value Qt.WindowShortcut
        (default) The shortcut is active when its parent item is in an active top-level window.
    \value Qt.ApplicationShortcut
        The shortcut is active when one of the application's windows are active.

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

    ungrabShortcut(m_shortcut);
    for (auto &s : m_shortcuts)
        ungrabShortcut(s);

    m_context = context;

    grabShortcut(m_shortcut, context);
    for (auto &s : m_shortcuts)
        grabShortcut(s, context);

    emit contextChanged();
}

void QQuickShortcut::classBegin()
{
}

void QQuickShortcut::componentComplete()
{
    m_completed = true;
    grabShortcut(m_shortcut, m_context);
    for (Shortcut &shortcut : m_shortcuts)
        grabShortcut(shortcut, m_context);
}

bool QQuickShortcut::event(QEvent *event)
{
    if (m_enabled && event->type() == QEvent::Shortcut) {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(event);
        bool match = m_shortcut.matches(se);
        int i = 0;
        while (!match && i < m_shortcuts.size())
            match |= m_shortcuts.at(i++).matches(se);
        if (match) {
            if (se->isAmbiguous())
                emit activatedAmbiguously();
            else
                emit activated();
            return true;
        }
    }
    return false;
}

bool QQuickShortcut::Shortcut::matches(QShortcutEvent *event) const
{
    return event->shortcutId() == id && event->key() == keySequence;
}

void QQuickShortcut::setEnabled(QQuickShortcut::Shortcut &shortcut, bool enabled)
{
    if (shortcut.id)
        QGuiApplicationPrivate::instance()->shortcutMap.setShortcutEnabled(enabled, shortcut.id, this);
}

void QQuickShortcut::setAutoRepeat(QQuickShortcut::Shortcut &shortcut, bool repeat)
{
    if (shortcut.id)
        QGuiApplicationPrivate::instance()->shortcutMap.setShortcutAutoRepeat(repeat, shortcut.id, this);
}

void QQuickShortcut::grabShortcut(Shortcut &shortcut, Qt::ShortcutContext context)
{
    if (m_completed && !shortcut.keySequence.isEmpty()) {
        QGuiApplicationPrivate *pApp = QGuiApplicationPrivate::instance();
        shortcut.id = pApp->shortcutMap.addShortcut(this, shortcut.keySequence, context, *ctxMatcher());
        if (!m_enabled)
            pApp->shortcutMap.setShortcutEnabled(false, shortcut.id, this);
        if (!m_autorepeat)
            pApp->shortcutMap.setShortcutAutoRepeat(false, shortcut.id, this);
    }
}

void QQuickShortcut::ungrabShortcut(Shortcut &shortcut)
{
    if (shortcut.id) {
        QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(shortcut.id, this);
        shortcut.id = 0;
    }
}

QT_END_NAMESPACE

#include "moc_qquickshortcut_p.cpp"
