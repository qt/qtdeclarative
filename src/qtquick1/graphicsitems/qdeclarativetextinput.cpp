/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include "private/qdeclarativetextinput_p.h"
#include "private/qdeclarativetextinput_p_p.h"

#include <private/qdeclarativeglobal_p.h>
#include <qdeclarativeinfo.h>

#include <QValidator>
#include <QTextCursor>
#include <QApplication>
#include <QtGui/QInputPanel>
#include <QFontMetrics>
#include <QPainter>
#include <QTextBoundaryFinder>
#include <QInputContext>
#include <qstyle.h>

#ifndef QT_NO_LINEEDIT

QT_BEGIN_NAMESPACE



/*!
    \qmlclass TextInput QDeclarative1TextInput
    \inqmlmodule QtQuick 1
    \ingroup qml-basic-visual-elements
    \since QtQuick 1.0
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
QDeclarative1TextInput::QDeclarative1TextInput(QDeclarativeItem* parent)
    : QDeclarative1ImplicitSizePaintedItem(*(new QDeclarative1TextInputPrivate), parent)
{
    Q_D(QDeclarative1TextInput);
    d->init();
}

QDeclarative1TextInput::~QDeclarative1TextInput()
{
}

/*!
    \qmlproperty string QtQuick1::TextInput::text

    The text in the TextInput.
*/

QString QDeclarative1TextInput::text() const
{
    Q_D(const QDeclarative1TextInput);
    return d->control->text();
}

void QDeclarative1TextInput::setText(const QString &s)
{
    Q_D(QDeclarative1TextInput);
    if(s == text())
        return;
    d->control->setText(s);
}

/*!
    \qmlproperty string QtQuick1::TextInput::font.family

    Sets the family name of the font.

    The family name is case insensitive and may optionally include a foundry name, e.g. "Helvetica [Cronyx]".
    If the family is available from more than one foundry and the foundry isn't specified, an arbitrary foundry is chosen.
    If the family isn't available a family will be set using the font matching algorithm.
*/

/*!
    \qmlproperty bool QtQuick1::TextInput::font.bold

    Sets whether the font weight is bold.
*/

/*!
    \qmlproperty enumeration QtQuick1::TextInput::font.weight

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
    \qmlproperty bool QtQuick1::TextInput::font.italic

    Sets whether the font has an italic style.
*/

/*!
    \qmlproperty bool QtQuick1::TextInput::font.underline

    Sets whether the text is underlined.
*/

/*!
    \qmlproperty bool QtQuick1::TextInput::font.strikeout

    Sets whether the font has a strikeout style.
*/

/*!
    \qmlproperty real QtQuick1::TextInput::font.pointSize

    Sets the font size in points. The point size must be greater than zero.
*/

/*!
    \qmlproperty int QtQuick1::TextInput::font.pixelSize

    Sets the font size in pixels.

    Using this function makes the font device dependent.
    Use \c pointSize to set the size of the font in a device independent manner.
*/

/*!
    \qmlproperty real QtQuick1::TextInput::font.letterSpacing

    Sets the letter spacing for the font.

    Letter spacing changes the default spacing between individual letters in the font.
    A positive value increases the letter spacing by the corresponding pixels; a negative value decreases the spacing.
*/

/*!
    \qmlproperty real QtQuick1::TextInput::font.wordSpacing

    Sets the word spacing for the font.

    Word spacing changes the default spacing between individual words.
    A positive value increases the word spacing by a corresponding amount of pixels,
    while a negative value decreases the inter-word spacing accordingly.
*/

/*!
    \qmlproperty enumeration QtQuick1::TextInput::font.capitalization

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

QFont QDeclarative1TextInput::font() const
{
    Q_D(const QDeclarative1TextInput);
    return d->sourceFont;
}

void QDeclarative1TextInput::setFont(const QFont &font)
{
    Q_D(QDeclarative1TextInput);
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
    \qmlproperty color QtQuick1::TextInput::color

    The text color.
*/
QColor QDeclarative1TextInput::color() const
{
    Q_D(const QDeclarative1TextInput);
    return d->color;
}

void QDeclarative1TextInput::setColor(const QColor &c)
{
    Q_D(QDeclarative1TextInput);
    if (c != d->color) {
        d->color = c;
        clearCache();
        update();
        emit colorChanged(c);
    }
}


/*!
    \qmlproperty color QtQuick1::TextInput::selectionColor

    The text highlight color, used behind selections.
*/
QColor QDeclarative1TextInput::selectionColor() const
{
    Q_D(const QDeclarative1TextInput);
    return d->selectionColor;
}

void QDeclarative1TextInput::setSelectionColor(const QColor &color)
{
    Q_D(QDeclarative1TextInput);
    if (d->selectionColor == color)
        return;

    d->selectionColor = color;
    QPalette p = d->control->palette();
    p.setColor(QPalette::Highlight, d->selectionColor);
    d->control->setPalette(p);
    if (d->control->hasSelectedText()) {
        clearCache();
        update();
    }
    emit selectionColorChanged(color);
}

/*!
    \qmlproperty color QtQuick1::TextInput::selectedTextColor

    The highlighted text color, used in selections.
*/
QColor QDeclarative1TextInput::selectedTextColor() const
{
    Q_D(const QDeclarative1TextInput);
    return d->selectedTextColor;
}

void QDeclarative1TextInput::setSelectedTextColor(const QColor &color)
{
    Q_D(QDeclarative1TextInput);
    if (d->selectedTextColor == color)
        return;

    d->selectedTextColor = color;
    QPalette p = d->control->palette();
    p.setColor(QPalette::HighlightedText, d->selectedTextColor);
    d->control->setPalette(p);
    if (d->control->hasSelectedText()) {
        clearCache();
        update();
    }
    emit selectedTextColorChanged(color);
}

/*!
    \qmlproperty enumeration QtQuick1::TextInput::horizontalAlignment
    \qmlproperty enumeration QtQuick1::TextInput::effectiveHorizontalAlignment

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
QDeclarative1TextInput::HAlignment QDeclarative1TextInput::hAlign() const
{
    Q_D(const QDeclarative1TextInput);
    return d->hAlign;
}

void QDeclarative1TextInput::setHAlign(HAlignment align)
{
    Q_D(QDeclarative1TextInput);
    bool forceAlign = d->hAlignImplicit && d->effectiveLayoutMirror;
    d->hAlignImplicit = false;
    if (d->setHAlign(align, forceAlign) && isComponentComplete()) {
        updateCursorRectangle();
    }
}

void QDeclarative1TextInput::resetHAlign()
{
    Q_D(QDeclarative1TextInput);
    d->hAlignImplicit = true;
    if (d->determineHorizontalAlignment() && isComponentComplete()) {
        updateCursorRectangle();
    }
}

QDeclarative1TextInput::HAlignment QDeclarative1TextInput::effectiveHAlign() const
{
    Q_D(const QDeclarative1TextInput);
    QDeclarative1TextInput::HAlignment effectiveAlignment = d->hAlign;
    if (!d->hAlignImplicit && d->effectiveLayoutMirror) {
        switch (d->hAlign) {
        case QDeclarative1TextInput::AlignLeft:
            effectiveAlignment = QDeclarative1TextInput::AlignRight;
            break;
        case QDeclarative1TextInput::AlignRight:
            effectiveAlignment = QDeclarative1TextInput::AlignLeft;
            break;
        default:
            break;
        }
    }
    return effectiveAlignment;
}

bool QDeclarative1TextInputPrivate::setHAlign(QDeclarative1TextInput::HAlignment alignment, bool forceAlign)
{
    Q_Q(QDeclarative1TextInput);
    if ((hAlign != alignment || forceAlign) && alignment <= QDeclarative1TextInput::AlignHCenter) { // justify not supported
        QDeclarative1TextInput::HAlignment oldEffectiveHAlign = q->effectiveHAlign();
        hAlign = alignment;
        emit q->horizontalAlignmentChanged(alignment);
        if (oldEffectiveHAlign != q->effectiveHAlign())
            emit q->effectiveHorizontalAlignmentChanged();
        return true;
    }
    return false;
}

bool QDeclarative1TextInputPrivate::determineHorizontalAlignment()
{
    if (hAlignImplicit) {
        // if no explicit alignment has been set, follow the natural layout direction of the text
        QString text = control->text();
        if (text.isEmpty())
            text = control->preeditAreaText();
        bool isRightToLeft = text.isEmpty()
                ? qApp->inputPanel()->inputDirection() == Qt::RightToLeft
                : text.isRightToLeft();
        return setHAlign(isRightToLeft ? QDeclarative1TextInput::AlignRight : QDeclarative1TextInput::AlignLeft);
    }
    return false;
}

void QDeclarative1TextInputPrivate::mirrorChange()
{
    Q_Q(QDeclarative1TextInput);
    if (q->isComponentComplete()) {
        if (!hAlignImplicit && (hAlign == QDeclarative1TextInput::AlignRight || hAlign == QDeclarative1TextInput::AlignLeft)) {
            q->updateCursorRectangle();
            emit q->effectiveHorizontalAlignmentChanged();
        }
    }
}

/*!
    \qmlproperty bool QtQuick1::TextInput::readOnly

    Sets whether user input can modify the contents of the TextInput.

    If readOnly is set to true, then user input will not affect the text
    property. Any bindings or attempts to set the text property will still
    work.
*/

bool QDeclarative1TextInput::isReadOnly() const
{
    Q_D(const QDeclarative1TextInput);
    return d->control->isReadOnly();
}

void QDeclarative1TextInput::setReadOnly(bool ro)
{
    Q_D(QDeclarative1TextInput);
    if (d->control->isReadOnly() == ro)
        return;

    setFlag(QGraphicsItem::ItemAcceptsInputMethod, !ro);
    d->control->setReadOnly(ro);

    emit readOnlyChanged(ro);
}

/*!
    \qmlproperty int QtQuick1::TextInput::maximumLength
    The maximum permitted length of the text in the TextInput.

    If the text is too long, it is truncated at the limit.

    By default, this property contains a value of 32767.
*/
int QDeclarative1TextInput::maxLength() const
{
    Q_D(const QDeclarative1TextInput);
    return d->control->maxLength();
}

void QDeclarative1TextInput::setMaxLength(int ml)
{
    Q_D(QDeclarative1TextInput);
    if (d->control->maxLength() == ml)
        return;

    d->control->setMaxLength(ml);

    emit maximumLengthChanged(ml);
}

/*!
    \qmlproperty bool QtQuick1::TextInput::cursorVisible
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
bool QDeclarative1TextInput::isCursorVisible() const
{
    Q_D(const QDeclarative1TextInput);
    return d->cursorVisible;
}

void QDeclarative1TextInput::setCursorVisible(bool on)
{
    Q_D(QDeclarative1TextInput);
    if (d->cursorVisible == on)
        return;
    d->cursorVisible = on;
    d->control->setCursorBlinkPeriod(on?QApplication::cursorFlashTime():0);
    QRect r = d->control->cursorRect();
    if (d->control->inputMask().isEmpty())
        updateRect(r);
    else
        updateRect();
    emit cursorVisibleChanged(d->cursorVisible);
}

/*!
    \qmlproperty int QtQuick1::TextInput::cursorPosition
    The position of the cursor in the TextInput.
*/
int QDeclarative1TextInput::cursorPosition() const
{
    Q_D(const QDeclarative1TextInput);
    return d->control->cursor();
}
void QDeclarative1TextInput::setCursorPosition(int cp)
{
    Q_D(QDeclarative1TextInput);
    if (cp < 0 || cp > d->control->text().length())
        return;
    d->control->moveCursor(cp);
}

/*!
  Returns a Rect which encompasses the cursor, but which may be larger than is
  required. Ignores custom cursor delegates.
*/
QRect QDeclarative1TextInput::cursorRectangle() const
{
    Q_D(const QDeclarative1TextInput);
    QRect r = d->control->cursorRect();
    // Scroll and make consistent with TextEdit
    // QWidgetLineControl inexplicably adds 1 to the height and horizontal padding
    // for unicode direction markers.
    r.adjust(5 - d->hscroll, 0, -4 - d->hscroll, -1);
    return r;
}

/*!
    \qmlproperty int QtQuick1::TextInput::selectionStart

    The cursor position before the first character in the current selection.

    This property is read-only. To change the selection, use select(start,end),
    selectAll(), or selectWord().

    \sa selectionEnd, cursorPosition, selectedText
*/
int QDeclarative1TextInput::selectionStart() const
{
    Q_D(const QDeclarative1TextInput);
    return d->lastSelectionStart;
}

/*!
    \qmlproperty int QtQuick1::TextInput::selectionEnd

    The cursor position after the last character in the current selection.

    This property is read-only. To change the selection, use select(start,end),
    selectAll(), or selectWord().

    \sa selectionStart, cursorPosition, selectedText
*/
int QDeclarative1TextInput::selectionEnd() const
{
    Q_D(const QDeclarative1TextInput);
    return d->lastSelectionEnd;
}

/*!
    \qmlmethod void QtQuick1::TextInput::select(int start, int end)

    Causes the text from \a start to \a end to be selected.

    If either start or end is out of range, the selection is not changed.

    After calling this, selectionStart will become the lesser
    and selectionEnd will become the greater (regardless of the order passed
    to this method).

    \sa selectionStart, selectionEnd
*/
void QDeclarative1TextInput::select(int start, int end)
{
    Q_D(QDeclarative1TextInput);
    if (start < 0 || end < 0 || start > d->control->text().length() || end > d->control->text().length())
        return;
    d->control->setSelection(start, end-start);
}

/*!
    \qmlproperty string QtQuick1::TextInput::selectedText

    This read-only property provides the text currently selected in the
    text input.

    It is equivalent to the following snippet, but is faster and easier
    to use.

    \js
    myTextInput.text.toString().substring(myTextInput.selectionStart,
        myTextInput.selectionEnd);
    \endjs
*/
QString QDeclarative1TextInput::selectedText() const
{
    Q_D(const QDeclarative1TextInput);
    return d->control->selectedText();
}

/*!
    \qmlproperty bool QtQuick1::TextInput::activeFocusOnPress

    Whether the TextInput should gain active focus on a mouse press. By default this is
    set to true.
*/
bool QDeclarative1TextInput::focusOnPress() const
{
    Q_D(const QDeclarative1TextInput);
    return d->focusOnPress;
}

void QDeclarative1TextInput::setFocusOnPress(bool b)
{
    Q_D(QDeclarative1TextInput);
    if (d->focusOnPress == b)
        return;

    d->focusOnPress = b;

    emit activeFocusOnPressChanged(d->focusOnPress);
}

/*!
    \qmlproperty bool QtQuick1::TextInput::autoScroll

    Whether the TextInput should scroll when the text is longer than the width. By default this is
    set to true.
*/
bool QDeclarative1TextInput::autoScroll() const
{
    Q_D(const QDeclarative1TextInput);
    return d->autoScroll;
}

void QDeclarative1TextInput::setAutoScroll(bool b)
{
    Q_D(QDeclarative1TextInput);
    if (d->autoScroll == b)
        return;

    d->autoScroll = b;
    //We need to repaint so that the scrolling is taking into account.
    updateSize(true);
    updateCursorRectangle();
    emit autoScrollChanged(d->autoScroll);
}

/*!
    \qmlclass IntValidator QIntValidator
    \inqmlmodule QtQuick 1
    \ingroup qml-basic-visual-elements

    This element provides a validator for integer values.

    IntValidator uses the \l {QLocale::setDefault()}{default locale} to interpret the number and
    will accept locale specific digits, group separators, and positive and negative signs.  In
    addition, IntValidator is always guaranteed to accept a number formatted according to the "C"
    locale.
*/
/*!
    \qmlproperty int QtQuick1::IntValidator::top

    This property holds the validator's highest acceptable value.
    By default, this property's value is derived from the highest signed integer available (typically 2147483647).
*/
/*!
    \qmlproperty int QtQuick1::IntValidator::bottom

    This property holds the validator's lowest acceptable value.
    By default, this property's value is derived from the lowest signed integer available (typically -2147483647).
*/

/*!
    \qmlclass DoubleValidator QDoubleValidator
    \inqmlmodule QtQuick 1
    \ingroup qml-basic-visual-elements

    This element provides a validator for non-integer numbers.
*/

/*!
    \qmlproperty real QtQuick1::DoubleValidator::top

    This property holds the validator's maximum acceptable value.
    By default, this property contains a value of infinity.
*/
/*!
    \qmlproperty real QtQuick1::DoubleValidator::bottom

    This property holds the validator's minimum acceptable value.
    By default, this property contains a value of -infinity.
*/
/*!
    \qmlproperty int QtQuick1::DoubleValidator::decimals

    This property holds the validator's maximum number of digits after the decimal point.
    By default, this property contains a value of 1000.
*/
/*!
    \qmlproperty enumeration QtQuick1::DoubleValidator::notation
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
    \inqmlmodule QtQuick 1
    \ingroup qml-basic-visual-elements

    This element provides a validator, which counts as valid any string which
    matches a specified regular expression.
*/
/*!
   \qmlproperty regExp QtQuick1::RegExpValidator::regExp

   This property holds the regular expression used for validation.

   Note that this property should be a regular expression in JS syntax, e.g /a/ for the regular expression
   matching "a".

   By default, this property contains a regular expression with the pattern .* that matches any string.
*/

/*!
    \qmlproperty Validator QtQuick1::TextInput::validator

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
#ifndef QT_NO_VALIDATOR
QValidator* QDeclarative1TextInput::validator() const
{
    Q_D(const QDeclarative1TextInput);
    //###const cast isn't good, but needed for property system?
    return const_cast<QValidator*>(d->control->validator());
}

void QDeclarative1TextInput::setValidator(QValidator* v)
{
    Q_D(QDeclarative1TextInput);
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
    \qmlproperty string QtQuick1::TextInput::inputMask

    Allows you to set an input mask on the TextInput, restricting the allowable
    text inputs. See QLineEdit::inputMask for further details, as the exact
    same mask strings are used by TextInput.

    \sa acceptableInput, validator
*/
QString QDeclarative1TextInput::inputMask() const
{
    Q_D(const QDeclarative1TextInput);
    return d->control->inputMask();
}

void QDeclarative1TextInput::setInputMask(const QString &im)
{
    Q_D(QDeclarative1TextInput);
    if (d->control->inputMask() == im)
        return;

    d->control->setInputMask(im);
    emit inputMaskChanged(d->control->inputMask());
}

/*!
    \qmlproperty bool QtQuick1::TextInput::acceptableInput

    This property is always true unless a validator or input mask has been set.
    If a validator or input mask has been set, this property will only be true
    if the current text is acceptable to the validator or input mask as a final
    string (not as an intermediate string).
*/
bool QDeclarative1TextInput::hasAcceptableInput() const
{
    Q_D(const QDeclarative1TextInput);
    return d->control->hasAcceptableInput();
}

/*!
    \qmlsignal QtQuick1::TextInput::onAccepted()

    This handler is called when the Return or Enter key is pressed.
    Note that if there is a \l validator or \l inputMask set on the text
    input, the handler will only be emitted if the input is in an acceptable
    state.
*/

void QDeclarative1TextInputPrivate::updateInputMethodHints()
{
    Q_Q(QDeclarative1TextInput);
    Qt::InputMethodHints hints = inputMethodHints;
    uint echo = control->echoMode();
    if (echo == QDeclarative1TextInput::Password || echo == QDeclarative1TextInput::NoEcho)
        hints |= Qt::ImhHiddenText;
    else if (echo == QDeclarative1TextInput::PasswordEchoOnEdit)
        hints &= ~Qt::ImhHiddenText;
    if (echo != QDeclarative1TextInput::Normal) {
        hints |= Qt::ImhNoAutoUppercase;
        hints |= Qt::ImhNoPredictiveText;
    }
    q->setInputMethodHints(hints);
}

/*!
    \qmlproperty enumeration QtQuick1::TextInput::echoMode

    Specifies how the text should be displayed in the TextInput.
    \list
    \o TextInput.Normal - Displays the text as it is. (Default)
    \o TextInput.Password - Displays asterixes instead of characters.
    \o TextInput.NoEcho - Displays nothing.
    \o TextInput.PasswordEchoOnEdit - Displays characters as they are entered
    while editing, otherwise displays asterisks.
    \endlist
*/
QDeclarative1TextInput::EchoMode QDeclarative1TextInput::echoMode() const
{
    Q_D(const QDeclarative1TextInput);
    return (QDeclarative1TextInput::EchoMode)d->control->echoMode();
}

void QDeclarative1TextInput::setEchoMode(QDeclarative1TextInput::EchoMode echo)
{
    Q_D(QDeclarative1TextInput);
    if (echoMode() == echo)
        return;
    d->control->setEchoMode(echo);
    d->updateInputMethodHints();
    q_textChanged();
    emit echoModeChanged(echoMode());
}

Qt::InputMethodHints QDeclarative1TextInput::imHints() const
{
    Q_D(const QDeclarative1TextInput);
    return d->inputMethodHints;
}

void QDeclarative1TextInput::setIMHints(Qt::InputMethodHints hints)
{
    Q_D(QDeclarative1TextInput);
    if (d->inputMethodHints == hints)
        return;
    d->inputMethodHints = hints;
    d->updateInputMethodHints();
}

/*!
    \qmlproperty Component QtQuick1::TextInput::cursorDelegate
    The delegate for the cursor in the TextInput.

    If you set a cursorDelegate for a TextInput, this delegate will be used for
    drawing the cursor instead of the standard cursor. An instance of the
    delegate will be created and managed by the TextInput when a cursor is
    needed, and the x property of delegate instance will be set so as
    to be one pixel before the top left of the current character.

    Note that the root item of the delegate component must be a QDeclarativeItem or
    QDeclarativeItem derived item.
*/
QDeclarativeComponent* QDeclarative1TextInput::cursorDelegate() const
{
    Q_D(const QDeclarative1TextInput);
    return d->cursorComponent;
}

void QDeclarative1TextInput::setCursorDelegate(QDeclarativeComponent* c)
{
    Q_D(QDeclarative1TextInput);
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

void QDeclarative1TextInputPrivate::startCreatingCursor()
{
    Q_Q(QDeclarative1TextInput);
    if(cursorComponent->isReady()){
        q->createCursor();
    }else if(cursorComponent->isLoading()){
        q->connect(cursorComponent, SIGNAL(statusChanged(int)),
                q, SLOT(createCursor()));
    }else {//isError
        qmlInfo(q, cursorComponent->errors()) << QDeclarative1TextInput::tr("Could not load cursor delegate");
    }
}

void QDeclarative1TextInput::createCursor()
{
    Q_D(QDeclarative1TextInput);
    if(d->cursorComponent->isError()){
        qmlInfo(this, d->cursorComponent->errors()) << tr("Could not load cursor delegate");
        return;
    }

    if(!d->cursorComponent->isReady())
        return;

    if(d->cursorItem)
        delete d->cursorItem;
    d->cursorItem = qobject_cast<QDeclarativeItem*>(d->cursorComponent->create());
    if(!d->cursorItem){
        qmlInfo(this, d->cursorComponent->errors()) << tr("Could not instantiate cursor delegate");
        return;
    }

    QDeclarative_setParent_noEvent(d->cursorItem, this);
    d->cursorItem->setParentItem(this);
    d->cursorItem->setX(d->control->cursorToX());
    d->cursorItem->setHeight(d->control->height()-1); // -1 to counter QWidgetLineControl's +1 which is not consistent with Text.
}

/*!
    \qmlmethod rect QtQuick1::TextInput::positionToRectangle(int pos)

    This function takes a character position and returns the rectangle that the
    cursor would occupy, if it was placed at that character position.

    This is similar to setting the cursorPosition, and then querying the cursor
    rectangle, but the cursorPosition is not changed.
*/
QRectF QDeclarative1TextInput::positionToRectangle(int pos) const
{
    Q_D(const QDeclarative1TextInput);
    if (pos > d->control->cursorPosition())
        pos += d->control->preeditAreaText().length();
    return QRectF(d->control->cursorToX(pos)-d->hscroll,
        0.0,
        d->control->cursorWidth(),
        cursorRectangle().height());
}

int QDeclarative1TextInput::positionAt(int x) const
{
    return positionAt(x, CursorBetweenCharacters);
}

/*!
    \qmlmethod int QtQuick1::TextInput::positionAt(int x, CursorPosition position = CursorBetweenCharacters)
    \since Quick 1.1

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
int QDeclarative1TextInput::positionAt(int x, CursorPosition position) const
{
    Q_D(const QDeclarative1TextInput);
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

void QDeclarative1TextInputPrivate::focusChanged(bool hasFocus)
{
    Q_Q(QDeclarative1TextInput);
    focused = hasFocus;
    q->setCursorVisible(hasFocus && scene && scene->hasFocus());
    if(!hasFocus && control->passwordEchoEditing())
        control->updatePasswordEchoEditing(false);//QWidgetLineControl sets it on key events, but doesn't deal with focus events
    if (!hasFocus) {
        control->commitPreedit();
        control->deselect();
    }
    QDeclarativeItemPrivate::focusChanged(hasFocus);
}

void QDeclarative1TextInput::keyPressEvent(QKeyEvent* ev)
{
    Q_D(QDeclarative1TextInput);
    keyPressPreHandler(ev);
    if (ev->isAccepted())
        return;

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
        QDeclarative1PaintedItem::keyPressEvent(ev);
}

void QDeclarative1TextInput::inputMethodEvent(QInputMethodEvent *ev)
{
    Q_D(QDeclarative1TextInput);
    ev->ignore();
    const bool wasComposing = d->control->preeditAreaText().length() > 0;
    inputMethodPreHandler(ev);
    if (!ev->isAccepted()) {
        if (d->control->isReadOnly()) {
            ev->ignore();
        } else {
            d->control->processInputMethodEvent(ev);
        }
    }
    if (!ev->isAccepted())
        QDeclarative1PaintedItem::inputMethodEvent(ev);

    if (wasComposing != (d->control->preeditAreaText().length() > 0))
        emit inputMethodComposingChanged();
}

/*!
\overload
Handles the given mouse \a event.
*/
void QDeclarative1TextInput::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1TextInput);
    if (d->selectByMouse && event->button() == Qt::LeftButton) {
        int cursor = d->xToPos(event->pos().x());
        d->control->selectWordAtPos(cursor);
        event->setAccepted(true);
    } else {
        if (d->sendMouseEventToInputContext(event, QEvent::MouseButtonDblClick))
            return;
        QDeclarative1PaintedItem::mouseDoubleClickEvent(event);
    }
}

void QDeclarative1TextInput::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1TextInput);

    d->pressPos = event->pos();

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
    }
    if (d->sendMouseEventToInputContext(event, QEvent::MouseButtonPress))
        return;

    bool mark = (event->modifiers() & Qt::ShiftModifier) && d->selectByMouse;
    int cursor = d->xToPos(event->pos().x());
    d->control->moveCursor(cursor, mark);
    event->setAccepted(true);
}

void QDeclarative1TextInput::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1TextInput);

    if (d->selectPressed) {
        if (qAbs(int(event->pos().x() - d->pressPos.x())) > QApplication::startDragDistance())
            setKeepMouseGrab(true);

        if (d->control->composeMode()) {
            // start selection
            int startPos = d->xToPos(d->pressPos.x());
            int currentPos = d->xToPos(event->pos().x());
            if (startPos != currentPos)
                d->control->setSelection(startPos, currentPos - startPos);
        } else {
            moveCursorSelection(d->xToPos(event->pos().x()), d->mouseSelectionMode);
        }
        event->setAccepted(true);
    } else {
        QDeclarative1PaintedItem::mouseMoveEvent(event);
    }
}

/*!
\overload
Handles the given mouse \a event.
*/
void QDeclarative1TextInput::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1TextInput);
    if (d->sendMouseEventToInputContext(event, QEvent::MouseButtonRelease))
        return;
    if (d->selectPressed) {
        d->selectPressed = false;
        setKeepMouseGrab(false);
    }
    if (!d->showInputPanelOnFocus) { // input panel on click
        if (d->focusOnPress && !isReadOnly() && boundingRect().contains(event->pos())) {
            if (QGraphicsView * view = qobject_cast<QGraphicsView*>(qApp->focusWidget())) {
                if (view->scene() && view->scene() == scene()) {
                    qt_widget_private(view)->handleSoftwareInputPanel(event->button(), d->clickCausedFocus);
                }
            }
        }
    }
    d->clickCausedFocus = false;
#ifndef QT_NO_CLIPBOARD
    if (QGuiApplication::clipboard()->supportsSelection()) {
        if (event->button() == Qt::LeftButton) {
            d->control->copy(QClipboard::Selection);
        } else if (!isReadOnly() && event->button() == Qt::MidButton) {
            d->control->deselect();
            d->control->insert(QGuiApplication::clipboard()->text(QClipboard::Selection));
        }
    }
#endif
    if (!event->isAccepted())
        QDeclarative1PaintedItem::mouseReleaseEvent(event);
}

bool QDeclarative1TextInputPrivate::sendMouseEventToInputContext(
        QGraphicsSceneMouseEvent *event, QEvent::Type eventType)
{
#if !defined QT_NO_IM
    if (event->widget() && control->composeMode()) {
        int tmp_cursor = xToPos(event->pos().x());
        int mousePos = tmp_cursor - control->cursor();
        if (mousePos >= 0 && mousePos <= control->preeditAreaText().length()) {
            if (eventType == QEvent::MouseButtonRelease) {
                qApp->inputPanel()->invokeAction(QInputPanel::Click, mousePos);
            }
            return true;
        }
    }
#else
    Q_UNUSED(event);
    Q_UNUSED(eventType)
#endif

    return false;
}

bool QDeclarative1TextInput::sceneEvent(QEvent *event)
{
    Q_D(QDeclarative1TextInput);
    bool rv = QDeclarativeItem::sceneEvent(event);
    if (event->type() == QEvent::UngrabMouse) {
        d->selectPressed = false;
        setKeepMouseGrab(false);
    }
    return rv;
}

bool QDeclarative1TextInput::event(QEvent* ev)
{
#ifndef QT_NO_SHORTCUT
    Q_D(QDeclarative1TextInput);

    if (ev->type() == QEvent::ShortcutOverride) {
        d->control->processShortcutOverrideEvent(static_cast<QKeyEvent *>(ev));
        return ev->isAccepted();
    }
#endif
    return QDeclarative1PaintedItem::event(ev);
}

void QDeclarative1TextInput::geometryChanged(const QRectF &newGeometry,
                                  const QRectF &oldGeometry)
{
    if (newGeometry.width() != oldGeometry.width()) {
        updateSize();
        updateCursorRectangle();
    }
    QDeclarative1PaintedItem::geometryChanged(newGeometry, oldGeometry);
}

int QDeclarative1TextInputPrivate::calculateTextWidth()
{
    return qRound(control->naturalTextWidth());
}

void QDeclarative1TextInputPrivate::updateHorizontalScroll()
{
    Q_Q(QDeclarative1TextInput);
    const int preeditLength = control->preeditAreaText().length();
    int cix = qRound(control->cursorToX(control->cursor() + preeditLength));
    QRect br(q->boundingRect().toRect());
    int widthUsed = calculateTextWidth();

    QDeclarative1TextInput::HAlignment effectiveHAlign = q->effectiveHAlign();
    if (autoScroll) {
        if (widthUsed <=  br.width()) {
            // text fits in br; use hscroll for alignment
            switch (effectiveHAlign & ~(Qt::AlignAbsolute|Qt::AlignVertical_Mask)) {
            case Qt::AlignRight:
                hscroll = widthUsed - br.width() - 1;
                break;
            case Qt::AlignHCenter:
                hscroll = (widthUsed - br.width()) / 2;
                break;
            default:
                // Left
                hscroll = 0;
                break;
            }
        } else if (cix - hscroll >= br.width()) {
            // text doesn't fit, cursor is to the right of br (scroll right)
            hscroll = cix - br.width() + 1;
        } else if (cix - hscroll < 0 && hscroll < widthUsed) {
            // text doesn't fit, cursor is to the left of br (scroll left)
            hscroll = cix;
        } else if (widthUsed - hscroll < br.width()) {
            // text doesn't fit, text document is to the left of br; align
            // right
            hscroll = widthUsed - br.width() + 1;
        }
        if (preeditLength > 0) {
            // check to ensure long pre-edit text doesn't push the cursor
            // off to the left
             cix = qRound(control->cursorToX(
                     control->cursor() + qMax(0, control->preeditCursor() - 1)));
             if (cix < hscroll)
                 hscroll = cix;
        }
    } else {
        switch (effectiveHAlign) {
        case QDeclarative1TextInput::AlignRight:
            hscroll = q->width() - widthUsed;
            break;
        case QDeclarative1TextInput::AlignHCenter:
            hscroll = (q->width() - widthUsed) / 2;
            break;
        default:
            // Left
            hscroll = 0;
            break;
        }
    }
}

void QDeclarative1TextInput::drawContents(QPainter *p, const QRect &r)
{
    Q_D(QDeclarative1TextInput);
    p->setRenderHint(QPainter::TextAntialiasing, true);
    p->save();
    p->setPen(QPen(d->color));
    int flags = QWidgetLineControl::DrawText;
    if(!isReadOnly() && d->cursorVisible && !d->cursorItem)
        flags |= QWidgetLineControl::DrawCursor;
    if (d->control->hasSelectedText())
            flags |= QWidgetLineControl::DrawSelections;
    QPoint offset = QPoint(0,0);
    QFontMetrics fm = QFontMetrics(d->font);
    QRect br(boundingRect().toRect());
    if (d->autoScroll) {
        // the y offset is there to keep the baseline constant in case we have script changes in the text.
        offset = br.topLeft() - QPoint(d->hscroll, d->control->ascent() - fm.ascent());
    } else {
        offset = QPoint(d->hscroll, 0);
    }
    d->control->draw(p, offset, r, flags);
    p->restore();
}

/*!
\overload
Returns the value of the given \a property.
*/
QVariant QDeclarative1TextInput::inputMethodQuery(Qt::InputMethodQuery property) const
{
    Q_D(const QDeclarative1TextInput);
    switch(property) {
    case Qt::ImMicroFocus:
        return cursorRectangle();
    case Qt::ImFont:
        return font();
    case Qt::ImCursorPosition:
        return QVariant(d->control->cursor());
    case Qt::ImSurroundingText:
        if (d->control->echoMode() == PasswordEchoOnEdit
            && !d->control->passwordEchoEditing())
            return QVariant(displayText());
        else
            return QVariant(d->control->realText());
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
    \qmlmethod void QtQuick1::TextInput::deselect()
    \since Quick 1.1

    Removes active text selection.
*/
void QDeclarative1TextInput::deselect()
{
    Q_D(QDeclarative1TextInput);
    d->control->deselect();
}

/*!
    \qmlmethod void QtQuick1::TextInput::selectAll()

    Causes all text to be selected.
*/
void QDeclarative1TextInput::selectAll()
{
    Q_D(QDeclarative1TextInput);
    d->control->setSelection(0, d->control->text().length());
}

/*!
    \qmlmethod void QtQuick1::TextInput::isRightToLeft(int start, int end)

    Returns true if the natural reading direction of the editor text
    found between positions \a start and \a end is right to left.
*/
bool QDeclarative1TextInput::isRightToLeft(int start, int end)
{
    Q_D(QDeclarative1TextInput);
    if (start > end) {
        qmlInfo(this) << "isRightToLeft(start, end) called with the end property being smaller than the start.";
        return false;
    } else {
        return d->control->text().mid(start, end - start).isRightToLeft();
    }
}

#ifndef QT_NO_CLIPBOARD
/*!
    \qmlmethod QtQuick1::TextInput::cut()

    Moves the currently selected text to the system clipboard.
*/
void QDeclarative1TextInput::cut()
{
    Q_D(QDeclarative1TextInput);
    d->control->copy();
    d->control->del();
}

/*!
    \qmlmethod QtQuick1::TextInput::copy()

    Copies the currently selected text to the system clipboard.
*/
void QDeclarative1TextInput::copy()
{
    Q_D(QDeclarative1TextInput);
    d->control->copy();
}

/*!
    \qmlmethod QtQuick1::TextInput::paste()

    Replaces the currently selected text by the contents of the system clipboard.
*/
void QDeclarative1TextInput::paste()
{
    Q_D(QDeclarative1TextInput);
    if(!d->control->isReadOnly())
        d->control->paste();
}
#endif // QT_NO_CLIPBOARD

/*!
    \qmlmethod void QtQuick1::TextInput::selectWord()

    Causes the word closest to the current cursor position to be selected.
*/
void QDeclarative1TextInput::selectWord()
{
    Q_D(QDeclarative1TextInput);
    d->control->selectWordAtPos(d->control->cursor());
}

/*!
    \qmlproperty bool QtQuick1::TextInput::smooth

    This property holds whether the text is smoothly scaled or transformed.

    Smooth filtering gives better visual quality, but is slower.  If
    the item is displayed at its natural size, this property has no visual or
    performance effect.

    \note Generally scaling artifacts are only visible if the item is stationary on
    the screen.  A common pattern when animating an item is to disable smooth
    filtering at the beginning of the animation and reenable it at the conclusion.
*/

/*!
   \qmlproperty string QtQuick1::TextInput::passwordCharacter

   This is the character displayed when echoMode is set to Password or
   PasswordEchoOnEdit. By default it is an asterisk.

   If this property is set to a string with more than one character,
   the first character is used. If the string is empty, the value
   is ignored and the property is not set.
*/
QString QDeclarative1TextInput::passwordCharacter() const
{
    Q_D(const QDeclarative1TextInput);
    return QString(d->control->passwordCharacter());
}

void QDeclarative1TextInput::setPasswordCharacter(const QString &str)
{
    Q_D(QDeclarative1TextInput);
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
   \qmlproperty string QtQuick1::TextInput::displayText

   This is the text displayed in the TextInput.

   If \l echoMode is set to TextInput::Normal, this holds the
   same value as the TextInput::text property. Otherwise,
   this property holds the text visible to the user, while
   the \l text property holds the actual entered text.
*/
QString QDeclarative1TextInput::displayText() const
{
    Q_D(const QDeclarative1TextInput);
    return d->control->displayText();
}

/*!
    \qmlproperty bool QtQuick1::TextInput::selectByMouse

    Defaults to false.

    If true, the user can use the mouse to select text in some
    platform-specific way. Note that for some platforms this may
    not be an appropriate interaction (eg. may conflict with how
    the text needs to behave inside a Flickable.
*/
bool QDeclarative1TextInput::selectByMouse() const
{
    Q_D(const QDeclarative1TextInput);
    return d->selectByMouse;
}

void QDeclarative1TextInput::setSelectByMouse(bool on)
{
    Q_D(QDeclarative1TextInput);
    if (d->selectByMouse != on) {
        d->selectByMouse = on;
        emit selectByMouseChanged(on);
    }
}

/*!
    \qmlproperty enum QtQuick1::TextInput::mouseSelectionMode
    \since Quick 1.1

    Specifies how text should be selected using a mouse.

    \list
    \o TextInput.SelectCharacters - The selection is updated with individual characters. (Default)
    \o TextInput.SelectWords - The selection is updated with whole words.
    \endlist

    This property only applies when \l selectByMouse is true.
*/

QDeclarative1TextInput::SelectionMode QDeclarative1TextInput::mouseSelectionMode() const
{
    Q_D(const QDeclarative1TextInput);
    return d->mouseSelectionMode;
}

void QDeclarative1TextInput::setMouseSelectionMode(SelectionMode mode)
{
    Q_D(QDeclarative1TextInput);
    if (d->mouseSelectionMode != mode) {
        d->mouseSelectionMode = mode;
        emit mouseSelectionModeChanged(mode);
    }
}

/*!
    \qmlproperty bool QtQuick1::TextInput::canPaste
    \since QtQuick 1.1

    Returns true if the TextInput is writable and the content of the clipboard is
    suitable for pasting into the TextEdit.
*/
bool QDeclarative1TextInput::canPaste() const
{
    Q_D(const QDeclarative1TextInput);
    return d->canPaste;
}

void QDeclarative1TextInput::moveCursorSelection(int position)
{
    Q_D(QDeclarative1TextInput);
    d->control->moveCursor(position, true);
}

/*!
    \qmlmethod void QtQuick1::TextInput::moveCursorSelection(int position, SelectionMode mode = TextInput.SelectCharacters)
    \since Quick 1.1

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
void QDeclarative1TextInput::moveCursorSelection(int pos, SelectionMode mode)
{
    Q_D(QDeclarative1TextInput);

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
    \qmlmethod void QtQuick1::TextInput::openSoftwareInputPanel()

    Opens software input panels like virtual keyboards for typing, useful for
    customizing when you want the input keyboard to be shown and hidden in
    your application.

    By default the opening of input panels follows the platform style.
    The panels are automatically opened when TextInput element gains active focus. Input panels are
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
void QDeclarative1TextInput::openSoftwareInputPanel()
{
    if (qApp) {
        if (QGraphicsView * view = qobject_cast<QGraphicsView*>(qApp->focusWidget())) {
            if (view->scene() && view->scene() == scene()) {
                qApp->inputPanel()->show();
            }
        }
    }
}

/*!
    \qmlmethod void QtQuick1::TextInput::closeSoftwareInputPanel()

    Closes a software input panel like a virtual keyboard shown on the screen, useful
    for customizing when you want the input keyboard to be shown and hidden in
    your application.

    By default the opening of input panels follows the platform style.
    The panels are automatically opened when TextInput element gains active focus. Input panels are
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
void QDeclarative1TextInput::closeSoftwareInputPanel()
{
    if (qApp) {
        if (QGraphicsView * view = qobject_cast<QGraphicsView*>(qApp->focusWidget())) {
            if (view->scene() && view->scene() == scene()) {
                qApp->inputPanel()->hide();
            }
        }
    }
}

void QDeclarative1TextInput::focusInEvent(QFocusEvent *event)
{
    Q_D(const QDeclarative1TextInput);
    if (d->showInputPanelOnFocus) {
        if (d->focusOnPress && !isReadOnly()) {
            openSoftwareInputPanel();
        }
    }
    QDeclarative1PaintedItem::focusInEvent(event);
}

/*!
    \qmlproperty bool QtQuick1::TextInput::inputMethodComposing

    \since QtQuick 1.1

    This property holds whether the TextInput has partial text input from an
    input method.

    While it is composing an input method may rely on mouse or key events from
    the TextInput to edit or commit the partial text.  This property can be
    used to determine when to disable events handlers that may interfere with
    the correct operation of an input method.
*/
bool QDeclarative1TextInput::isInputMethodComposing() const
{
    Q_D(const QDeclarative1TextInput);
    return d->control->preeditAreaText().length() > 0;
}

void QDeclarative1TextInputPrivate::init()
{
    Q_Q(QDeclarative1TextInput);
    control->setParent(q);//Now mandatory due to accessibility changes
    control->setCursorWidth(1);
    control->setPasswordCharacter(QLatin1Char('*'));
    q->setSmooth(smooth);
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFlag(QGraphicsItem::ItemHasNoContents, false);
    q->setFlag(QGraphicsItem::ItemAcceptsInputMethod);
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
    q->connect(QApplication::clipboard(), SIGNAL(dataChanged()),
            q, SLOT(q_canPasteChanged()));
    canPaste = !control->isReadOnly() && QApplication::clipboard()->text().length() != 0;
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
}

void QDeclarative1TextInput::cursorPosChanged()
{
    Q_D(QDeclarative1TextInput);
    updateCursorRectangle();
    emit cursorPositionChanged();
    d->control->resetCursorBlinkTimer();

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

void QDeclarative1TextInput::updateCursorRectangle()
{
    Q_D(QDeclarative1TextInput);
    d->determineHorizontalAlignment();
    d->updateHorizontalScroll();
    updateRect();//TODO: Only update rect between pos's
    updateMicroFocus();
    emit cursorRectangleChanged();
    if (d->cursorItem)
        d->cursorItem->setX(d->control->cursorToX() - d->hscroll);
}

void QDeclarative1TextInput::selectionChanged()
{
    Q_D(QDeclarative1TextInput);
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

void QDeclarative1TextInput::q_textChanged()
{
    Q_D(QDeclarative1TextInput);
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

void QDeclarative1TextInput::updateRect(const QRect &r)
{
    Q_D(QDeclarative1TextInput);
    if(r == QRect())
        clearCache();
    else
        dirtyCache(QRect(r.x() - d->hscroll, r.y(), r.width(), r.height()));
    update();
}

QRectF QDeclarative1TextInput::boundingRect() const
{
    Q_D(const QDeclarative1TextInput);
    QRectF r = QDeclarative1PaintedItem::boundingRect();

    int cursorWidth = d->cursorItem ? d->cursorItem->width() : d->control->cursorWidth();

    // Could include font max left/right bearings to either side of rectangle.

    r.setRight(r.right() + cursorWidth);
    return r;
}

void QDeclarative1TextInput::updateSize(bool needsRedraw)
{
    Q_D(QDeclarative1TextInput);
    int w = width();
    int h = height();
    setImplicitHeight(d->control->height()-1); // -1 to counter QWidgetLineControl's +1 which is not consistent with Text.
    setImplicitWidth(d->calculateTextWidth());
    setContentsSize(QSize(width(), height()));//Repaints if changed
    if(w==width() && h==height() && needsRedraw){
        clearCache();
        update();
    }
}

void QDeclarative1TextInput::q_canPasteChanged()
{
    Q_D(QDeclarative1TextInput);
    bool old = d->canPaste;
#ifndef QT_NO_CLIPBOARD
    d->canPaste = !d->control->isReadOnly() && QApplication::clipboard()->text().length() != 0;
#endif
    if(d->canPaste != old)
        emit canPasteChanged();
}



QT_END_NAMESPACE

#endif // QT_NO_LINEEDIT

