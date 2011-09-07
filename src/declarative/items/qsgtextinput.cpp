/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgtextinput_p.h"
#include "qsgtextinput_p_p.h"
#include "qsgcanvas.h"

#include <private/qdeclarativeglobal_p.h>
#include <private/qsgdistancefieldglyphcache_p.h>

#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtGui/qevent.h>
#include <QTextBoundaryFinder>
#include <qsgtextnode_p.h>
#include <qsgsimplerectnode.h>

#include <QtGui/qstylehints.h>
#include <QtGui/qinputpanel.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)

/*!
    \qmlclass TextInput QSGTextInput
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-visual-elements
    \brief The TextInput item displays an editable line of text.
    \inherits Item

    The TextInput element displays a single line of editable plain text.

    TextInput is used to accept a line of text input. Input constraints
    can be placed on a TextInput item (for example, through a \l validator or \l inputMask),
    and setting \l echoMode to an appropriate value enables TextInput to be used for
    a password input field.

    On Mac OS X, the Up/Down key bindings for Home/End are explicitly disabled.
    If you want such bindings (on any platform), you will need to construct them in QML.

    \sa TextEdit, Text, {declarative/text/textselection}{Text Selection example}
*/
QSGTextInput::QSGTextInput(QSGItem* parent)
: QSGImplicitSizeItem(*(new QSGTextInputPrivate), parent)
{
    Q_D(QSGTextInput);
    d->init();
}

QSGTextInput::~QSGTextInput()
{
}

/*!
    \qmlproperty string QtQuick2::TextInput::text

    The text in the TextInput.
*/
QString QSGTextInput::text() const
{
    Q_D(const QSGTextInput);
    return d->control->text();
}

void QSGTextInput::setText(const QString &s)
{
    Q_D(QSGTextInput);
    if(s == text())
        return;
    d->control->setText(s);
}

/*!
    \qmlproperty string QtQuick2::TextInput::font.family

    Sets the family name of the font.

    The family name is case insensitive and may optionally include a foundry name, e.g. "Helvetica [Cronyx]".
    If the family is available from more than one foundry and the foundry isn't specified, an arbitrary foundry is chosen.
    If the family isn't available a family will be set using the font matching algorithm.
*/

/*!
    \qmlproperty bool QtQuick2::TextInput::font.bold

    Sets whether the font weight is bold.
*/

/*!
    \qmlproperty enumeration QtQuick2::TextInput::font.weight

    Sets the font's weight.

    The weight can be one of:
    \list
    \o Font.Light
    \o Font.Normal - the default
    \o Font.DemiBold
    \o Font.Bold
    \o Font.Black
    \endlist

    \qml
    TextInput { text: "Hello"; font.weight: Font.DemiBold }
    \endqml
*/

/*!
    \qmlproperty bool QtQuick2::TextInput::font.italic

    Sets whether the font has an italic style.
*/

/*!
    \qmlproperty bool QtQuick2::TextInput::font.underline

    Sets whether the text is underlined.
*/

/*!
    \qmlproperty bool QtQuick2::TextInput::font.strikeout

    Sets whether the font has a strikeout style.
*/

/*!
    \qmlproperty real QtQuick2::TextInput::font.pointSize

    Sets the font size in points. The point size must be greater than zero.
*/

/*!
    \qmlproperty int QtQuick2::TextInput::font.pixelSize

    Sets the font size in pixels.

    Using this function makes the font device dependent.
    Use \c pointSize to set the size of the font in a device independent manner.
*/

/*!
    \qmlproperty real QtQuick2::TextInput::font.letterSpacing

    Sets the letter spacing for the font.

    Letter spacing changes the default spacing between individual letters in the font.
    A positive value increases the letter spacing by the corresponding pixels; a negative value decreases the spacing.
*/

/*!
    \qmlproperty real QtQuick2::TextInput::font.wordSpacing

    Sets the word spacing for the font.

    Word spacing changes the default spacing between individual words.
    A positive value increases the word spacing by a corresponding amount of pixels,
    while a negative value decreases the inter-word spacing accordingly.
*/

/*!
    \qmlproperty enumeration QtQuick2::TextInput::font.capitalization

    Sets the capitalization for the text.

    \list
    \o Font.MixedCase - This is the normal text rendering option where no capitalization change is applied.
    \o Font.AllUppercase - This alters the text to be rendered in all uppercase type.
    \o Font.AllLowercase	 - This alters the text to be rendered in all lowercase type.
    \o Font.SmallCaps -	This alters the text to be rendered in small-caps type.
    \o Font.Capitalize - This alters the text to be rendered with the first character of each word as an uppercase character.
    \endlist

    \qml
    TextInput { text: "Hello"; font.capitalization: Font.AllLowercase }
    \endqml
*/

QFont QSGTextInput::font() const
{
    Q_D(const QSGTextInput);
    return d->sourceFont;
}

void QSGTextInput::setFont(const QFont &font)
{
    Q_D(QSGTextInput);
    if (d->sourceFont == font)
        return;

    d->sourceFont = font;
    QFont oldFont = d->font;
    d->font = font;
    if (d->font.pointSizeF() != -1) {
        // 0.5pt resolution
        qreal size = qRound(d->font.pointSizeF()*2.0);
        d->font.setPointSizeF(size/2.0);
    }
    if (oldFont != d->font) {
        d->control->setFont(d->font);
        updateSize();
        updateCursorRectangle();
        if(d->cursorItem){
            d->cursorItem->setHeight(QFontMetrics(d->font).height());
        }
    }
    emit fontChanged(d->sourceFont);
}

/*!
    \qmlproperty color QtQuick2::TextInput::color

    The text color.
*/
QColor QSGTextInput::color() const
{
    Q_D(const QSGTextInput);
    return d->color;
}

void QSGTextInput::setColor(const QColor &c)
{
    Q_D(QSGTextInput);
    if (c != d->color) {
        d->color = c;
        update();
        emit colorChanged(c);
    }
}


/*!
    \qmlproperty color QtQuick2::TextInput::selectionColor

    The text highlight color, used behind selections.
*/
QColor QSGTextInput::selectionColor() const
{
    Q_D(const QSGTextInput);
    return d->selectionColor;
}

void QSGTextInput::setSelectionColor(const QColor &color)
{
    Q_D(QSGTextInput);
    if (d->selectionColor == color)
        return;

    d->selectionColor = color;
    QPalette p = d->control->palette();
    p.setColor(QPalette::Highlight, d->selectionColor);
    d->control->setPalette(p);
    if (d->control->hasSelectedText())
        update();
    emit selectionColorChanged(color);
}
/*!
    \qmlproperty color QtQuick2::TextInput::selectedTextColor

    The highlighted text color, used in selections.
*/
QColor QSGTextInput::selectedTextColor() const
{
    Q_D(const QSGTextInput);
    return d->selectedTextColor;
}

void QSGTextInput::setSelectedTextColor(const QColor &color)
{
    Q_D(QSGTextInput);
    if (d->selectedTextColor == color)
        return;

    d->selectedTextColor = color;
    QPalette p = d->control->palette();
    p.setColor(QPalette::HighlightedText, d->selectedTextColor);
    d->control->setPalette(p);
    if (d->control->hasSelectedText())
        update();
    emit selectedTextColorChanged(color);
}

/*!
    \qmlproperty enumeration QtQuick2::TextInput::horizontalAlignment
    \qmlproperty enumeration QtQuick2::TextInput::effectiveHorizontalAlignment

    Sets the horizontal alignment of the text within the TextInput item's
    width and height. By default, the text alignment follows the natural alignment
    of the text, for example text that is read from left to right will be aligned to
    the left.

    TextInput does not have vertical alignment, as the natural height is
    exactly the height of the single line of text. If you set the height
    manually to something larger, TextInput will always be top aligned
    vertically. You can use anchors to align it however you want within
    another item.

    The valid values for \c horizontalAlignment are \c TextInput.AlignLeft, \c TextInput.AlignRight and
    \c TextInput.AlignHCenter.

    When using the attached property LayoutMirroring::enabled to mirror application
    layouts, the horizontal alignment of text will also be mirrored. However, the property
    \c horizontalAlignment will remain unchanged. To query the effective horizontal alignment
    of TextInput, use the read-only property \c effectiveHorizontalAlignment.
*/
QSGTextInput::HAlignment QSGTextInput::hAlign() const
{
    Q_D(const QSGTextInput);
    return d->hAlign;
}

void QSGTextInput::setHAlign(HAlignment align)
{
    Q_D(QSGTextInput);
    bool forceAlign = d->hAlignImplicit && d->effectiveLayoutMirror;
    d->hAlignImplicit = false;
    if (d->setHAlign(align, forceAlign) && isComponentComplete()) {
        updateCursorRectangle();
    }
}

void QSGTextInput::resetHAlign()
{
    Q_D(QSGTextInput);
    d->hAlignImplicit = true;
    if (d->determineHorizontalAlignment() && isComponentComplete()) {
        updateCursorRectangle();
    }
}

QSGTextInput::HAlignment QSGTextInput::effectiveHAlign() const
{
    Q_D(const QSGTextInput);
    QSGTextInput::HAlignment effectiveAlignment = d->hAlign;
    if (!d->hAlignImplicit && d->effectiveLayoutMirror) {
        switch (d->hAlign) {
        case QSGTextInput::AlignLeft:
            effectiveAlignment = QSGTextInput::AlignRight;
            break;
        case QSGTextInput::AlignRight:
            effectiveAlignment = QSGTextInput::AlignLeft;
            break;
        default:
            break;
        }
    }
    return effectiveAlignment;
}

bool QSGTextInputPrivate::setHAlign(QSGTextInput::HAlignment alignment, bool forceAlign)
{
    Q_Q(QSGTextInput);
    if ((hAlign != alignment || forceAlign) && alignment <= QSGTextInput::AlignHCenter) { // justify not supported
        QSGTextInput::HAlignment oldEffectiveHAlign = q->effectiveHAlign();
        hAlign = alignment;
        emit q->horizontalAlignmentChanged(alignment);
        if (oldEffectiveHAlign != q->effectiveHAlign())
            emit q->effectiveHorizontalAlignmentChanged();
        return true;
    }
    return false;
}

bool QSGTextInputPrivate::determineHorizontalAlignment()
{
    if (hAlignImplicit) {
        // if no explicit alignment has been set, follow the natural layout direction of the text
        QString text = control->text();
        bool isRightToLeft = text.isEmpty() ? QGuiApplication::keyboardInputDirection() == Qt::RightToLeft : text.isRightToLeft();
        return setHAlign(isRightToLeft ? QSGTextInput::AlignRight : QSGTextInput::AlignLeft);
    }
    return false;
}

void QSGTextInputPrivate::mirrorChange()
{
    Q_Q(QSGTextInput);
    if (q->isComponentComplete()) {
        if (!hAlignImplicit && (hAlign == QSGTextInput::AlignRight || hAlign == QSGTextInput::AlignLeft)) {
            q->updateCursorRectangle();
            emit q->effectiveHorizontalAlignmentChanged();
        }
    }
}

/*!
    \qmlproperty bool QtQuick2::TextInput::readOnly

    Sets whether user input can modify the contents of the TextInput.

    If readOnly is set to true, then user input will not affect the text
    property. Any bindings or attempts to set the text property will still
    work.
*/
bool QSGTextInput::isReadOnly() const
{
    Q_D(const QSGTextInput);
    return d->control->isReadOnly();
}

void QSGTextInput::setReadOnly(bool ro)
{
    Q_D(QSGTextInput);
    if (d->control->isReadOnly() == ro)
        return;

    setFlag(QSGItem::ItemAcceptsInputMethod, !ro);
    d->control->setReadOnly(ro);
    if (!ro)
        d->control->setCursorPosition(d->control->end());

    emit readOnlyChanged(ro);
}

/*!
    \qmlproperty int QtQuick2::TextInput::maximumLength
    The maximum permitted length of the text in the TextInput.

    If the text is too long, it is truncated at the limit.

    By default, this property contains a value of 32767.
*/
int QSGTextInput::maxLength() const
{
    Q_D(const QSGTextInput);
    return d->control->maxLength();
}

void QSGTextInput::setMaxLength(int ml)
{
    Q_D(QSGTextInput);
    if (d->control->maxLength() == ml)
        return;

    d->control->setMaxLength(ml);

    emit maximumLengthChanged(ml);
}

/*!
    \qmlproperty bool QtQuick2::TextInput::cursorVisible
    Set to true when the TextInput shows a cursor.

    This property is set and unset when the TextInput gets active focus, so that other
    properties can be bound to whether the cursor is currently showing. As it
    gets set and unset automatically, when you set the value yourself you must
    keep in mind that your value may be overwritten.

    It can be set directly in script, for example if a KeyProxy might
    forward keys to it and you desire it to look active when this happens
    (but without actually giving it active focus).

    It should not be set directly on the element, like in the below QML,
    as the specified value will be overridden an lost on focus changes.

    \code
    TextInput {
        text: "Text"
        cursorVisible: false
    }
    \endcode

    In the above snippet the cursor will still become visible when the
    TextInput gains active focus.
*/
bool QSGTextInput::isCursorVisible() const
{
    Q_D(const QSGTextInput);
    return d->cursorVisible;
}

void QSGTextInput::setCursorVisible(bool on)
{
    Q_D(QSGTextInput);
    if (d->cursorVisible == on)
        return;
    d->cursorVisible = on;
    d->control->setCursorBlinkPeriod(on ? qApp->styleHints()->cursorFlashTime() : 0);
    QRect r = d->control->cursorRect();
    if (d->control->inputMask().isEmpty())
        updateRect(r);
    else
        updateRect();
    emit cursorVisibleChanged(d->cursorVisible);
}

/*!
    \qmlproperty int QtQuick2::TextInput::cursorPosition
    The position of the cursor in the TextInput.
*/
int QSGTextInput::cursorPosition() const
{
    Q_D(const QSGTextInput);
    return d->control->cursor();
}
void QSGTextInput::setCursorPosition(int cp)
{
    Q_D(QSGTextInput);
    if (cp < 0 || cp > d->control->text().length())
        return;
    d->control->moveCursor(cp);
}

/*!
  Returns a Rect which encompasses the cursor, but which may be larger than is
  required. Ignores custom cursor delegates.
*/
QRect QSGTextInput::cursorRectangle() const
{
    Q_D(const QSGTextInput);
    QRect r = d->control->cursorRect();
    // Scroll and make consistent with TextEdit
    // QLineControl inexplicably adds 1 to the height and horizontal padding
    // for unicode direction markers.
    r.adjust(5 - d->hscroll, 0, -4 - d->hscroll, -1);
    return r;
}
/*!
    \qmlproperty int QtQuick2::TextInput::selectionStart

    The cursor position before the first character in the current selection.

    This property is read-only. To change the selection, use select(start,end),
    selectAll(), or selectWord().

    \sa selectionEnd, cursorPosition, selectedText
*/
int QSGTextInput::selectionStart() const
{
    Q_D(const QSGTextInput);
    return d->lastSelectionStart;
}
/*!
    \qmlproperty int QtQuick2::TextInput::selectionEnd

    The cursor position after the last character in the current selection.

    This property is read-only. To change the selection, use select(start,end),
    selectAll(), or selectWord().

    \sa selectionStart, cursorPosition, selectedText
*/
int QSGTextInput::selectionEnd() const
{
    Q_D(const QSGTextInput);
    return d->lastSelectionEnd;
}
/*!
    \qmlmethod void QtQuick2::TextInput::select(int start, int end)

    Causes the text from \a start to \a end to be selected.

    If either start or end is out of range, the selection is not changed.

    After calling this, selectionStart will become the lesser
    and selectionEnd will become the greater (regardless of the order passed
    to this method).

    \sa selectionStart, selectionEnd
*/
void QSGTextInput::select(int start, int end)
{
    Q_D(QSGTextInput);
    if (start < 0 || end < 0 || start > d->control->text().length() || end > d->control->text().length())
        return;
    d->control->setSelection(start, end-start);
}

/*!
    \qmlproperty string QtQuick2::TextInput::selectedText

    This read-only property provides the text currently selected in the
    text input.

    It is equivalent to the following snippet, but is faster and easier
    to use.

    \js
    myTextInput.text.toString().substring(myTextInput.selectionStart,
        myTextInput.selectionEnd);
    \endjs
*/
QString QSGTextInput::selectedText() const
{
    Q_D(const QSGTextInput);
    return d->control->selectedText();
}

/*!
    \qmlproperty bool QtQuick2::TextInput::activeFocusOnPress

    Whether the TextInput should gain active focus on a mouse press. By default this is
    set to true.
*/
bool QSGTextInput::focusOnPress() const
{
    Q_D(const QSGTextInput);
    return d->focusOnPress;
}

void QSGTextInput::setFocusOnPress(bool b)
{
    Q_D(QSGTextInput);
    if (d->focusOnPress == b)
        return;

    d->focusOnPress = b;

    emit activeFocusOnPressChanged(d->focusOnPress);
}
/*!
    \qmlproperty bool QtQuick2::TextInput::autoScroll

    Whether the TextInput should scroll when the text is longer than the width. By default this is
    set to true.
*/
bool QSGTextInput::autoScroll() const
{
    Q_D(const QSGTextInput);
    return d->autoScroll;
}

void QSGTextInput::setAutoScroll(bool b)
{
    Q_D(QSGTextInput);
    if (d->autoScroll == b)
        return;

    d->autoScroll = b;
    //We need to repaint so that the scrolling is taking into account.
    updateSize(true);
    updateCursorRectangle();
    emit autoScrollChanged(d->autoScroll);
}

#ifndef QT_NO_VALIDATOR

/*!
    \qmlclass IntValidator QIntValidator
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-visual-elements

    This element provides a validator for integer values.

    IntValidator uses the \l {QLocale::setDefault()}{default locale} to interpret the number and
    will accept locale specific digits, group separators, and positive and negative signs.  In
    addition, IntValidator is always guaranteed to accept a number formatted according to the "C"
    locale.
*/
/*!
    \qmlproperty int QtQuick2::IntValidator::top

    This property holds the validator's highest acceptable value.
    By default, this property's value is derived from the highest signed integer available (typically 2147483647).
*/
/*!
    \qmlproperty int QtQuick2::IntValidator::bottom

    This property holds the validator's lowest acceptable value.
    By default, this property's value is derived from the lowest signed integer available (typically -2147483647).
*/

/*!
    \qmlclass DoubleValidator QDoubleValidator
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-visual-elements

    This element provides a validator for non-integer numbers.
*/

/*!
    \qmlproperty real QtQuick2::DoubleValidator::top

    This property holds the validator's maximum acceptable value.
    By default, this property contains a value of infinity.
*/
/*!
    \qmlproperty real QtQuick2::DoubleValidator::bottom

    This property holds the validator's minimum acceptable value.
    By default, this property contains a value of -infinity.
*/
/*!
    \qmlproperty int QtQuick2::DoubleValidator::decimals

    This property holds the validator's maximum number of digits after the decimal point.
    By default, this property contains a value of 1000.
*/
/*!
    \qmlproperty enumeration QtQuick2::DoubleValidator::notation
    This property holds the notation of how a string can describe a number.

    The possible values for this property are:

    \list
    \o DoubleValidator.StandardNotation
    \o DoubleValidator.ScientificNotation (default)
    \endlist

    If this property is set to DoubleValidator.ScientificNotation, the written number may have an exponent part (e.g. 1.5E-2).
*/

/*!
    \qmlclass RegExpValidator QRegExpValidator
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-visual-elements

    This element provides a validator, which counts as valid any string which
    matches a specified regular expression.
*/
/*!
   \qmlproperty regExp QtQuick2::RegExpValidator::regExp

   This property holds the regular expression used for validation.

   Note that this property should be a regular expression in JS syntax, e.g /a/ for the regular expression
   matching "a".

   By default, this property contains a regular expression with the pattern .* that matches any string.
*/

/*!
    \qmlproperty Validator QtQuick2::TextInput::validator

    Allows you to set a validator on the TextInput. When a validator is set
    the TextInput will only accept input which leaves the text property in
    an acceptable or intermediate state. The accepted signal will only be sent
    if the text is in an acceptable state when enter is pressed.

    Currently supported validators are IntValidator, DoubleValidator and
    RegExpValidator. An example of using validators is shown below, which allows
    input of integers between 11 and 31 into the text input:

    \code
    import QtQuick 1.0
    TextInput{
        validator: IntValidator{bottom: 11; top: 31;}
        focus: true
    }
    \endcode

    \sa acceptableInput, inputMask
*/

QValidator* QSGTextInput::validator() const
{
    Q_D(const QSGTextInput);
    //###const cast isn't good, but needed for property system?
    return const_cast<QValidator*>(d->control->validator());
}

void QSGTextInput::setValidator(QValidator* v)
{
    Q_D(QSGTextInput);
    if (d->control->validator() == v)
        return;

    d->control->setValidator(v);
    if(!d->control->hasAcceptableInput()){
        d->oldValidity = false;
        emit acceptableInputChanged();
    }

    emit validatorChanged();
}
#endif // QT_NO_VALIDATOR

/*!
    \qmlproperty string QtQuick2::TextInput::inputMask

    Allows you to set an input mask on the TextInput, restricting the allowable
    text inputs. See QLineEdit::inputMask for further details, as the exact
    same mask strings are used by TextInput.

    \sa acceptableInput, validator
*/
QString QSGTextInput::inputMask() const
{
    Q_D(const QSGTextInput);
    return d->control->inputMask();
}

void QSGTextInput::setInputMask(const QString &im)
{
    Q_D(QSGTextInput);
    if (d->control->inputMask() == im)
        return;

    d->control->setInputMask(im);
    emit inputMaskChanged(d->control->inputMask());
}

/*!
    \qmlproperty bool QtQuick2::TextInput::acceptableInput

    This property is always true unless a validator or input mask has been set.
    If a validator or input mask has been set, this property will only be true
    if the current text is acceptable to the validator or input mask as a final
    string (not as an intermediate string).
*/
bool QSGTextInput::hasAcceptableInput() const
{
    Q_D(const QSGTextInput);
    return d->control->hasAcceptableInput();
}

/*!
    \qmlsignal QtQuick2::TextInput::onAccepted()

    This handler is called when the Return or Enter key is pressed.
    Note that if there is a \l validator or \l inputMask set on the text
    input, the handler will only be emitted if the input is in an acceptable
    state.
*/

void QSGTextInputPrivate::updateInputMethodHints()
{
    Q_Q(QSGTextInput);
    Qt::InputMethodHints hints = inputMethodHints;
    uint echo = control->echoMode();
    if (echo == QSGTextInput::Password || echo == QSGTextInput::NoEcho)
        hints |= Qt::ImhHiddenText;
    else if (echo == QSGTextInput::PasswordEchoOnEdit)
        hints &= ~Qt::ImhHiddenText;
    if (echo != QSGTextInput::Normal)
        hints |= (Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    q->setInputMethodHints(hints);
}
/*!
    \qmlproperty enumeration QtQuick2::TextInput::echoMode

    Specifies how the text should be displayed in the TextInput.
    \list
    \o TextInput.Normal - Displays the text as it is. (Default)
    \o TextInput.Password - Displays asterixes instead of characters.
    \o TextInput.NoEcho - Displays nothing.
    \o TextInput.PasswordEchoOnEdit - Displays characters as they are entered
    while editing, otherwise displays asterisks.
    \endlist
*/
QSGTextInput::EchoMode QSGTextInput::echoMode() const
{
    Q_D(const QSGTextInput);
    return (QSGTextInput::EchoMode)d->control->echoMode();
}

void QSGTextInput::setEchoMode(QSGTextInput::EchoMode echo)
{
    Q_D(QSGTextInput);
    if (echoMode() == echo)
        return;
    d->control->setEchoMode((QLineControl::EchoMode)echo);
    d->updateInputMethodHints();
    q_textChanged();
    emit echoModeChanged(echoMode());
}

Qt::InputMethodHints QSGTextInput::imHints() const
{
    Q_D(const QSGTextInput);
    return d->inputMethodHints;
}

void QSGTextInput::setIMHints(Qt::InputMethodHints hints)
{
    Q_D(QSGTextInput);
    if (d->inputMethodHints == hints)
        return;
    d->inputMethodHints = hints;
    d->updateInputMethodHints();
}

/*!
    \qmlproperty Component QtQuick2::TextInput::cursorDelegate
    The delegate for the cursor in the TextInput.

    If you set a cursorDelegate for a TextInput, this delegate will be used for
    drawing the cursor instead of the standard cursor. An instance of the
    delegate will be created and managed by the TextInput when a cursor is
    needed, and the x property of delegate instance will be set so as
    to be one pixel before the top left of the current character.

    Note that the root item of the delegate component must be a QDeclarativeItem or
    QDeclarativeItem derived item.
*/
QDeclarativeComponent* QSGTextInput::cursorDelegate() const
{
    Q_D(const QSGTextInput);
    return d->cursorComponent;
}

void QSGTextInput::setCursorDelegate(QDeclarativeComponent* c)
{
    Q_D(QSGTextInput);
    if (d->cursorComponent == c)
        return;

    d->cursorComponent = c;
    if(!c){
        //note that the components are owned by something else
        delete d->cursorItem;
    }else{
        d->startCreatingCursor();
    }

    emit cursorDelegateChanged();
}

void QSGTextInputPrivate::startCreatingCursor()
{
    Q_Q(QSGTextInput);
    if(cursorComponent->isReady()){
        q->createCursor();
    }else if(cursorComponent->isLoading()){
        q->connect(cursorComponent, SIGNAL(statusChanged(int)),
                q, SLOT(createCursor()));
    }else {//isError
        qmlInfo(q, cursorComponent->errors()) << QSGTextInput::tr("Could not load cursor delegate");
    }
}

void QSGTextInput::createCursor()
{
    Q_D(QSGTextInput);
    if(d->cursorComponent->isError()){
        qmlInfo(this, d->cursorComponent->errors()) << tr("Could not load cursor delegate");
        return;
    }

    if(!d->cursorComponent->isReady())
        return;

    if(d->cursorItem)
        delete d->cursorItem;
    d->cursorItem = qobject_cast<QSGItem*>(d->cursorComponent->create());
    if(!d->cursorItem){
        qmlInfo(this, d->cursorComponent->errors()) << tr("Could not instantiate cursor delegate");
        return;
    }

    QDeclarative_setParent_noEvent(d->cursorItem, this);
    d->cursorItem->setParentItem(this);
    d->cursorItem->setX(d->control->cursorToX());
    d->cursorItem->setHeight(d->control->height()-1); // -1 to counter QLineControl's +1 which is not consistent with Text.
}

/*!
    \qmlmethod rect QtQuick2::TextInput::positionToRectangle(int pos)

    This function takes a character position and returns the rectangle that the
    cursor would occupy, if it was placed at that character position.

    This is similar to setting the cursorPosition, and then querying the cursor
    rectangle, but the cursorPosition is not changed.
*/
QRectF QSGTextInput::positionToRectangle(int pos) const
{
    Q_D(const QSGTextInput);
    if (pos > d->control->cursorPosition())
        pos += d->control->preeditAreaText().length();
    return QRectF(d->control->cursorToX(pos)-d->hscroll,
        0.0,
        d->control->cursorWidth(),
        cursorRectangle().height());
}

/*!
    \qmlmethod int QtQuick2::TextInput::positionAt(int x, CursorPosition position = CursorBetweenCharacters)

    This function returns the character position at
    x pixels from the left of the textInput. Position 0 is before the
    first character, position 1 is after the first character but before the second,
    and so on until position text.length, which is after all characters.

    This means that for all x values before the first character this function returns 0,
    and for all x values after the last character this function returns text.length.

    The cursor position type specifies how the cursor position should be resolved.

    \list
    \o TextInput.CursorBetweenCharacters - Returns the position between characters that is nearest x.
    \o TextInput.CursorOnCharacter - Returns the position before the character that is nearest x.
    \endlist
*/
int QSGTextInput::positionAt(int x) const
{
    return positionAt(x, CursorBetweenCharacters);
}

int QSGTextInput::positionAt(int x, CursorPosition position) const
{
    Q_D(const QSGTextInput);
    int pos = d->control->xToPos(x + d->hscroll, QTextLine::CursorPosition(position));
    const int cursor = d->control->cursor();
    if (pos > cursor) {
        const int preeditLength = d->control->preeditAreaText().length();
        pos = pos > cursor + preeditLength
                ? pos - preeditLength
                : cursor;
    }
    return pos;
}

void QSGTextInput::keyPressEvent(QKeyEvent* ev)
{
    Q_D(QSGTextInput);
    // Don't allow MacOSX up/down support, and we don't allow a completer.
    bool ignore = (ev->key() == Qt::Key_Up || ev->key() == Qt::Key_Down) && ev->modifiers() == Qt::NoModifier;
    if (!ignore && (d->lastSelectionStart == d->lastSelectionEnd) && (ev->key() == Qt::Key_Right || ev->key() == Qt::Key_Left)) {
        // Ignore when moving off the end unless there is a selection,
        // because then moving will do something (deselect).
        int cursorPosition = d->control->cursor();
        if (cursorPosition == 0)
            ignore = ev->key() == (d->control->layoutDirection() == Qt::LeftToRight ? Qt::Key_Left : Qt::Key_Right);
        if (cursorPosition == d->control->text().length())
            ignore = ev->key() == (d->control->layoutDirection() == Qt::LeftToRight ? Qt::Key_Right : Qt::Key_Left);
    }
    if (ignore) {
        ev->ignore();
    } else {
        d->control->processKeyEvent(ev);
    }
    if (!ev->isAccepted())
        QSGImplicitSizeItem::keyPressEvent(ev);
}

void QSGTextInput::inputMethodEvent(QInputMethodEvent *ev)
{
    Q_D(QSGTextInput);
    const bool wasComposing = d->control->preeditAreaText().length() > 0;
    if (d->control->isReadOnly()) {
        ev->ignore();
    } else {
        d->control->processInputMethodEvent(ev);
    }
    if (!ev->isAccepted())
        QSGImplicitSizeItem::inputMethodEvent(ev);

    if (wasComposing != (d->control->preeditAreaText().length() > 0))
        emit inputMethodComposingChanged();
}

void QSGTextInput::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QSGTextInput);
    if (d->sendMouseEventToInputContext(event))
        return;
    if (d->selectByMouse) {
        int cursor = d->xToPos(event->localPos().x());
        d->control->selectWordAtPos(cursor);
        event->setAccepted(true);
    } else {
        QSGImplicitSizeItem::mouseDoubleClickEvent(event);
    }
}

void QSGTextInput::mousePressEvent(QMouseEvent *event)
{
    Q_D(QSGTextInput);
    if (d->sendMouseEventToInputContext(event))
        return;
    if(d->focusOnPress){
        bool hadActiveFocus = hasActiveFocus();
        forceActiveFocus();
        if (d->showInputPanelOnFocus) {
            if (hasActiveFocus() && hadActiveFocus && !isReadOnly()) {
                // re-open input panel on press if already focused
                openSoftwareInputPanel();
            }
        } else { // show input panel on click
            if (hasActiveFocus() && !hadActiveFocus) {
                d->clickCausedFocus = true;
            }
        }
    }
    if (d->selectByMouse) {
        setKeepMouseGrab(false);
        d->selectPressed = true;
        d->pressPos = event->localPos();
    }
    bool mark = (event->modifiers() & Qt::ShiftModifier) && d->selectByMouse;
    int cursor = d->xToPos(event->localPos().x());
    d->control->moveCursor(cursor, mark);
    event->setAccepted(true);
}

void QSGTextInput::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QSGTextInput);
    if (d->sendMouseEventToInputContext(event))
        return;
    if (d->selectPressed) {
        if (qAbs(int(event->localPos().x() - d->pressPos.x())) > qApp->styleHints()->startDragDistance())
            setKeepMouseGrab(true);
        moveCursorSelection(d->xToPos(event->localPos().x()), d->mouseSelectionMode);
        event->setAccepted(true);
    } else {
        QSGImplicitSizeItem::mouseMoveEvent(event);
    }
}

void QSGTextInput::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QSGTextInput);
    if (d->sendMouseEventToInputContext(event))
        return;
    if (d->selectPressed) {
        d->selectPressed = false;
        setKeepMouseGrab(false);
    }
    if (!d->showInputPanelOnFocus) { // input panel on click
        if (d->focusOnPress && !isReadOnly() && boundingRect().contains(event->localPos())) {
            if (canvas() && canvas() == QGuiApplication::activeWindow()) {
                // ### refactor: implement virtual keyboard properly..
                qDebug("QSGTextInput: virtual keyboard no implemented...");
//                qt_widget_private(canvas())->handleSoftwareInputPanel(event->button(), d->clickCausedFocus);
            }
        }
    }
    d->clickCausedFocus = false;
    d->control->processEvent(event);
    if (!event->isAccepted())
        QSGImplicitSizeItem::mouseReleaseEvent(event);
}

bool QSGTextInputPrivate::sendMouseEventToInputContext(QMouseEvent *event)
{
#if !defined QT_NO_IM
    if (control->composeMode() && event->type() == QEvent::KeyRelease) {
        int tmp_cursor = xToPos(event->localPos().x());
        int mousePos = tmp_cursor - control->cursor();
        if (mousePos < 0 || mousePos > control->preeditAreaText().length()) {
            mousePos = -1;
            // don't send move events outside the preedit area
            if (event->type() == QEvent::MouseMove)
                return true;
        }

        // may be causing reset() in some input methods
        qApp->inputPanel()->invokeAction(QInputPanel::Click, mousePos);
        if (!control->preeditAreaText().isEmpty())
            return true;
    }
#else
    Q_UNUSED(event);
    Q_UNUSED(eventType)
#endif

    return false;
}

void QSGTextInput::mouseUngrabEvent()
{
    Q_D(QSGTextInput);
    d->selectPressed = false;
    setKeepMouseGrab(false);
}

bool QSGTextInput::event(QEvent* ev)
{
    Q_D(QSGTextInput);
    //Anything we don't deal with ourselves, pass to the control
    bool handled = false;
    switch(ev->type()){
        case QEvent::KeyPress:
        case QEvent::KeyRelease://###Should the control be doing anything with release?
        case QEvent::InputMethod:
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
            break;
        default:
            handled = d->control->processEvent(ev);
    }
    if(!handled)
        handled = QSGImplicitSizeItem::event(ev);
    return handled;
}

void QSGTextInput::geometryChanged(const QRectF &newGeometry,
                                  const QRectF &oldGeometry)
{
    if (newGeometry.width() != oldGeometry.width()) {
        updateSize();
        updateCursorRectangle();
    }
    QSGImplicitSizeItem::geometryChanged(newGeometry, oldGeometry);
}

int QSGTextInputPrivate::calculateTextWidth()
{
    return qRound(control->naturalTextWidth());
}

void QSGTextInputPrivate::updateHorizontalScroll()
{
    Q_Q(QSGTextInput);
    const int preeditLength = control->preeditAreaText().length();
    const int width = q->width();
    int widthUsed = calculateTextWidth();

    if (!autoScroll || widthUsed <=  width) {
        QSGTextInput::HAlignment effectiveHAlign = q->effectiveHAlign();
        // text fits in br; use hscroll for alignment
        switch (effectiveHAlign & ~(Qt::AlignAbsolute|Qt::AlignVertical_Mask)) {
        case Qt::AlignRight:
            hscroll = widthUsed - width;
            break;
        case Qt::AlignHCenter:
            hscroll = (widthUsed - width) / 2;
            break;
        default:
            // Left
            hscroll = 0;
            break;
        }
    } else {
        int cix = qRound(control->cursorToX(control->cursor() + preeditLength));
        if (cix - hscroll >= width) {
            // text doesn't fit, cursor is to the right of br (scroll right)
            hscroll = cix - width;
        } else if (cix - hscroll < 0 && hscroll < widthUsed) {
            // text doesn't fit, cursor is to the left of br (scroll left)
            hscroll = cix;
        } else if (widthUsed - hscroll < width) {
            // text doesn't fit, text document is to the left of br; align
            // right
            hscroll = widthUsed - width;
        }
        if (preeditLength > 0) {
            // check to ensure long pre-edit text doesn't push the cursor
            // off to the left
             cix = qRound(control->cursorToX(
                     control->cursor() + qMax(0, control->preeditCursor() - 1)));
             if (cix < hscroll)
                 hscroll = cix;
        }
    }
}

QSGNode *QSGTextInput::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    Q_D(QSGTextInput);

    QSGTextNode *node = static_cast<QSGTextNode *>(oldNode);
    if (node == 0)
        node = new QSGTextNode(QSGItemPrivate::get(this)->sceneGraphContext());
    d->textNode = node;

    if (!d->textLayoutDirty) {
        QSGSimpleRectNode *cursorNode = node->cursorNode();
        if (cursorNode != 0 && !isReadOnly()) {
            QFontMetrics fm = QFontMetrics(d->font);
            // the y offset is there to keep the baseline constant in case we have script changes in the text.
            QPoint offset(-d->hscroll, fm.ascent() - d->control->ascent());
            offset.rx() += d->control->cursorToX();

            QRect br(boundingRect().toRect());
            cursorNode->setRect(QRectF(offset, QSizeF(d->control->cursorWidth(), br.height())));

            if (!d->cursorVisible
                    || (!d->control->cursorBlinkStatus() && d->control->cursorBlinkPeriod() > 0)) {
                d->hideCursor();
            } else {
                d->showCursor();
            }
        }
    } else {
        node->deleteContent();
        node->setMatrix(QMatrix4x4());

        QPoint offset = QPoint(0,0);
        QFontMetrics fm = QFontMetrics(d->font);
        QRect br(boundingRect().toRect());
        if (d->autoScroll) {
            // the y offset is there to keep the baseline constant in case we have script changes in the text.
            offset = br.topLeft() - QPoint(d->hscroll, d->control->ascent() - fm.ascent());
        } else {
            offset = QPoint(d->hscroll, 0);
        }

        QTextLayout *textLayout = d->control->textLayout();
        if (!textLayout->text().isEmpty()) {
            node->addTextLayout(offset, textLayout, d->color,
                                QSGText::Normal, QColor(),
                                d->selectionColor, d->selectedTextColor,
                                d->control->selectionStart(),
                                d->control->selectionEnd() - 1); // selectionEnd() returns first char after
                                                                 // selection
        }

        if (!isReadOnly() && d->cursorItem == 0) {
            offset.rx() += d->control->cursorToX();
            node->setCursor(QRectF(offset, QSizeF(d->control->cursorWidth(), br.height())), d->color);
            if (!d->cursorVisible
                    || (!d->control->cursorBlinkStatus() && d->control->cursorBlinkPeriod() > 0)) {
                d->hideCursor();
            } else {
                d->showCursor();
            }
        }

        d->textLayoutDirty = false;
    }

    return node;
}

QVariant QSGTextInput::inputMethodQuery(Qt::InputMethodQuery property) const
{
    Q_D(const QSGTextInput);
    switch(property) {
    case Qt::ImEnabled:
        return QVariant((bool)(flags() & ItemAcceptsInputMethod));
    case Qt::ImHints:
        return QVariant((int)inputMethodHints());
    case Qt::ImMicroFocus:
        return cursorRectangle();
    case Qt::ImFont:
        return font();
    case Qt::ImCursorPosition:
        return QVariant(d->control->cursor());
    case Qt::ImSurroundingText:
        if (d->control->echoMode() == QLineControl::PasswordEchoOnEdit && !d->control->passwordEchoEditing())
            return QVariant(displayText());
        else
            return QVariant(text());
    case Qt::ImCurrentSelection:
        return QVariant(selectedText());
    case Qt::ImMaximumTextLength:
        return QVariant(maxLength());
    case Qt::ImAnchorPosition:
        if (d->control->selectionStart() == d->control->selectionEnd())
            return QVariant(d->control->cursor());
        else if (d->control->selectionStart() == d->control->cursor())
            return QVariant(d->control->selectionEnd());
        else
            return QVariant(d->control->selectionStart());
    default:
        return QVariant();
    }
}

/*!
    \qmlmethod void QtQuick2::TextInput::deselect()

    Removes active text selection.
*/
void QSGTextInput::deselect()
{
    Q_D(QSGTextInput);
    d->control->deselect();
}

/*!
    \qmlmethod void QtQuick2::TextInput::selectAll()

    Causes all text to be selected.
*/
void QSGTextInput::selectAll()
{
    Q_D(QSGTextInput);
    d->control->setSelection(0, d->control->text().length());
}

/*!
    \qmlmethod void QtQuick2::TextInput::isRightToLeft(int start, int end)

    Returns true if the natural reading direction of the editor text
    found between positions \a start and \a end is right to left.
*/
bool QSGTextInput::isRightToLeft(int start, int end)
{
    Q_D(QSGTextInput);
    if (start > end) {
        qmlInfo(this) << "isRightToLeft(start, end) called with the end property being smaller than the start.";
        return false;
    } else {
        return d->control->text().mid(start, end - start).isRightToLeft();
    }
}

#ifndef QT_NO_CLIPBOARD
/*!
    \qmlmethod QtQuick2::TextInput::cut()

    Moves the currently selected text to the system clipboard.
*/
void QSGTextInput::cut()
{
    Q_D(QSGTextInput);
    d->control->copy();
    d->control->del();
}

/*!
    \qmlmethod QtQuick2::TextInput::copy()

    Copies the currently selected text to the system clipboard.
*/
void QSGTextInput::copy()
{
    Q_D(QSGTextInput);
    d->control->copy();
}

/*!
    \qmlmethod QtQuick2::TextInput::paste()

    Replaces the currently selected text by the contents of the system clipboard.
*/
void QSGTextInput::paste()
{
    Q_D(QSGTextInput);
    if (!d->control->isReadOnly())
        d->control->paste();
}
#endif // QT_NO_CLIPBOARD

/*!
    \qmlmethod void QtQuick2::TextInput::selectWord()

    Causes the word closest to the current cursor position to be selected.
*/
void QSGTextInput::selectWord()
{
    Q_D(QSGTextInput);
    d->control->selectWordAtPos(d->control->cursor());
}

/*!
    \qmlproperty bool QtQuick2::TextInput::smooth

    This property holds whether the text is smoothly scaled or transformed.

    Smooth filtering gives better visual quality, but is slower.  If
    the item is displayed at its natural size, this property has no visual or
    performance effect.

    \note Generally scaling artifacts are only visible if the item is stationary on
    the screen.  A common pattern when animating an item is to disable smooth
    filtering at the beginning of the animation and reenable it at the conclusion.
*/

/*!
   \qmlproperty string QtQuick2::TextInput::passwordCharacter

   This is the character displayed when echoMode is set to Password or
   PasswordEchoOnEdit. By default it is an asterisk.

   If this property is set to a string with more than one character,
   the first character is used. If the string is empty, the value
   is ignored and the property is not set.
*/
QString QSGTextInput::passwordCharacter() const
{
    Q_D(const QSGTextInput);
    return QString(d->control->passwordCharacter());
}

void QSGTextInput::setPasswordCharacter(const QString &str)
{
    Q_D(QSGTextInput);
    if(str.length() < 1)
        return;
    d->control->setPasswordCharacter(str.constData()[0]);
    EchoMode echoMode_ = echoMode();
    if (echoMode_ == Password || echoMode_ == PasswordEchoOnEdit) {
        updateSize();
    }
    emit passwordCharacterChanged();
}

/*!
   \qmlproperty string QtQuick2::TextInput::displayText

   This is the text displayed in the TextInput.

   If \l echoMode is set to TextInput::Normal, this holds the
   same value as the TextInput::text property. Otherwise,
   this property holds the text visible to the user, while
   the \l text property holds the actual entered text.
*/
QString QSGTextInput::displayText() const
{
    Q_D(const QSGTextInput);
    return d->control->displayText();
}

/*!
    \qmlproperty bool QtQuick2::TextInput::selectByMouse

    Defaults to false.

    If true, the user can use the mouse to select text in some
    platform-specific way. Note that for some platforms this may
    not be an appropriate interaction (eg. may conflict with how
    the text needs to behave inside a Flickable.
*/
bool QSGTextInput::selectByMouse() const
{
    Q_D(const QSGTextInput);
    return d->selectByMouse;
}

void QSGTextInput::setSelectByMouse(bool on)
{
    Q_D(QSGTextInput);
    if (d->selectByMouse != on) {
        d->selectByMouse = on;
        emit selectByMouseChanged(on);
    }
}

/*!
    \qmlproperty enum QtQuick2::TextInput::mouseSelectionMode

    Specifies how text should be selected using a mouse.

    \list
    \o TextInput.SelectCharacters - The selection is updated with individual characters. (Default)
    \o TextInput.SelectWords - The selection is updated with whole words.
    \endlist

    This property only applies when \l selectByMouse is true.
*/

QSGTextInput::SelectionMode QSGTextInput::mouseSelectionMode() const
{
    Q_D(const QSGTextInput);
    return d->mouseSelectionMode;
}

void QSGTextInput::setMouseSelectionMode(SelectionMode mode)
{
    Q_D(QSGTextInput);
    if (d->mouseSelectionMode != mode) {
        d->mouseSelectionMode = mode;
        emit mouseSelectionModeChanged(mode);
    }
}

/*!
    \qmlproperty bool QtQuick2::TextInput::canPaste

    Returns true if the TextInput is writable and the content of the clipboard is
    suitable for pasting into the TextEdit.
*/
bool QSGTextInput::canPaste() const
{
    Q_D(const QSGTextInput);
    return d->canPaste;
}

void QSGTextInput::moveCursorSelection(int position)
{
    Q_D(QSGTextInput);
    d->control->moveCursor(position, true);
}

/*!
    \qmlmethod void QtQuick2::TextInput::moveCursorSelection(int position, SelectionMode mode = TextInput.SelectCharacters)

    Moves the cursor to \a position and updates the selection according to the optional \a mode
    parameter.  (To only move the cursor, set the \l cursorPosition property.)

    When this method is called it additionally sets either the
    selectionStart or the selectionEnd (whichever was at the previous cursor position)
    to the specified position. This allows you to easily extend and contract the selected
    text range.

    The selection mode specifies whether the selection is updated on a per character or a per word
    basis.  If not specified the selection mode will default to TextInput.SelectCharacters.

    \list
    \o TextEdit.SelectCharacters - Sets either the selectionStart or selectionEnd (whichever was at
    the previous cursor position) to the specified position.
    \o TextEdit.SelectWords - Sets the selectionStart and selectionEnd to include all
    words between the specified postion and the previous cursor position.  Words partially in the
    range are included.
    \endlist

    For example, take this sequence of calls:

    \code
        cursorPosition = 5
        moveCursorSelection(9, TextInput.SelectCharacters)
        moveCursorSelection(7, TextInput.SelectCharacters)
    \endcode

    This moves the cursor to position 5, extend the selection end from 5 to 9
    and then retract the selection end from 9 to 7, leaving the text from position 5 to 7
    selected (the 6th and 7th characters).

    The same sequence with TextInput.SelectWords will extend the selection start to a word boundary
    before or on position 5 and extend the selection end to a word boundary on or past position 9.
*/
void QSGTextInput::moveCursorSelection(int pos, SelectionMode mode)
{
    Q_D(QSGTextInput);

    if (mode == SelectCharacters) {
        d->control->moveCursor(pos, true);
    } else if (pos != d->control->cursor()){
        const int cursor = d->control->cursor();
        int anchor;
        if (!d->control->hasSelectedText())
            anchor = d->control->cursor();
        else if (d->control->selectionStart() == d->control->cursor())
            anchor = d->control->selectionEnd();
        else
            anchor = d->control->selectionStart();

        if (anchor < pos || (anchor == pos && cursor < pos)) {
            const QString text = d->control->text();
            QTextBoundaryFinder finder(QTextBoundaryFinder::Word, text);
            finder.setPosition(anchor);

            const QTextBoundaryFinder::BoundaryReasons reasons = finder.boundaryReasons();
            if (anchor < text.length() && (!(reasons & QTextBoundaryFinder::StartWord)
                    || ((reasons & QTextBoundaryFinder::EndWord) && anchor > cursor))) {
                finder.toPreviousBoundary();
            }
            anchor = finder.position() != -1 ? finder.position() : 0;

            finder.setPosition(pos);
            if (pos > 0 && !finder.boundaryReasons())
                finder.toNextBoundary();
            const int cursor = finder.position() != -1 ? finder.position() : text.length();

            d->control->setSelection(anchor, cursor - anchor);
        } else if (anchor > pos || (anchor == pos && cursor > pos)) {
            const QString text = d->control->text();
            QTextBoundaryFinder finder(QTextBoundaryFinder::Word, text);
            finder.setPosition(anchor);

            const QTextBoundaryFinder::BoundaryReasons reasons = finder.boundaryReasons();
            if (anchor > 0 && (!(reasons & QTextBoundaryFinder::EndWord)
                    || ((reasons & QTextBoundaryFinder::StartWord) && anchor < cursor))) {
                finder.toNextBoundary();
            }

            anchor = finder.position() != -1 ? finder.position() : text.length();

            finder.setPosition(pos);
            if (pos < text.length() && !finder.boundaryReasons())
                 finder.toPreviousBoundary();
            const int cursor = finder.position() != -1 ? finder.position() : 0;

            d->control->setSelection(anchor, cursor - anchor);
        }
    }
}

/*!
    \qmlmethod void QtQuick2::TextInput::openSoftwareInputPanel()

    Opens software input panels like virtual keyboards for typing, useful for
    customizing when you want the input keyboard to be shown and hidden in
    your application.

    By default the opening of input panels follows the platform style. On Symbian^1 and
    Symbian^3 -based devices the panels are opened by clicking TextInput. On other platforms
    the panels are automatically opened when TextInput element gains active focus. Input panels are
    always closed if no editor has active focus.

  . You can disable the automatic behavior by setting the property \c activeFocusOnPress to false
    and use functions openSoftwareInputPanel() and closeSoftwareInputPanel() to implement
    the behavior you want.

    Only relevant on platforms, which provide virtual keyboards.

    \qml
        import QtQuick 1.0
        TextInput {
            id: textInput
            text: "Hello world!"
            activeFocusOnPress: false
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (!textInput.activeFocus) {
                        textInput.forceActiveFocus()
                        textInput.openSoftwareInputPanel();
                    } else {
                        textInput.focus = false;
                    }
                }
                onPressAndHold: textInput.closeSoftwareInputPanel();
            }
        }
    \endqml
*/
void QSGTextInput::openSoftwareInputPanel()
{
    if (qApp) {
        if (canvas()) {
            QEvent event(QEvent::RequestSoftwareInputPanel);
            QCoreApplication::sendEvent(canvas(), &event);
        }
    }
}

/*!
    \qmlmethod void QtQuick2::TextInput::closeSoftwareInputPanel()

    Closes a software input panel like a virtual keyboard shown on the screen, useful
    for customizing when you want the input keyboard to be shown and hidden in
    your application.

    By default the opening of input panels follows the platform style. On Symbian^1 and
    Symbian^3 -based devices the panels are opened by clicking TextInput. On other platforms
    the panels are automatically opened when TextInput element gains active focus. Input panels are
    always closed if no editor has active focus.

  . You can disable the automatic behavior by setting the property \c activeFocusOnPress to false
    and use functions openSoftwareInputPanel() and closeSoftwareInputPanel() to implement
    the behavior you want.

    Only relevant on platforms, which provide virtual keyboards.

    \qml
        import QtQuick 1.0
        TextInput {
            id: textInput
            text: "Hello world!"
            activeFocusOnPress: false
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (!textInput.activeFocus) {
                        textInput.forceActiveFocus();
                        textInput.openSoftwareInputPanel();
                    } else {
                        textInput.focus = false;
                    }
                }
                onPressAndHold: textInput.closeSoftwareInputPanel();
            }
        }
    \endqml
*/
void QSGTextInput::closeSoftwareInputPanel()
{
    if (qApp) {
        if (canvas()) {
            QEvent event(QEvent::CloseSoftwareInputPanel);
            QCoreApplication::sendEvent(canvas(), &event);
        }
    }
}

void QSGTextInput::focusInEvent(QFocusEvent *event)
{
    Q_D(const QSGTextInput);
    if (d->showInputPanelOnFocus) {
        if (d->focusOnPress && !isReadOnly()) {
            openSoftwareInputPanel();
        }
    }
    QSGImplicitSizeItem::focusInEvent(event);
}

void QSGTextInput::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QSGTextInput);
    if (change == ItemActiveFocusHasChanged) {
        bool hasFocus = value.boolValue;
        d->focused = hasFocus;
        setCursorVisible(hasFocus); // ### refactor:  && d->canvas && d->canvas->hasFocus()
        if(echoMode() == QSGTextInput::PasswordEchoOnEdit && !hasFocus)
            d->control->updatePasswordEchoEditing(false);//QLineControl sets it on key events, but doesn't deal with focus events
        if (!hasFocus)
            d->control->deselect();
    }
    QSGItem::itemChange(change, value);
}

/*!
    \qmlproperty bool QtQuick2::TextInput::inputMethodComposing


    This property holds whether the TextInput has partial text input from an
    input method.

    While it is composing an input method may rely on mouse or key events from
    the TextInput to edit or commit the partial text.  This property can be
    used to determine when to disable events handlers that may interfere with
    the correct operation of an input method.
*/
bool QSGTextInput::isInputMethodComposing() const
{
    Q_D(const QSGTextInput);
    return d->control->preeditAreaText().length() > 0;
}

void QSGTextInputPrivate::init()
{
    Q_Q(QSGTextInput);
#if defined(Q_WS_MAC)
    control->setThreadChecks(true);
#endif
    control->setParent(q);//Now mandatory due to accessibility changes
    control->setCursorWidth(1);
    control->setPasswordCharacter(QLatin1Char('*'));
    q->setSmooth(smooth);
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFlag(QSGItem::ItemAcceptsInputMethod);
    q->setFlag(QSGItem::ItemHasContents);
    q->connect(control, SIGNAL(cursorPositionChanged(int,int)),
               q, SLOT(cursorPosChanged()));
    q->connect(control, SIGNAL(selectionChanged()),
               q, SLOT(selectionChanged()));
    q->connect(control, SIGNAL(textChanged(QString)),
               q, SLOT(q_textChanged()));
    q->connect(control, SIGNAL(accepted()),
               q, SIGNAL(accepted()));
    q->connect(control, SIGNAL(updateNeeded(QRect)),
               q, SLOT(updateRect(QRect)));
#ifndef QT_NO_CLIPBOARD
    q->connect(q, SIGNAL(readOnlyChanged(bool)),
            q, SLOT(q_canPasteChanged()));
    q->connect(QGuiApplication::clipboard(), SIGNAL(dataChanged()),
            q, SLOT(q_canPasteChanged()));
    canPaste = !control->isReadOnly() && QGuiApplication::clipboard()->text().length() != 0;
#endif // QT_NO_CLIPBOARD
    q->connect(control, SIGNAL(updateMicroFocus()),
               q, SLOT(updateCursorRectangle()));
    q->connect(control, SIGNAL(displayTextChanged(QString)),
               q, SLOT(updateRect()));
    q->updateSize();
    oldValidity = control->hasAcceptableInput();
    lastSelectionStart = 0;
    lastSelectionEnd = 0;
    QPalette p = control->palette();
    selectedTextColor = p.color(QPalette::HighlightedText);
    selectionColor = p.color(QPalette::Highlight);
    determineHorizontalAlignment();

    if (!qmlDisableDistanceField()) {
        QTextOption option = control->textLayout()->textOption();
        option.setUseDesignMetrics(true);
        control->textLayout()->setTextOption(option);
    }
}

void QSGTextInput::cursorPosChanged()
{
    Q_D(QSGTextInput);
    updateCursorRectangle();
    emit cursorPositionChanged();
    // XXX todo - not in 4.8?
#if 0
    d->control->resetCursorBlinkTimer();
#endif

    if(!d->control->hasSelectedText()){
        if(d->lastSelectionStart != d->control->cursor()){
            d->lastSelectionStart = d->control->cursor();
            emit selectionStartChanged();
        }
        if(d->lastSelectionEnd != d->control->cursor()){
            d->lastSelectionEnd = d->control->cursor();
            emit selectionEndChanged();
        }
    }
}

void QSGTextInput::updateCursorRectangle()
{
    Q_D(QSGTextInput);
    d->updateHorizontalScroll();
    updateRect();//TODO: Only update rect between pos's
    updateMicroFocus();
    emit cursorRectangleChanged();
    if (d->cursorItem)
        d->cursorItem->setX(d->control->cursorToX() - d->hscroll);
}

void QSGTextInput::selectionChanged()
{
    Q_D(QSGTextInput);
    updateRect();//TODO: Only update rect in selection
    emit selectedTextChanged();

    if(d->lastSelectionStart != d->control->selectionStart()){
        d->lastSelectionStart = d->control->selectionStart();
        if(d->lastSelectionStart == -1)
            d->lastSelectionStart = d->control->cursor();
        emit selectionStartChanged();
    }
    if(d->lastSelectionEnd != d->control->selectionEnd()){
        d->lastSelectionEnd = d->control->selectionEnd();
        if(d->lastSelectionEnd == -1)
            d->lastSelectionEnd = d->control->cursor();
        emit selectionEndChanged();
    }
}

void QSGTextInput::q_textChanged()
{
    Q_D(QSGTextInput);
    emit textChanged();
    emit displayTextChanged();
    updateSize();
    d->determineHorizontalAlignment();
    d->updateHorizontalScroll();
    updateMicroFocus();
    if(hasAcceptableInput() != d->oldValidity){
        d->oldValidity = hasAcceptableInput();
        emit acceptableInputChanged();
    }
}

void QSGTextInputPrivate::showCursor()
{
    if (textNode != 0 && textNode->cursorNode() != 0)
        textNode->cursorNode()->setColor(color);
}

void QSGTextInputPrivate::hideCursor()
{
    if (textNode != 0 && textNode->cursorNode() != 0)
        textNode->cursorNode()->setColor(QColor(0, 0, 0, 0));
}

void QSGTextInput::updateRect(const QRect &r)
{
    Q_D(QSGTextInput);
    if (!isComponentComplete())
        return;

    if (r.isEmpty()) {
        d->textLayoutDirty = true;
    }

    update();
}

QRectF QSGTextInput::boundingRect() const
{
    Q_D(const QSGTextInput);
    QRectF r = QSGImplicitSizeItem::boundingRect();

    int cursorWidth = d->cursorItem ? d->cursorItem->width() : d->control->cursorWidth();

    // Could include font max left/right bearings to either side of rectangle.

    r.setRight(r.right() + cursorWidth);
    return r;
}

void QSGTextInput::updateSize(bool needsRedraw)
{
    Q_D(QSGTextInput);
    int w = width();
    int h = height();
    setImplicitHeight(d->control->height()-1); // -1 to counter QLineControl's +1 which is not consistent with Text.
    setImplicitWidth(d->calculateTextWidth());
    if(w==width() && h==height() && needsRedraw)
        update();
}

void QSGTextInput::q_canPasteChanged()
{
    Q_D(QSGTextInput);
    bool old = d->canPaste;
#ifndef QT_NO_CLIPBOARD
    d->canPaste = !d->control->isReadOnly() && QGuiApplication::clipboard()->text().length() != 0;
#endif
    if(d->canPaste != old)
        emit canPasteChanged();
}

QT_END_NAMESPACE

