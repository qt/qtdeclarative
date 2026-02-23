// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquickinputmethod_p.h"

#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

/*!
    \qmlsingletontype InputMethod
    \inqmlmodule QtQuick

    \brief Provides access to \l QInputMethod for QML applications.

    The InputMethod singleton allows access to application's \l QInputMethod object
    and all its properties and slots. See the \l QInputMethod documentation for
    further details.

    \since 6.4
*/

/*!
    \qmlproperty rect InputMethod::cursorRectangle

    \brief Input item's cursor rectangle in window coordinates.

    The cursor rectangle is often used by various text editing controls
    like text prediction popups for following the text being typed.
*/

/*!
    \qmlproperty rect InputMethod::anchorRectangle

    \brief Input item's anchor rectangle in window coordinates.

    The anchor rectangle is often used by various text editing controls
    like text prediction popups for following the text selection.
*/

/*!
    \qmlproperty rect InputMethod::keyboardRectangle
    \brief Virtual keyboard's geometry in window coordinates.

    This might be an empty rectangle if it is not possible to know the geometry
    of the keyboard. This is the case for a floating keyboard on android.
*/

/*!
    \qmlproperty rect InputMethod::inputItemClipRectangle
    \brief Input item's clipped rectangle in window coordinates.

    The clipped input rectangle is often used by various input methods to determine
    how much screen real estate is available for the input method (e.g. Virtual Keyboard).
*/

/*!
    \qmlproperty bool InputMethod::visible
    \brief Virtual keyboard's visibility on the screen

    The value of this property remains false for devices
    with no virtual keyboards.

    \sa show(), hide()
*/

/*!
    \qmlproperty bool InputMethod::animating
    \brief True when the virtual keyboard is being opened or closed.

    The value of this property is false when keyboard is fully open or closed.
    When \c animating is \c true and \c visibility is \c true keyboard
    is being opened. When \c animating is \c true and \c visibility is
    false keyboard is being closed.
*/

/*!
    \qmlproperty Locale InputMethod::locale
    \brief Current input locale.
*/

/*!
    \qmlproperty enumeration InputMethod::inputDirection
    \brief Current input direction.

    Possible values:

    \value Qt.LeftToRight   (default) Items are laid out from left to right.
    \value Qt.RightToLeft   Items are laid out from right to left.
 */

/*!
    \qmlmethod void InputMethod::commit()

    Commits the word that the user is currently composing to the editor. The function is
    mostly needed by the input methods with text prediction features and by the
    methods where the script used for typing characters is different from the
    script that actually gets appended to the editor. Any kind of action that
    interrupts the text composing needs to flush the composing state by calling the
    commit() function, for example when the cursor is moved elsewhere.
*/

/*!
    \qmlmethod void InputMethod::hide()

    Requests the virtual keyboard to close.

    Normally applications should not need to call this function,
    as the keyboard should close automatically when the text editor loses
    focus, for example when the parent view is closed.
*/

/*!
    \qmlmethod void InputMethod::invokeAction(var a, int cursorPosition)

    Called by the input item when the word currently being composed is tapped by
    the user, as indicated by the action \a a and the given \a cursorPosition.
    Input methods often use this information to offer more word suggestions to the user.

    \sa QInputMethod::Action
*/

/*!
    \qmlmethod void InputMethod::reset()

    Resets the input method state. For example, a text editor normally calls
    this method before inserting a text to make widget ready to accept a text.

    Input method resets automatically when the focused editor changes.
*/

/*!
    \qmlmethod void InputMethod::show()

    Requests the virtual keyboard to open. If the platform
    doesn't provide a virtual keyboard the visibility
    remains false.

    Normally applications should not need to call this
    function, as the keyboard should open automatically when
    the text editor gains focus.
*/

/*!
    \qmlmethod void InputMethod::update(enumeration queries)

    Called by the input item to inform the platform input methods when there have been
    state changes in the editor's input method query attributes. When calling the function
    \a queries parameter has to be used to tell what has changed. An input method
    can use this to make queries for attributes it is interested in with QInputMethodQueryEvent.

    In particular calling update() whenever the cursor position changes is important as
    that often causes other query attributes, like surrounding text and text selection,
    to change as well. The attributes that often change together with cursor position
    have been grouped in Qt::ImQueryInput value for convenience.

    \sa Qt::InputMethodQueries
*/

QQuickInputMethod::QQuickInputMethod(QObject *parent) : QObject(parent)
{
    QInputMethod *inputMethod = QGuiApplication::inputMethod();
    connect(inputMethod, &QInputMethod::anchorRectangleChanged, this,
            &QQuickInputMethod::anchorRectangleChanged);
    connect(inputMethod, &QInputMethod::animatingChanged, this,
            &QQuickInputMethod::animatingChanged);
    connect(inputMethod, &QInputMethod::cursorRectangleChanged, this,
            &QQuickInputMethod::cursorRectangleChanged);
    connect(inputMethod, &QInputMethod::inputDirectionChanged, this,
            &QQuickInputMethod::inputDirectionChanged);
    connect(inputMethod, &QInputMethod::inputItemClipRectangleChanged, this,
            &QQuickInputMethod::inputItemClipRectangleChanged);
    connect(inputMethod, &QInputMethod::keyboardRectangleChanged, this,
            &QQuickInputMethod::keyboardRectangleChanged);
    connect(inputMethod, &QInputMethod::localeChanged, this, &QQuickInputMethod::localeChanged);
    connect(inputMethod, &QInputMethod::visibleChanged, this, &QQuickInputMethod::visibleChanged);
}

void QQuickInputMethod::commit()
{
    QGuiApplication::inputMethod()->commit();
}
void QQuickInputMethod::hide()
{
    QGuiApplication::inputMethod()->hide();
}
void QQuickInputMethod::invokeAction(QInputMethod::Action a, int cursorPosition)
{
    QGuiApplication::inputMethod()->invokeAction(a, cursorPosition);
}
void QQuickInputMethod::reset()
{
    QGuiApplication::inputMethod()->reset();
}
void QQuickInputMethod::show()
{
    QGuiApplication::inputMethod()->show();
}
void QQuickInputMethod::update(Qt::InputMethodQueries queries)
{
    QGuiApplication::inputMethod()->update(queries);
}

QRectF QQuickInputMethod::anchorRectangle() const
{
    return QGuiApplication::inputMethod()->cursorRectangle();
}
QRectF QQuickInputMethod::cursorRectangle() const
{
    return QGuiApplication::inputMethod()->cursorRectangle();
}
Qt::LayoutDirection QQuickInputMethod::inputDirection() const
{
    return QGuiApplication::inputMethod()->inputDirection();
}
QRectF QQuickInputMethod::inputItemClipRectangle() const
{
    return QGuiApplication::inputMethod()->inputItemClipRectangle();
}

QRectF QQuickInputMethod::inputItemRectangle() const
{
    return QGuiApplication::inputMethod()->inputItemRectangle();
}
void QQuickInputMethod::setInputItemRectangle(const QRectF &rect)
{
    QGuiApplication::inputMethod()->setInputItemRectangle(rect);
}

QTransform QQuickInputMethod::inputItemTransform() const
{
    return QGuiApplication::inputMethod()->inputItemTransform();
}
void QQuickInputMethod::setInputItemTransform(const QTransform &transform)
{
    QGuiApplication::inputMethod()->setInputItemTransform(transform);
}

bool QQuickInputMethod::isAnimating() const
{
    return QGuiApplication::inputMethod()->isAnimating();
}

bool QQuickInputMethod::isVisible() const
{
    return QGuiApplication::inputMethod()->isVisible();
}
void QQuickInputMethod::setVisible(bool visible)
{
    QGuiApplication::inputMethod()->setVisible(visible);
}

QRectF QQuickInputMethod::keyboardRectangle() const
{
    return QGuiApplication::inputMethod()->keyboardRectangle();
}
QLocale QQuickInputMethod::locale() const
{
    return QGuiApplication::inputMethod()->locale();
}

QT_END_NAMESPACE

#include "moc_qquickinputmethod_p.cpp"
