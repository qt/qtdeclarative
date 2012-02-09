/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktextinput_p.h"
#include "qquicktextinput_p_p.h"
#include "qquickcanvas.h"

#include <private/qdeclarativeglobal_p.h>

#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtGui/qevent.h>
#include <QTextBoundaryFinder>
#include "qquicktextnode_p.h"
#include <QtQuick/qsgsimplerectnode.h>

#include <QtGui/qstylehints.h>
#include <QtGui/qinputpanel.h>

#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)

#ifdef QT_GUI_PASSWORD_ECHO_DELAY
static const int qt_passwordEchoDelay = QT_GUI_PASSWORD_ECHO_DELAY;
#endif

/*!
    \qmlclass TextInput QQuickTextInput
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
QQuickTextInput::QQuickTextInput(QQuickItem* parent)
: QQuickImplicitSizeItem(*(new QQuickTextInputPrivate), parent)
{
    Q_D(QQuickTextInput);
    d->init();
}

QQuickTextInput::~QQuickTextInput()
{
}

void QQuickTextInput::componentComplete()
{
    Q_D(QQuickTextInput);

    QQuickImplicitSizeItem::componentComplete();

    d->checkIsValid();
    d->updateLayout();
    updateCursorRectangle();
    if (d->cursorComponent && d->cursorComponent->isReady())
        createCursor();
}

/*!
    \qmlproperty string QtQuick2::TextInput::text

    The text in the TextInput.
*/
QString QQuickTextInput::text() const
{
    Q_D(const QQuickTextInput);

    QString content = d->m_text;
    if (!d->m_tentativeCommit.isEmpty())
        content.insert(d->m_cursor, d->m_tentativeCommit);
    QString res = d->m_maskData ? d->stripString(content) : content;
    return (res.isNull() ? QString::fromLatin1("") : res);
}

void QQuickTextInput::setText(const QString &s)
{
    Q_D(QQuickTextInput);
    if (s == text())
        return;
    if (d->composeMode())
        qApp->inputPanel()->reset();
    d->m_tentativeCommit.clear();
    d->internalSetText(s, -1, false);
}

/*!
    \qmlproperty int QtQuick2::TextInput::length

    Returns the total number of characters in the TextInput item.

    If the TextInput has an inputMask the length will include mask characters and may differ
    from the length of the string returned by the \l text property.

    This property can be faster than querying the length the \l text property as it doesn't
    require any copying or conversion of the TextInput's internal string data.
*/

int QQuickTextInput::length() const
{
    Q_D(const QQuickTextInput);
    return d->m_text.length();
}

/*!
    \qmlmethod string QtQuick2::TextInput::getText(int start, int end)

    Returns the section of text that is between the \a start and \a end positions.

    If the TextInput has an inputMask the length will include mask characters.
*/

QString QQuickTextInput::getText(int start, int end) const
{
    Q_D(const QQuickTextInput);

    if (start > end)
        qSwap(start, end);

    return d->m_text.mid(start, end - start);
}

QString QQuickTextInputPrivate::realText() const
{
    QString res = m_maskData ? stripString(m_text) : m_text;
    return (res.isNull() ? QString::fromLatin1("") : res);
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
    \o Font.AllLowercase - This alters the text to be rendered in all lowercase type.
    \o Font.SmallCaps - This alters the text to be rendered in small-caps type.
    \o Font.Capitalize - This alters the text to be rendered with the first character of each word as an uppercase character.
    \endlist

    \qml
    TextInput { text: "Hello"; font.capitalization: Font.AllLowercase }
    \endqml
*/

QFont QQuickTextInput::font() const
{
    Q_D(const QQuickTextInput);
    return d->sourceFont;
}

void QQuickTextInput::setFont(const QFont &font)
{
    Q_D(QQuickTextInput);
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
        d->updateLayout();
        updateCursorRectangle();
        updateInputMethod(Qt::ImCursorRectangle | Qt::ImFont);
    }
    emit fontChanged(d->sourceFont);
}

/*!
    \qmlproperty color QtQuick2::TextInput::color

    The text color.
*/
QColor QQuickTextInput::color() const
{
    Q_D(const QQuickTextInput);
    return d->color;
}

void QQuickTextInput::setColor(const QColor &c)
{
    Q_D(QQuickTextInput);
    if (c != d->color) {
        d->color = c;
        d->textLayoutDirty = true;
        d->updateType = QQuickTextInputPrivate::UpdatePaintNode;
        update();
        emit colorChanged(c);
    }
}


/*!
    \qmlproperty color QtQuick2::TextInput::selectionColor

    The text highlight color, used behind selections.
*/
QColor QQuickTextInput::selectionColor() const
{
    Q_D(const QQuickTextInput);
    return d->selectionColor;
}

void QQuickTextInput::setSelectionColor(const QColor &color)
{
    Q_D(QQuickTextInput);
    if (d->selectionColor == color)
        return;

    d->selectionColor = color;
    d->m_palette.setColor(QPalette::Highlight, d->selectionColor);
    if (d->hasSelectedText()) {
        d->textLayoutDirty = true;
        d->updateType = QQuickTextInputPrivate::UpdatePaintNode;
        update();
    }
    emit selectionColorChanged(color);
}
/*!
    \qmlproperty color QtQuick2::TextInput::selectedTextColor

    The highlighted text color, used in selections.
*/
QColor QQuickTextInput::selectedTextColor() const
{
    Q_D(const QQuickTextInput);
    return d->selectedTextColor;
}

void QQuickTextInput::setSelectedTextColor(const QColor &color)
{
    Q_D(QQuickTextInput);
    if (d->selectedTextColor == color)
        return;

    d->selectedTextColor = color;
    d->m_palette.setColor(QPalette::HighlightedText, d->selectedTextColor);
    if (d->hasSelectedText()) {
        d->textLayoutDirty = true;
        d->updateType = QQuickTextInputPrivate::UpdatePaintNode;
        update();
    }
    emit selectedTextColorChanged(color);
}

/*!
    \qmlproperty enumeration QtQuick2::TextInput::horizontalAlignment
    \qmlproperty enumeration QtQuick2::TextInput::effectiveHorizontalAlignment
    \qmlproperty enumeration QtQuick2::TextInput::verticalAlignment

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

    Valid values for \c verticalAlignment are \c TextInput.AlignTop (default),
    \c TextInput.AlignBottom \c TextInput.AlignVCenter.

    When using the attached property LayoutMirroring::enabled to mirror application
    layouts, the horizontal alignment of text will also be mirrored. However, the property
    \c horizontalAlignment will remain unchanged. To query the effective horizontal alignment
    of TextInput, use the read-only property \c effectiveHorizontalAlignment.
*/
QQuickTextInput::HAlignment QQuickTextInput::hAlign() const
{
    Q_D(const QQuickTextInput);
    return d->hAlign;
}

void QQuickTextInput::setHAlign(HAlignment align)
{
    Q_D(QQuickTextInput);
    bool forceAlign = d->hAlignImplicit && d->effectiveLayoutMirror;
    d->hAlignImplicit = false;
    if (d->setHAlign(align, forceAlign) && isComponentComplete()) {
        d->updateLayout();
        updateCursorRectangle();
    }
}

void QQuickTextInput::resetHAlign()
{
    Q_D(QQuickTextInput);
    d->hAlignImplicit = true;
    if (d->determineHorizontalAlignment() && isComponentComplete()) {
        d->updateLayout();
        updateCursorRectangle();
    }
}

QQuickTextInput::HAlignment QQuickTextInput::effectiveHAlign() const
{
    Q_D(const QQuickTextInput);
    QQuickTextInput::HAlignment effectiveAlignment = d->hAlign;
    if (!d->hAlignImplicit && d->effectiveLayoutMirror) {
        switch (d->hAlign) {
        case QQuickTextInput::AlignLeft:
            effectiveAlignment = QQuickTextInput::AlignRight;
            break;
        case QQuickTextInput::AlignRight:
            effectiveAlignment = QQuickTextInput::AlignLeft;
            break;
        default:
            break;
        }
    }
    return effectiveAlignment;
}

bool QQuickTextInputPrivate::setHAlign(QQuickTextInput::HAlignment alignment, bool forceAlign)
{
    Q_Q(QQuickTextInput);
    if ((hAlign != alignment || forceAlign) && alignment <= QQuickTextInput::AlignHCenter) { // justify not supported
        QQuickTextInput::HAlignment oldEffectiveHAlign = q->effectiveHAlign();
        hAlign = alignment;
        emit q->horizontalAlignmentChanged(alignment);
        if (oldEffectiveHAlign != q->effectiveHAlign())
            emit q->effectiveHorizontalAlignmentChanged();
        return true;
    }
    return false;
}

bool QQuickTextInputPrivate::determineHorizontalAlignment()
{
    if (hAlignImplicit) {
        // if no explicit alignment has been set, follow the natural layout direction of the text
        QString text = q_func()->text();
        if (text.isEmpty())
            text = m_textLayout.preeditAreaText();
        bool isRightToLeft = text.isEmpty() ? qApp->inputPanel()->inputDirection() == Qt::RightToLeft
                                            : text.isRightToLeft();
        return setHAlign(isRightToLeft ? QQuickTextInput::AlignRight : QQuickTextInput::AlignLeft);
    }
    return false;
}

QQuickTextInput::VAlignment QQuickTextInput::vAlign() const
{
    Q_D(const QQuickTextInput);
    return d->vAlign;
}

void QQuickTextInput::setVAlign(QQuickTextInput::VAlignment alignment)
{
    Q_D(QQuickTextInput);
    if (alignment == d->vAlign)
        return;
    d->vAlign = alignment;
    emit verticalAlignmentChanged(d->vAlign);
    if (isComponentComplete()) {
        updateCursorRectangle();
    }
}

/*!
    \qmlproperty enumeration QtQuick2::TextInput::wrapMode

    Set this property to wrap the text to the TextInput item's width.
    The text will only wrap if an explicit width has been set.

    \list
    \o TextInput.NoWrap - no wrapping will be performed. If the text contains insufficient newlines, then implicitWidth will exceed a set width.
    \o TextInput.WordWrap - wrapping is done on word boundaries only. If a word is too long, implicitWidth will exceed a set width.
    \o TextInput.WrapAnywhere - wrapping is done at any point on a line, even if it occurs in the middle of a word.
    \o TextInput.Wrap - if possible, wrapping occurs at a word boundary; otherwise it will occur at the appropriate point on the line, even in the middle of a word.
    \endlist

    The default is TextInput.NoWrap. If you set a width, consider using TextInput.Wrap.
*/
QQuickTextInput::WrapMode QQuickTextInput::wrapMode() const
{
    Q_D(const QQuickTextInput);
    return d->wrapMode;
}

void QQuickTextInput::setWrapMode(WrapMode mode)
{
    Q_D(QQuickTextInput);
    if (mode == d->wrapMode)
        return;
    d->wrapMode = mode;
    d->updateLayout();
    updateCursorRectangle();
    emit wrapModeChanged();
}

void QQuickTextInputPrivate::mirrorChange()
{
    Q_Q(QQuickTextInput);
    if (q->isComponentComplete()) {
        if (!hAlignImplicit && (hAlign == QQuickTextInput::AlignRight || hAlign == QQuickTextInput::AlignLeft)) {
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
bool QQuickTextInput::isReadOnly() const
{
    Q_D(const QQuickTextInput);
    return d->m_readOnly;
}

void QQuickTextInput::setReadOnly(bool ro)
{
    Q_D(QQuickTextInput);
    if (d->m_readOnly == ro)
        return;

    setFlag(QQuickItem::ItemAcceptsInputMethod, !ro);
    d->m_readOnly = ro;
    if (!ro)
        d->setCursorPosition(d->end());
    updateInputMethod(Qt::ImEnabled);
    q_canPasteChanged();
    d->emitUndoRedoChanged();
    emit readOnlyChanged(ro);
}

/*!
    \qmlproperty int QtQuick2::TextInput::maximumLength
    The maximum permitted length of the text in the TextInput.

    If the text is too long, it is truncated at the limit.

    By default, this property contains a value of 32767.
*/
int QQuickTextInput::maxLength() const
{
    Q_D(const QQuickTextInput);
    return d->m_maxLength;
}

void QQuickTextInput::setMaxLength(int ml)
{
    Q_D(QQuickTextInput);
    if (d->m_maxLength == ml || d->m_maskData)
        return;

    d->m_maxLength = ml;
    d->internalSetText(d->m_text, -1, false);

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
bool QQuickTextInput::isCursorVisible() const
{
    Q_D(const QQuickTextInput);
    return d->cursorVisible;
}

void QQuickTextInput::setCursorVisible(bool on)
{
    Q_D(QQuickTextInput);
    if (d->cursorVisible == on)
        return;
    d->cursorVisible = on;
    d->setCursorBlinkPeriod(on ? qApp->styleHints()->cursorFlashTime() : 0);
    d->updateType = QQuickTextInputPrivate::UpdatePaintNode;
    update();
    emit cursorVisibleChanged(d->cursorVisible);
}

/*!
    \qmlproperty int QtQuick2::TextInput::cursorPosition
    The position of the cursor in the TextInput.
*/
int QQuickTextInput::cursorPosition() const
{
    Q_D(const QQuickTextInput);
    return d->m_cursor;
}

void QQuickTextInput::setCursorPosition(int cp)
{
    Q_D(QQuickTextInput);
    if (cp < 0 || cp > text().length())
        return;
    d->moveCursor(cp);
}

/*!
    \qmlproperty rectangle QtQuick2::TextInput::cursorRectangle

    The rectangle where the standard text cursor is rendered within the text input.  Read only.

    The position and height of a custom cursorDelegate are updated to follow the cursorRectangle
    automatically when it changes.  The width of the delegate is unaffected by changes in the
    cursor rectangle.
*/

QRect QQuickTextInput::cursorRectangle() const
{
    Q_D(const QQuickTextInput);

    int c = d->m_cursor;
    if (d->m_preeditCursor != -1)
        c += d->m_preeditCursor;
    if (d->m_echoMode == NoEcho)
        c = 0;
    QTextLine l = d->m_textLayout.lineForTextPosition(c);
    if (!l.isValid())
        return QRect();
    return QRect(
            qRound(l.cursorToX(c) - d->hscroll),
            qRound(l.y() - d->vscroll),
            d->m_cursorWidth,
            qCeil(l.height()));
}

/*!
    \qmlproperty int QtQuick2::TextInput::selectionStart

    The cursor position before the first character in the current selection.

    This property is read-only. To change the selection, use select(start,end),
    selectAll(), or selectWord().

    \sa selectionEnd, cursorPosition, selectedText
*/
int QQuickTextInput::selectionStart() const
{
    Q_D(const QQuickTextInput);
    return d->lastSelectionStart;
}
/*!
    \qmlproperty int QtQuick2::TextInput::selectionEnd

    The cursor position after the last character in the current selection.

    This property is read-only. To change the selection, use select(start,end),
    selectAll(), or selectWord().

    \sa selectionStart, cursorPosition, selectedText
*/
int QQuickTextInput::selectionEnd() const
{
    Q_D(const QQuickTextInput);
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
void QQuickTextInput::select(int start, int end)
{
    Q_D(QQuickTextInput);
    if (start < 0 || end < 0 || start > d->m_text.length() || end > d->m_text.length())
        return;
    d->setSelection(start, end-start);
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
QString QQuickTextInput::selectedText() const
{
    Q_D(const QQuickTextInput);
    return d->selectedText();
}

/*!
    \qmlproperty bool QtQuick2::TextInput::activeFocusOnPress

    Whether the TextInput should gain active focus on a mouse press. By default this is
    set to true.
*/
bool QQuickTextInput::focusOnPress() const
{
    Q_D(const QQuickTextInput);
    return d->focusOnPress;
}

void QQuickTextInput::setFocusOnPress(bool b)
{
    Q_D(QQuickTextInput);
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
bool QQuickTextInput::autoScroll() const
{
    Q_D(const QQuickTextInput);
    return d->autoScroll;
}

void QQuickTextInput::setAutoScroll(bool b)
{
    Q_D(QQuickTextInput);
    if (d->autoScroll == b)
        return;

    d->autoScroll = b;
    //We need to repaint so that the scrolling is taking into account.
    updateCursorRectangle();
    emit autoScrollChanged(d->autoScroll);
}

#ifndef QT_NO_VALIDATOR

/*!
    \qmlclass IntValidator QIntValidator
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-visual-elements

    This element provides a validator for integer values.

    If no \l locale is set IntValidator uses the \l {QLocale::setDefault()}{default locale} to
    interpret the number and will accept locale specific digits, group separators, and positive
    and negative signs.  In addition, IntValidator is always guaranteed to accept a number
    formatted according to the "C" locale.
*/


QQuickIntValidator::QQuickIntValidator(QObject *parent)
    : QIntValidator(parent)
{
}

/*!
    \qmlproperty string QtQuick2::IntValidator::locale

    This property holds the name of the locale used to interpret the number.

    \sa QML:Qt::locale()
*/

QString QQuickIntValidator::localeName() const
{
    return locale().name();
}

void QQuickIntValidator::setLocaleName(const QString &name)
{
    if (locale().name() != name) {
        setLocale(QLocale(name));
        emit localeNameChanged();
    }
}

void QQuickIntValidator::resetLocaleName()
{
    QLocale defaultLocale;
    if (locale() != defaultLocale) {
        setLocale(defaultLocale);
        emit localeNameChanged();
    }
}

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

    Input is accepted if it contains a double that is within the valid range
    and is in the  correct format.

    Input is accepected but invalid if it contains a double that is outside
    the range or is in the wrong format; e.g. with too many digits after the
    decimal point or is empty.

    Input is rejected if it is not a double.

    Note: If the valid range consists of just positive doubles (e.g. 0.0 to
    100.0) and input is a negative double then it is rejected. If \l notation
    is set to DoubleValidator.StandardNotation, and  the input contains more
    digits before the decimal point than a double in the valid range may have,
    it is also rejected. If \l notation is DoubleValidator.ScientificNotation,
    and the input is not in the valid range, it is accecpted but invalid. The
    value may yet become valid by changing the exponent.
*/

QQuickDoubleValidator::QQuickDoubleValidator(QObject *parent)
    : QDoubleValidator(parent)
{
}

/*!
    \qmlproperty string QtQuick2::DoubleValidator::locale

    This property holds the name of the locale used to interpret the number.

    \sa QML:Qt::locale()
*/

QString QQuickDoubleValidator::localeName() const
{
    return locale().name();
}

void QQuickDoubleValidator::setLocaleName(const QString &name)
{
    if (locale().name() != name) {
        setLocale(QLocale(name));
        emit localeNameChanged();
    }
}

void QQuickDoubleValidator::resetLocaleName()
{
    QLocale defaultLocale;
    if (locale() != defaultLocale) {
        setLocale(defaultLocale);
        emit localeNameChanged();
    }
}

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
    import QtQuick 2.0
    TextInput{
        validator: IntValidator{bottom: 11; top: 31;}
        focus: true
    }
    \endcode

    \sa acceptableInput, inputMask
*/

QValidator* QQuickTextInput::validator() const
{
    Q_D(const QQuickTextInput);
    return d->m_validator;
}

void QQuickTextInput::setValidator(QValidator* v)
{
    Q_D(QQuickTextInput);
    if (d->m_validator == v)
        return;

    d->m_validator = v;

    if (isComponentComplete())
        d->checkIsValid();

    emit validatorChanged();
}

#endif // QT_NO_VALIDATOR

void QQuickTextInputPrivate::checkIsValid()
{
    Q_Q(QQuickTextInput);

    ValidatorState state = hasAcceptableInput(m_text);
    m_validInput = state != InvalidInput;
    if (state != AcceptableInput) {
        if (m_acceptableInput) {
            m_acceptableInput = false;
            emit q->acceptableInputChanged();
        }
    } else if (!m_acceptableInput) {
        m_acceptableInput = true;
        emit q->acceptableInputChanged();
    }
}

/*!
    \qmlproperty string QtQuick2::TextInput::inputMask

    Allows you to set an input mask on the TextInput, restricting the allowable
    text inputs. See QLineEdit::inputMask for further details, as the exact
    same mask strings are used by TextInput.

    \sa acceptableInput, validator
*/
QString QQuickTextInput::inputMask() const
{
    Q_D(const QQuickTextInput);
    return d->inputMask();
}

void QQuickTextInput::setInputMask(const QString &im)
{
    Q_D(QQuickTextInput);
    if (d->inputMask() == im)
        return;

    d->setInputMask(im);
    emit inputMaskChanged(d->inputMask());
}

/*!
    \qmlproperty bool QtQuick2::TextInput::acceptableInput

    This property is always true unless a validator or input mask has been set.
    If a validator or input mask has been set, this property will only be true
    if the current text is acceptable to the validator or input mask as a final
    string (not as an intermediate string).
*/
bool QQuickTextInput::hasAcceptableInput() const
{
    Q_D(const QQuickTextInput);
    return d->hasAcceptableInput(d->m_text) == QQuickTextInputPrivate::AcceptableInput;
}

/*!
    \qmlsignal QtQuick2::TextInput::onAccepted()

    This handler is called when the Return or Enter key is pressed.
    Note that if there is a \l validator or \l inputMask set on the text
    input, the handler will only be emitted if the input is in an acceptable
    state.
*/

void QQuickTextInputPrivate::updateInputMethodHints()
{
    Q_Q(QQuickTextInput);
    Qt::InputMethodHints hints = inputMethodHints;
    if (m_echoMode == QQuickTextInput::Password || m_echoMode == QQuickTextInput::NoEcho)
        hints |= Qt::ImhHiddenText;
    else if (m_echoMode == QQuickTextInput::PasswordEchoOnEdit)
        hints &= ~Qt::ImhHiddenText;
    if (m_echoMode != QQuickTextInput::Normal)
        hints |= (Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText | Qt::ImhSensitiveData);
    q->setInputMethodHints(hints);
}
/*!
    \qmlproperty enumeration QtQuick2::TextInput::echoMode

    Specifies how the text should be displayed in the TextInput.
    \list
    \o TextInput.Normal - Displays the text as it is. (Default)
    \o TextInput.Password - Displays asterisks instead of characters.
    \o TextInput.NoEcho - Displays nothing.
    \o TextInput.PasswordEchoOnEdit - Displays characters as they are entered
    while editing, otherwise displays asterisks.
    \endlist
*/
QQuickTextInput::EchoMode QQuickTextInput::echoMode() const
{
    Q_D(const QQuickTextInput);
    return QQuickTextInput::EchoMode(d->m_echoMode);
}

void QQuickTextInput::setEchoMode(QQuickTextInput::EchoMode echo)
{
    Q_D(QQuickTextInput);
    if (echoMode() == echo)
        return;
    d->cancelPasswordEchoTimer();
    d->m_echoMode = echo;
    d->m_passwordEchoEditing = false;
    d->updateInputMethodHints();
    d->updateDisplayText();
    updateCursorRectangle();

    emit echoModeChanged(echoMode());
}

/*!
    \qmlproperty enumeration QtQuick2::TextInput::inputMethodHints

    Provides hints to the input method about the expected content of the text input and how it
    should operate.

    The value is a bit-wise combination of flags, or Qt.ImhNone if no hints are set.

    Flags that alter behaviour are:

    \list
    \o Qt.ImhHiddenText - Characters should be hidden, as is typically used when entering passwords.
            This is automatically set when setting echoMode to \c TextInput.Password.
    \o Qt.ImhSensitiveData - Typed text should not be stored by the active input method
            in any persistent storage like predictive user dictionary.
    \o Qt.ImhNoAutoUppercase - The input method should not try to automatically switch to upper case
            when a sentence ends.
    \o Qt.ImhPreferNumbers - Numbers are preferred (but not required).
    \o Qt.ImhPreferUppercase - Upper case letters are preferred (but not required).
    \o Qt.ImhPreferLowercase - Lower case letters are preferred (but not required).
    \o Qt.ImhNoPredictiveText - Do not use predictive text (i.e. dictionary lookup) while typing.

    \o Qt.ImhDate - The text editor functions as a date field.
    \o Qt.ImhTime - The text editor functions as a time field.
    \endlist

    Flags that restrict input (exclusive flags) are:

    \list
    \o Qt.ImhDigitsOnly - Only digits are allowed.
    \o Qt.ImhFormattedNumbersOnly - Only number input is allowed. This includes decimal point and minus sign.
    \o Qt.ImhUppercaseOnly - Only upper case letter input is allowed.
    \o Qt.ImhLowercaseOnly - Only lower case letter input is allowed.
    \o Qt.ImhDialableCharactersOnly - Only characters suitable for phone dialing are allowed.
    \o Qt.ImhEmailCharactersOnly - Only characters suitable for email addresses are allowed.
    \o Qt.ImhUrlCharactersOnly - Only characters suitable for URLs are allowed.
    \endlist

    Masks:

    \list
    \o Qt.ImhExclusiveInputMask - This mask yields nonzero if any of the exclusive flags are used.
    \endlist
*/

Qt::InputMethodHints QQuickTextInput::imHints() const
{
    Q_D(const QQuickTextInput);
    return d->inputMethodHints;
}

void QQuickTextInput::setIMHints(Qt::InputMethodHints hints)
{
    Q_D(QQuickTextInput);
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
QDeclarativeComponent* QQuickTextInput::cursorDelegate() const
{
    Q_D(const QQuickTextInput);
    return d->cursorComponent;
}

void QQuickTextInput::setCursorDelegate(QDeclarativeComponent* c)
{
    Q_D(QQuickTextInput);
    if (d->cursorComponent == c)
        return;

    d->cursorComponent = c;
    if (!c) {
        //note that the components are owned by something else
        delete d->cursorItem;
    } else {
        d->startCreatingCursor();
    }

    emit cursorDelegateChanged();
}

void QQuickTextInputPrivate::startCreatingCursor()
{
    Q_Q(QQuickTextInput);
    if (cursorComponent->isReady()) {
        q->createCursor();
    } else if (cursorComponent->isLoading()) {
        q->connect(cursorComponent, SIGNAL(statusChanged(int)),
                q, SLOT(createCursor()));
    } else { // isError
        qmlInfo(q, cursorComponent->errors()) << QQuickTextInput::tr("Could not load cursor delegate");
    }
}

void QQuickTextInput::createCursor()
{
    Q_D(QQuickTextInput);
    if (!isComponentComplete())
        return;

    if (d->cursorComponent->isError()) {
        qmlInfo(this, d->cursorComponent->errors()) << tr("Could not load cursor delegate");
        return;
    }

    if (!d->cursorComponent->isReady())
        return;

    if (d->cursorItem)
        delete d->cursorItem;
    QDeclarativeContext *creationContext = d->cursorComponent->creationContext();
    QObject *object = d->cursorComponent->create(creationContext ? creationContext : qmlContext(this));
    d->cursorItem = qobject_cast<QQuickItem*>(object);
    if (!d->cursorItem) {
        delete object;
        qmlInfo(this, d->cursorComponent->errors()) << tr("Could not instantiate cursor delegate");
        return;
    }

    QRectF r = cursorRectangle();

    QDeclarative_setParent_noEvent(d->cursorItem, this);
    d->cursorItem->setParentItem(this);
    d->cursorItem->setPos(r.topLeft());
    d->cursorItem->setHeight(r.height());
}

/*!
    \qmlmethod rect QtQuick2::TextInput::positionToRectangle(int pos)

    This function takes a character position and returns the rectangle that the
    cursor would occupy, if it was placed at that character position.

    This is similar to setting the cursorPosition, and then querying the cursor
    rectangle, but the cursorPosition is not changed.
*/
QRectF QQuickTextInput::positionToRectangle(int pos) const
{
    Q_D(const QQuickTextInput);
    if (d->m_echoMode == NoEcho)
        pos = 0;
    else if (pos > d->m_cursor)
        pos += d->preeditAreaText().length();
    QTextLine l = d->m_textLayout.lineForTextPosition(pos);
    return l.isValid()
            ? QRectF(l.cursorToX(pos) - d->hscroll, l.y() - d->vscroll, d->m_cursorWidth, l.height())
            : QRectF();
}

/*!
    \qmlmethod int QtQuick2::TextInput::positionAt(real x, real y, CursorPosition position = CursorBetweenCharacters)

    This function returns the character position at
    x and y pixels from the top left  of the textInput. Position 0 is before the
    first character, position 1 is after the first character but before the second,
    and so on until position text.length, which is after all characters.

    This means that for all x values before the first character this function returns 0,
    and for all x values after the last character this function returns text.length.  If
    the y value is above the text the position will be that of the nearest character on
    the first line line and if it is below the text the position of the nearest character
    on the last line will be returned.

    The cursor position type specifies how the cursor position should be resolved.

    \list
    \o TextInput.CursorBetweenCharacters - Returns the position between characters that is nearest x.
    \o TextInput.CursorOnCharacter - Returns the position before the character that is nearest x.
    \endlist
*/

void QQuickTextInput::positionAt(QDeclarativeV8Function *args) const
{
    Q_D(const QQuickTextInput);

    qreal x = 0;
    qreal y = 0;
    QTextLine::CursorPosition position = QTextLine::CursorBetweenCharacters;

    if (args->Length() < 1)
        return;

    int i = 0;
    v8::Local<v8::Value> arg = (*args)[i];
    x = arg->NumberValue();

    if (++i < args->Length()) {
        arg = (*args)[i];
        y = arg->NumberValue();
    }

    if (++i < args->Length()) {
        arg = (*args)[i];
        position = QTextLine::CursorPosition(arg->Int32Value());
    }

    int pos = d->positionAt(x, y, position);
    const int cursor = d->m_cursor;
    if (pos > cursor) {
        const int preeditLength = d->preeditAreaText().length();
        pos = pos > cursor + preeditLength
                ? pos - preeditLength
                : cursor;
    }
    args->returnValue(v8::Int32::New(pos));
}

int QQuickTextInputPrivate::positionAt(int x, int y, QTextLine::CursorPosition position) const
{
    x += hscroll;
    y += vscroll;
    QTextLine line = m_textLayout.lineAt(0);
    for (int i = 1; i < m_textLayout.lineCount(); ++i) {
        QTextLine nextLine = m_textLayout.lineAt(i);

        if (y < (line.rect().bottom() + nextLine.y()) / 2)
            break;
        line = nextLine;
    }
    return line.isValid() ? line.xToCursor(x, position) : 0;
}

void QQuickTextInput::keyPressEvent(QKeyEvent* ev)
{
    Q_D(QQuickTextInput);
    // Don't allow MacOSX up/down support, and we don't allow a completer.
    bool ignore = (ev->key() == Qt::Key_Up || ev->key() == Qt::Key_Down) && ev->modifiers() == Qt::NoModifier;
    if (!ignore && (d->lastSelectionStart == d->lastSelectionEnd) && (ev->key() == Qt::Key_Right || ev->key() == Qt::Key_Left)) {
        // Ignore when moving off the end unless there is a selection,
        // because then moving will do something (deselect).
        int cursorPosition = d->m_cursor;
        if (cursorPosition == 0)
            ignore = ev->key() == (d->layoutDirection() == Qt::LeftToRight ? Qt::Key_Left : Qt::Key_Right);
        if (cursorPosition == text().length())
            ignore = ev->key() == (d->layoutDirection() == Qt::LeftToRight ? Qt::Key_Right : Qt::Key_Left);
    }
    if (ignore) {
        ev->ignore();
    } else {
        d->processKeyEvent(ev);
    }
    if (!ev->isAccepted())
        QQuickImplicitSizeItem::keyPressEvent(ev);
}

void QQuickTextInput::inputMethodEvent(QInputMethodEvent *ev)
{
    Q_D(QQuickTextInput);
    const bool wasComposing = d->preeditAreaText().length() > 0;
    if (d->m_readOnly) {
        ev->ignore();
    } else {
        d->processInputMethodEvent(ev);
    }
    if (!ev->isAccepted())
        QQuickImplicitSizeItem::inputMethodEvent(ev);

    if (wasComposing != (d->m_textLayout.preeditAreaText().length() > 0))
        emit inputMethodComposingChanged();
}

void QQuickTextInput::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QQuickTextInput);

    if (d->selectByMouse && event->button() == Qt::LeftButton) {
        d->commitPreedit();
        int cursor = d->positionAt(event->localPos());
        d->selectWordAtPos(cursor);
        event->setAccepted(true);
        if (!d->hasPendingTripleClick()) {
            d->tripleClickStartPoint = event->localPos().toPoint();
            d->tripleClickTimer.start();
        }
    } else {
        if (d->sendMouseEventToInputContext(event))
            return;
        QQuickImplicitSizeItem::mouseDoubleClickEvent(event);
    }
}

void QQuickTextInput::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickTextInput);

    d->pressPos = event->localPos();

    if (d->focusOnPress) {
        bool hadActiveFocus = hasActiveFocus();
        forceActiveFocus();
        // re-open input panel on press if already focused
        if (hasActiveFocus() && hadActiveFocus && !d->m_readOnly)
            openSoftwareInputPanel();
    }
    if (d->selectByMouse) {
        setKeepMouseGrab(false);
        d->selectPressed = true;
        QPoint distanceVector = d->pressPos.toPoint() - d->tripleClickStartPoint;
        if (d->hasPendingTripleClick()
            && distanceVector.manhattanLength() < qApp->styleHints()->startDragDistance()) {
            event->setAccepted(true);
            selectAll();
            return;
        }
    }

    if (d->sendMouseEventToInputContext(event))
        return;

    bool mark = (event->modifiers() & Qt::ShiftModifier) && d->selectByMouse;
    int cursor = d->positionAt(event->localPos());
    d->moveCursor(cursor, mark);
    event->setAccepted(true);
}

void QQuickTextInput::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickTextInput);

    if (d->selectPressed) {
        if (qAbs(int(event->localPos().x() - d->pressPos.x())) > qApp->styleHints()->startDragDistance())
            setKeepMouseGrab(true);

        if (d->composeMode()) {
            // start selection
            int startPos = d->positionAt(d->pressPos);
            int currentPos = d->positionAt(event->localPos());
            if (startPos != currentPos)
                d->setSelection(startPos, currentPos - startPos);
        } else {
            moveCursorSelection(d->positionAt(event->localPos()), d->mouseSelectionMode);
        }
        event->setAccepted(true);
    } else {
        QQuickImplicitSizeItem::mouseMoveEvent(event);
    }
}

void QQuickTextInput::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickTextInput);
    if (d->sendMouseEventToInputContext(event))
        return;
    if (d->selectPressed) {
        d->selectPressed = false;
        setKeepMouseGrab(false);
    }
#ifndef QT_NO_CLIPBOARD
    if (QGuiApplication::clipboard()->supportsSelection()) {
        if (event->button() == Qt::LeftButton) {
            d->copy(QClipboard::Selection);
        } else if (!d->m_readOnly && event->button() == Qt::MidButton) {
            d->deselect();
            d->insert(QGuiApplication::clipboard()->text(QClipboard::Selection));
        }
    }
#endif
    if (!event->isAccepted())
        QQuickImplicitSizeItem::mouseReleaseEvent(event);
}

bool QQuickTextInputPrivate::sendMouseEventToInputContext(QMouseEvent *event)
{
#if !defined QT_NO_IM
    if (composeMode()) {
        int tmp_cursor = positionAt(event->localPos());
        int mousePos = tmp_cursor - m_cursor;
        if (mousePos >= 0 && mousePos <= m_textLayout.preeditAreaText().length()) {
            if (event->type() == QEvent::MouseButtonRelease) {
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

void QQuickTextInput::mouseUngrabEvent()
{
    Q_D(QQuickTextInput);
    d->selectPressed = false;
    setKeepMouseGrab(false);
}

bool QQuickTextInput::event(QEvent* ev)
{
#ifndef QT_NO_SHORTCUT
    Q_D(QQuickTextInput);
    if (ev->type() == QEvent::ShortcutOverride) {
        if (d->m_readOnly)
            return false;
        QKeyEvent* ke = static_cast<QKeyEvent*>(ev);
        if (ke == QKeySequence::Copy
            || ke == QKeySequence::Paste
            || ke == QKeySequence::Cut
            || ke == QKeySequence::Redo
            || ke == QKeySequence::Undo
            || ke == QKeySequence::MoveToNextWord
            || ke == QKeySequence::MoveToPreviousWord
            || ke == QKeySequence::MoveToStartOfDocument
            || ke == QKeySequence::MoveToEndOfDocument
            || ke == QKeySequence::SelectNextWord
            || ke == QKeySequence::SelectPreviousWord
            || ke == QKeySequence::SelectStartOfLine
            || ke == QKeySequence::SelectEndOfLine
            || ke == QKeySequence::SelectStartOfBlock
            || ke == QKeySequence::SelectEndOfBlock
            || ke == QKeySequence::SelectStartOfDocument
            || ke == QKeySequence::SelectAll
            || ke == QKeySequence::SelectEndOfDocument) {
            ke->accept();
        } else if (ke->modifiers() == Qt::NoModifier || ke->modifiers() == Qt::ShiftModifier
                   || ke->modifiers() == Qt::KeypadModifier) {
            if (ke->key() < Qt::Key_Escape) {
                ke->accept();
                return true;
            } else {
                switch (ke->key()) {
                case Qt::Key_Delete:
                case Qt::Key_Home:
                case Qt::Key_End:
                case Qt::Key_Backspace:
                case Qt::Key_Left:
                case Qt::Key_Right:
                    return true;
                default:
                    break;
                }
            }
        }
    }
#endif

    return QQuickImplicitSizeItem::event(ev);
}

void QQuickTextInput::geometryChanged(const QRectF &newGeometry,
                                  const QRectF &oldGeometry)
{
    Q_D(QQuickTextInput);
    if (newGeometry.width() != oldGeometry.width())
        d->updateLayout();
    updateCursorRectangle();
    QQuickImplicitSizeItem::geometryChanged(newGeometry, oldGeometry);
}

void QQuickTextInputPrivate::updateHorizontalScroll()
{
    Q_Q(QQuickTextInput);
    QTextLine currentLine = m_textLayout.lineForTextPosition(m_cursor + m_preeditCursor);
    const int preeditLength = m_textLayout.preeditAreaText().length();
    const int width = qMax(0, qFloor(q->width()));
    int widthUsed = currentLine.isValid() ? qRound(currentLine.naturalTextWidth()) : 0;
    int previousScroll = hscroll;

    if (!autoScroll || widthUsed <=  width || m_echoMode == QQuickTextInput::NoEcho) {
        hscroll = 0;
    } else {
        Q_ASSERT(currentLine.isValid());
        int cix = qRound(currentLine.cursorToX(m_cursor + preeditLength));
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
             cix = qRound(currentLine.cursorToX(m_cursor + qMax(0, m_preeditCursor - 1)));
             if (cix < hscroll)
                 hscroll = cix;
        }
    }
    if (previousScroll != hscroll)
        textLayoutDirty = true;
}

void QQuickTextInputPrivate::updateVerticalScroll()
{
    Q_Q(QQuickTextInput);
    const int preeditLength = m_textLayout.preeditAreaText().length();
    const int height = qMax(0, qFloor(q->height()));
    int heightUsed = boundingRect.height();
    int previousScroll = vscroll;

    if (!autoScroll || heightUsed <=  height) {
        // text fits in br; use vscroll for alignment
        switch (vAlign & ~(Qt::AlignAbsolute|Qt::AlignHorizontal_Mask)) {
        case Qt::AlignBottom:
            vscroll = heightUsed - height;
            break;
        case Qt::AlignVCenter:
            vscroll = (heightUsed - height) / 2;
            break;
        default:
            // Top
            vscroll = 0;
            break;
        }
    } else {
        QTextLine currentLine = m_textLayout.lineForTextPosition(m_cursor + preeditLength);
        QRectF r = currentLine.isValid() ? currentLine.rect() : QRectF();
        int top = qFloor(r.top());
        int bottom = qCeil(r.bottom());

        if (bottom - vscroll >= height) {
            // text doesn't fit, cursor is to the below the br (scroll down)
            vscroll = bottom - height;
        } else if (top - vscroll < 0 && vscroll < heightUsed) {
            // text doesn't fit, cursor is above br (scroll up)
            vscroll = top;
        } else if (heightUsed - vscroll < height) {
            // text doesn't fit, text document is to the left of br; align
            // right
            vscroll = heightUsed - height;
        }
        if (preeditLength > 0) {
            // check to ensure long pre-edit text doesn't push the cursor
            // off the top
            currentLine = m_textLayout.lineForTextPosition(m_cursor + qMax(0, m_preeditCursor - 1));
            top = currentLine.isValid() ? qRound(currentLine.rect().top()) : 0;
            if (top < vscroll)
                vscroll = top;
        }
    }
    if (previousScroll != vscroll)
        textLayoutDirty = true;
}

void QQuickTextInput::triggerPreprocess()
{
    Q_D(QQuickTextInput);
    if (d->updateType == QQuickTextInputPrivate::UpdateNone)
        d->updateType = QQuickTextInputPrivate::UpdateOnlyPreprocess;
    update();
}

QSGNode *QQuickTextInput::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    Q_D(QQuickTextInput);

    if (d->updateType != QQuickTextInputPrivate::UpdatePaintNode && oldNode != 0) {
        // Update done in preprocess() in the nodes
        d->updateType = QQuickTextInputPrivate::UpdateNone;
        return oldNode;
    }

    d->updateType = QQuickTextInputPrivate::UpdateNone;

    QQuickTextNode *node = static_cast<QQuickTextNode *>(oldNode);
    if (node == 0)
        node = new QQuickTextNode(QQuickItemPrivate::get(this)->sceneGraphContext(), this);
    d->textNode = node;

    if (!d->textLayoutDirty) {
        QSGSimpleRectNode *cursorNode = node->cursorNode();
        if (cursorNode != 0 && !isReadOnly()) {
            cursorNode->setRect(cursorRectangle());

            if (!d->cursorVisible || (!d->m_blinkStatus && d->m_blinkPeriod > 0)) {
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
        if (d->autoScroll) {
            // the y offset is there to keep the baseline constant in case we have script changes in the text.
            offset = -QPoint(d->hscroll, d->vscroll + d->m_ascent - fm.ascent());
        } else {
            offset = -QPoint(d->hscroll, d->vscroll);
        }

        if (!d->m_textLayout.text().isEmpty() || !d->m_textLayout.preeditAreaText().isEmpty()) {
            node->addTextLayout(offset, &d->m_textLayout, d->color,
                                QQuickText::Normal, QColor(), QColor(),
                                d->selectionColor, d->selectedTextColor,
                                d->selectionStart(),
                                d->selectionEnd() - 1); // selectionEnd() returns first char after
                                                                 // selection
        }

        if (!isReadOnly() && d->cursorItem == 0) {
            node->setCursor(cursorRectangle(), d->color);
            if (!d->cursorVisible || (!d->m_blinkStatus && d->m_blinkPeriod > 0)) {
                d->hideCursor();
            } else {
                d->showCursor();
            }
        }

        d->textLayoutDirty = false;
    }

    return node;
}

QVariant QQuickTextInput::inputMethodQuery(Qt::InputMethodQuery property) const
{
    Q_D(const QQuickTextInput);
    switch (property) {
    case Qt::ImEnabled:
        return QVariant((bool)(flags() & ItemAcceptsInputMethod));
    case Qt::ImHints:
        return QVariant((int)inputMethodHints());
    case Qt::ImCursorRectangle:
        return cursorRectangle();
    case Qt::ImFont:
        return font();
    case Qt::ImCursorPosition:
        return QVariant(d->m_cursor);
    case Qt::ImSurroundingText:
        if (d->m_echoMode == PasswordEchoOnEdit && !d->m_passwordEchoEditing) {
            return QVariant(displayText());
        } else {
            return QVariant(d->realText());
        }
    case Qt::ImCurrentSelection:
        return QVariant(selectedText());
    case Qt::ImMaximumTextLength:
        return QVariant(maxLength());
    case Qt::ImAnchorPosition:
        if (d->selectionStart() == d->selectionEnd())
            return QVariant(d->m_cursor);
        else if (d->selectionStart() == d->m_cursor)
            return QVariant(d->selectionEnd());
        else
            return QVariant(d->selectionStart());
    default:
        return QVariant();
    }
}

/*!
    \qmlmethod void QtQuick2::TextInput::deselect()

    Removes active text selection.
*/
void QQuickTextInput::deselect()
{
    Q_D(QQuickTextInput);
    d->deselect();
}

/*!
    \qmlmethod void QtQuick2::TextInput::selectAll()

    Causes all text to be selected.
*/
void QQuickTextInput::selectAll()
{
    Q_D(QQuickTextInput);
    d->setSelection(0, text().length());
}

/*!
    \qmlmethod void QtQuick2::TextInput::isRightToLeft(int start, int end)

    Returns true if the natural reading direction of the editor text
    found between positions \a start and \a end is right to left.
*/
bool QQuickTextInput::isRightToLeft(int start, int end)
{
    if (start > end) {
        qmlInfo(this) << "isRightToLeft(start, end) called with the end property being smaller than the start.";
        return false;
    } else {
        return text().mid(start, end - start).isRightToLeft();
    }
}

#ifndef QT_NO_CLIPBOARD
/*!
    \qmlmethod QtQuick2::TextInput::cut()

    Moves the currently selected text to the system clipboard.
*/
void QQuickTextInput::cut()
{
    Q_D(QQuickTextInput);
    d->copy();
    d->del();
}

/*!
    \qmlmethod QtQuick2::TextInput::copy()

    Copies the currently selected text to the system clipboard.
*/
void QQuickTextInput::copy()
{
    Q_D(QQuickTextInput);
    d->copy();
}

/*!
    \qmlmethod QtQuick2::TextInput::paste()

    Replaces the currently selected text by the contents of the system clipboard.
*/
void QQuickTextInput::paste()
{
    Q_D(QQuickTextInput);
    if (!d->m_readOnly)
        d->paste();
}
#endif // QT_NO_CLIPBOARD

/*!
    Undoes the last operation if undo is \l {canUndo}{available}. Deselects any
    current selection, and updates the selection start to the current cursor
    position.
*/

void QQuickTextInput::undo()
{
    Q_D(QQuickTextInput);
    if (!d->m_readOnly) {
        d->internalUndo();
        d->finishChange(-1, true);
    }
}

/*!
    Redoes the last operation if redo is \l {canRedo}{available}.
*/

void QQuickTextInput::redo()
{
    Q_D(QQuickTextInput);
    if (!d->m_readOnly) {
        d->internalRedo();
        d->finishChange();
    }
}

/*!
    \qmlmethod void QtQuick2::TextInput::insert(int position, string text)

    Inserts \a text into the TextInput at position.
*/

void QQuickTextInput::insert(int position, const QString &text)
{
    Q_D(QQuickTextInput);
#ifdef QT_GUI_PASSWORD_ECHO_DELAY
    if (d->m_echoMode == QQuickTextInput::Password)
        d->m_passwordEchoTimer.start(qt_passwordEchoDelay, this);
#endif

    if (position < 0 || position > d->m_text.length())
        return;

    const int priorState = d->m_undoState;

    QString insertText = text;

    if (d->hasSelectedText()) {
        d->addCommand(QQuickTextInputPrivate::Command(
                QQuickTextInputPrivate::SetSelection, d->m_cursor, 0, d->m_selstart, d->m_selend));
    }
    if (d->m_maskData) {
        insertText = d->maskString(position, insertText);
        for (int i = 0; i < insertText.length(); ++i) {
            d->addCommand(QQuickTextInputPrivate::Command(
                    QQuickTextInputPrivate::DeleteSelection, position + i, d->m_text.at(position + i), -1, -1));
            d->addCommand(QQuickTextInputPrivate::Command(
                    QQuickTextInputPrivate::Insert, position + i, insertText.at(i), -1, -1));
        }
        d->m_text.replace(position, insertText.length(), insertText);
        if (!insertText.isEmpty())
            d->m_textDirty = true;
        if (position < d->m_selend && position + insertText.length() > d->m_selstart)
            d->m_selDirty = true;
    } else {
        int remaining = d->m_maxLength - d->m_text.length();
        if (remaining != 0) {
            insertText = insertText.left(remaining);
            d->m_text.insert(position, insertText);
            for (int i = 0; i < insertText.length(); ++i)
               d->addCommand(QQuickTextInputPrivate::Command(
                    QQuickTextInputPrivate::Insert, position + i, insertText.at(i), -1, -1));
            if (d->m_cursor >= position)
                d->m_cursor += insertText.length();
            if (d->m_selstart >= position)
                d->m_selstart += insertText.length();
            if (d->m_selend >= position)
                d->m_selend += insertText.length();
            d->m_textDirty = true;
            if (position >= d->m_selstart && position <= d->m_selend)
                d->m_selDirty = true;
        }
    }

    d->addCommand(QQuickTextInputPrivate::Command(
            QQuickTextInputPrivate::SetSelection, d->m_cursor, 0, d->m_selstart, d->m_selend));
    d->finishChange(priorState);

    if (d->lastSelectionStart != d->lastSelectionEnd) {
        if (d->m_selstart != d->lastSelectionStart) {
            d->lastSelectionStart = d->m_selstart;
            emit selectionStartChanged();
        }
        if (d->m_selend != d->lastSelectionEnd) {
            d->lastSelectionEnd = d->m_selend;
            emit selectionEndChanged();
        }
    }
}

/*!
    \qmlmethod string QtQuick2::TextInput::getText(int start, int end)

    Removes the section of text that is between the \a start and \a end positions from the TextInput.
*/

void QQuickTextInput::remove(int start, int end)
{
    Q_D(QQuickTextInput);

    start = qBound(0, start, d->m_text.length());
    end = qBound(0, end, d->m_text.length());

    if (start > end)
        qSwap(start, end);
    else if (start == end)
        return;

    if (start < d->m_selend && end > d->m_selstart)
        d->m_selDirty = true;

    const int priorState = d->m_undoState;

    d->addCommand(QQuickTextInputPrivate::Command(
            QQuickTextInputPrivate::SetSelection, d->m_cursor, 0, d->m_selstart, d->m_selend));

    if (start <= d->m_cursor && d->m_cursor < end) {
        // cursor is within the selection. Split up the commands
        // to be able to restore the correct cursor position
        for (int i = d->m_cursor; i >= start; --i) {
            d->addCommand(QQuickTextInputPrivate::Command(
                    QQuickTextInputPrivate::DeleteSelection, i, d->m_text.at(i), -1, 1));
        }
        for (int i = end - 1; i > d->m_cursor; --i) {
            d->addCommand(QQuickTextInputPrivate::Command(
                    QQuickTextInputPrivate::DeleteSelection, i - d->m_cursor + start - 1, d->m_text.at(i), -1, -1));
        }
    } else {
        for (int i = end - 1; i >= start; --i) {
            d->addCommand(QQuickTextInputPrivate::Command(
                    QQuickTextInputPrivate::RemoveSelection, i, d->m_text.at(i), -1, -1));
        }
    }
    if (d->m_maskData) {
        d->m_text.replace(start, end - start,  d->clearString(start, end - start));
        for (int i = 0; i < end - start; ++i) {
            d->addCommand(QQuickTextInputPrivate::Command(
                    QQuickTextInputPrivate::Insert, start + i, d->m_text.at(start + i), -1, -1));
        }
    } else {
        d->m_text.remove(start, end - start);

        if (d->m_cursor > start)
            d->m_cursor -= qMin(d->m_cursor, end) - start;
        if (d->m_selstart > start)
            d->m_selstart -= qMin(d->m_selstart, end) - start;
        if (d->m_selend > end)
            d->m_selend -= qMin(d->m_selend, end) - start;
    }
    d->addCommand(QQuickTextInputPrivate::Command(
            QQuickTextInputPrivate::SetSelection, d->m_cursor, 0, d->m_selstart, d->m_selend));

    d->m_textDirty = true;
    d->finishChange(priorState);

    if (d->lastSelectionStart != d->lastSelectionEnd) {
        if (d->m_selstart != d->lastSelectionStart) {
            d->lastSelectionStart = d->m_selstart;
            emit selectionStartChanged();
        }
        if (d->m_selend != d->lastSelectionEnd) {
            d->lastSelectionEnd = d->m_selend;
            emit selectionEndChanged();
        }
    }
}


/*!
    \qmlmethod void QtQuick2::TextInput::selectWord()

    Causes the word closest to the current cursor position to be selected.
*/
void QQuickTextInput::selectWord()
{
    Q_D(QQuickTextInput);
    d->selectWordAtPos(d->m_cursor);
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
QString QQuickTextInput::passwordCharacter() const
{
    Q_D(const QQuickTextInput);
    return QString(d->m_passwordCharacter);
}

void QQuickTextInput::setPasswordCharacter(const QString &str)
{
    Q_D(QQuickTextInput);
    if (str.length() < 1)
        return;
    d->m_passwordCharacter = str.constData()[0];
    if (d->m_echoMode == Password || d->m_echoMode == PasswordEchoOnEdit)
        d->updateDisplayText();
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
QString QQuickTextInput::displayText() const
{
    Q_D(const QQuickTextInput);
    return d->m_textLayout.text();
}

/*!
    \qmlproperty bool QtQuick2::TextInput::selectByMouse

    Defaults to false.

    If true, the user can use the mouse to select text in some
    platform-specific way. Note that for some platforms this may
    not be an appropriate interaction (eg. may conflict with how
    the text needs to behave inside a Flickable.
*/
bool QQuickTextInput::selectByMouse() const
{
    Q_D(const QQuickTextInput);
    return d->selectByMouse;
}

void QQuickTextInput::setSelectByMouse(bool on)
{
    Q_D(QQuickTextInput);
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

QQuickTextInput::SelectionMode QQuickTextInput::mouseSelectionMode() const
{
    Q_D(const QQuickTextInput);
    return d->mouseSelectionMode;
}

void QQuickTextInput::setMouseSelectionMode(SelectionMode mode)
{
    Q_D(QQuickTextInput);
    if (d->mouseSelectionMode != mode) {
        d->mouseSelectionMode = mode;
        emit mouseSelectionModeChanged(mode);
    }
}

/*!
    \qmlproperty bool QtQuick2::TextInput::persistentSelection

    Whether the TextInput should keep its selection when it loses active focus to another
    item in the scene. By default this is set to false;
*/

bool QQuickTextInput::persistentSelection() const
{
    Q_D(const QQuickTextInput);
    return d->persistentSelection;
}

void QQuickTextInput::setPersistentSelection(bool on)
{
    Q_D(QQuickTextInput);
    if (d->persistentSelection == on)
        return;
    d->persistentSelection = on;
    emit persistentSelectionChanged();
}

/*!
    \qmlproperty bool QtQuick2::TextInput::canPaste

    Returns true if the TextInput is writable and the content of the clipboard is
    suitable for pasting into the TextInput.
*/
bool QQuickTextInput::canPaste() const
{
    Q_D(const QQuickTextInput);
    if (!d->canPasteValid) {
        if (const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData())
            const_cast<QQuickTextInputPrivate *>(d)->canPaste = !d->m_readOnly && mimeData->hasText();
        const_cast<QQuickTextInputPrivate *>(d)->canPasteValid = true;
    }
    return d->canPaste;
}

/*!
    \qmlproperty bool QtQuick2::TextInput::canUndo

    Returns true if the TextInput is writable and there are previous operations
    that can be undone.
*/

bool QQuickTextInput::canUndo() const
{
    Q_D(const QQuickTextInput);
    return d->canUndo;
}

/*!
    \qmlproperty bool QtQuick2::TextInput::canRedo

    Returns true if the TextInput is writable and there are \l {undo}{undone}
    operations that can be redone.
*/

bool QQuickTextInput::canRedo() const
{
    Q_D(const QQuickTextInput);
    return d->canRedo;
}

/*!
    \qmlproperty real QtQuick2::TextInput::contentWidth

    Returns the width of the text, including the width past the width
    which is covered due to insufficient wrapping if \l wrapMode is set.
*/

qreal QQuickTextInput::contentWidth() const
{
    Q_D(const QQuickTextInput);
    return d->boundingRect.width();
}

/*!
    \qmlproperty real QtQuick2::TextInput::contentHeight

    Returns the height of the text, including the height past the height
    that is covered if the text does not fit within the set height.
*/

qreal QQuickTextInput::contentHeight() const
{
    Q_D(const QQuickTextInput);
    return d->boundingRect.height();
}

void QQuickTextInput::moveCursorSelection(int position)
{
    Q_D(QQuickTextInput);
    d->moveCursor(position, true);
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
    \o TextInput.SelectCharacters - Sets either the selectionStart or selectionEnd (whichever was at
    the previous cursor position) to the specified position.
    \o TextInput.SelectWords - Sets the selectionStart and selectionEnd to include all
    words between the specified position and the previous cursor position.  Words partially in the
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
void QQuickTextInput::moveCursorSelection(int pos, SelectionMode mode)
{
    Q_D(QQuickTextInput);

    if (mode == SelectCharacters) {
        d->moveCursor(pos, true);
    } else if (pos != d->m_cursor){
        const int cursor = d->m_cursor;
        int anchor;
        if (!d->hasSelectedText())
            anchor = d->m_cursor;
        else if (d->selectionStart() == d->m_cursor)
            anchor = d->selectionEnd();
        else
            anchor = d->selectionStart();

        if (anchor < pos || (anchor == pos && cursor < pos)) {
            const QString text = this->text();
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

            d->setSelection(anchor, cursor - anchor);
        } else if (anchor > pos || (anchor == pos && cursor > pos)) {
            const QString text = this->text();
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

            d->setSelection(anchor, cursor - anchor);
        }
    }
}

/*!
    \qmlmethod void QtQuick2::TextInput::openSoftwareInputPanel()

    Opens software input panels like virtual keyboards for typing, useful for
    customizing when you want the input keyboard to be shown and hidden in
    your application.

    By default the opening of input panels follows the platform style. Input panels are
    always closed if no editor has active focus.

    You can disable the automatic behavior by setting the property \c activeFocusOnPress to false
    and use functions openSoftwareInputPanel() and closeSoftwareInputPanel() to implement
    the behavior you want.

    Only relevant on platforms, which provide virtual keyboards.

    \qml
        import QtQuick 2.0
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
void QQuickTextInput::openSoftwareInputPanel()
{
    if (qGuiApp)
        qGuiApp->inputPanel()->show();
}

/*!
    \qmlmethod void QtQuick2::TextInput::closeSoftwareInputPanel()

    Closes a software input panel like a virtual keyboard shown on the screen, useful
    for customizing when you want the input keyboard to be shown and hidden in
    your application.

    By default the opening of input panels follows the platform style. Input panels are
    always closed if no editor has active focus.

    You can disable the automatic behavior by setting the property \c activeFocusOnPress to false
    and use functions openSoftwareInputPanel() and closeSoftwareInputPanel() to implement
    the behavior you want.

    Only relevant on platforms, which provide virtual keyboards.

    \qml
        import QtQuick 2.0
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
void QQuickTextInput::closeSoftwareInputPanel()
{
    if (qGuiApp)
        qGuiApp->inputPanel()->hide();
}

void QQuickTextInput::focusInEvent(QFocusEvent *event)
{
    Q_D(const QQuickTextInput);
    if (d->focusOnPress && !d->m_readOnly)
        openSoftwareInputPanel();
    QQuickImplicitSizeItem::focusInEvent(event);
}

void QQuickTextInput::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickTextInput);
    if (change == ItemActiveFocusHasChanged) {
        bool hasFocus = value.boolValue;
        d->focused = hasFocus;
        setCursorVisible(hasFocus); // ### refactor:  && d->canvas && d->canvas->hasFocus()
#ifdef QT_GUI_PASSWORD_ECHO_DELAY
        if (!hasFocus && (d->m_passwordEchoEditing || d->m_passwordEchoTimer.isActive())) {
#else
        if (!hasFocus && d->m_passwordEchoEditing) {
#endif
            d->updatePasswordEchoEditing(false);//QQuickTextInputPrivate sets it on key events, but doesn't deal with focus events
        }

        if (!hasFocus) {
            d->commitPreedit();
            if (!d->persistentSelection)
                d->deselect();
            disconnect(qApp->inputPanel(), SIGNAL(inputDirectionChanged(Qt::LayoutDirection)),
                       this, SLOT(q_updateAlignment()));
        } else {
            q_updateAlignment();
            connect(qApp->inputPanel(), SIGNAL(inputDirectionChanged(Qt::LayoutDirection)),
                    this, SLOT(q_updateAlignment()));
        }
    }
    QQuickItem::itemChange(change, value);
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
bool QQuickTextInput::isInputMethodComposing() const
{
    Q_D(const QQuickTextInput);
    return d->preeditAreaText().length() > 0;
}

void QQuickTextInputPrivate::init()
{
    Q_Q(QQuickTextInput);
    q->setSmooth(smooth);
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFlag(QQuickItem::ItemAcceptsInputMethod);
    q->setFlag(QQuickItem::ItemHasContents);
#ifndef QT_NO_CLIPBOARD
    q->connect(QGuiApplication::clipboard(), SIGNAL(dataChanged()),
            q, SLOT(q_canPasteChanged()));
#endif // QT_NO_CLIPBOARD

    lastSelectionStart = 0;
    lastSelectionEnd = 0;
    selectedTextColor = m_palette.color(QPalette::HighlightedText);
    selectionColor = m_palette.color(QPalette::Highlight);
    determineHorizontalAlignment();

    if (!qmlDisableDistanceField()) {
        QTextOption option = m_textLayout.textOption();
        option.setUseDesignMetrics(true);
        m_textLayout.setTextOption(option);
    }
}

void QQuickTextInput::updateCursorRectangle()
{
    Q_D(QQuickTextInput);
    if (!isComponentComplete())
        return;

    d->updateHorizontalScroll();
    d->updateVerticalScroll();
    d->updateType = QQuickTextInputPrivate::UpdatePaintNode;
    update();
    emit cursorRectangleChanged();
    if (d->cursorItem) {
        QRectF r = cursorRectangle();
        d->cursorItem->setPos(r.topLeft());
        d->cursorItem->setHeight(r.height());
    }
    updateInputMethod(Qt::ImCursorRectangle);
}

void QQuickTextInput::selectionChanged()
{
    Q_D(QQuickTextInput);
    d->textLayoutDirty = true; //TODO: Only update rect in selection
    d->updateType = QQuickTextInputPrivate::UpdatePaintNode;
    update();
    emit selectedTextChanged();

    if (d->lastSelectionStart != d->selectionStart()) {
        d->lastSelectionStart = d->selectionStart();
        if (d->lastSelectionStart == -1)
            d->lastSelectionStart = d->m_cursor;
        emit selectionStartChanged();
    }
    if (d->lastSelectionEnd != d->selectionEnd()) {
        d->lastSelectionEnd = d->selectionEnd();
        if (d->lastSelectionEnd == -1)
            d->lastSelectionEnd = d->m_cursor;
        emit selectionEndChanged();
    }
}

void QQuickTextInputPrivate::showCursor()
{
    if (textNode != 0 && textNode->cursorNode() != 0)
        textNode->cursorNode()->setColor(color);
}

void QQuickTextInputPrivate::hideCursor()
{
    if (textNode != 0 && textNode->cursorNode() != 0)
        textNode->cursorNode()->setColor(QColor(0, 0, 0, 0));
}

QRectF QQuickTextInput::boundingRect() const
{
    Q_D(const QQuickTextInput);

    int cursorWidth = d->cursorItem ? d->cursorItem->width() : d->m_cursorWidth;

    // Could include font max left/right bearings to either side of rectangle.
    QRectF r = QQuickImplicitSizeItem::boundingRect();
    r.setRight(r.right() + cursorWidth);
    return r;
}

void QQuickTextInput::q_canPasteChanged()
{
    Q_D(QQuickTextInput);
    bool old = d->canPaste;
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData())
        d->canPaste = !d->m_readOnly && mimeData->hasText();
    else
        d->canPaste = false;
#endif

    bool changed = d->canPaste != old || !d->canPasteValid;
    d->canPasteValid = true;
    if (changed)
        emit canPasteChanged();

}

void QQuickTextInput::q_updateAlignment()
{
    Q_D(QQuickTextInput);
    if (d->determineHorizontalAlignment()) {
        d->updateLayout();
        updateCursorRectangle();
    }
}

// ### these should come from QStyleHints
const int textCursorWidth = 1;
const bool fullWidthSelection = true;

/*!
    \internal

    Updates the display text based of the current edit text
    If the text has changed will emit displayTextChanged()
*/
void QQuickTextInputPrivate::updateDisplayText(bool forceUpdate)
{
    QString orig = m_textLayout.text();
    QString str;
    if (m_echoMode == QQuickTextInput::NoEcho)
        str = QString::fromLatin1("");
    else
        str = m_text;

    if (m_echoMode == QQuickTextInput::Password) {
         str.fill(m_passwordCharacter);
#ifdef QT_GUI_PASSWORD_ECHO_DELAY
        if (m_passwordEchoTimer.isActive() && m_cursor > 0 && m_cursor <= m_text.length()) {
            int cursor = m_cursor - 1;
            QChar uc = m_text.at(cursor);
            str[cursor] = uc;
            if (cursor > 0 && uc.unicode() >= 0xdc00 && uc.unicode() < 0xe000) {
                // second half of a surrogate, check if we have the first half as well,
                // if yes restore both at once
                uc = m_text.at(cursor - 1);
                if (uc.unicode() >= 0xd800 && uc.unicode() < 0xdc00)
                    str[cursor - 1] = uc;
            }
        }
#endif
    } else if (m_echoMode == QQuickTextInput::PasswordEchoOnEdit && !m_passwordEchoEditing) {
        str.fill(m_passwordCharacter);
    }

    // replace certain non-printable characters with spaces (to avoid
    // drawing boxes when using fonts that don't have glyphs for such
    // characters)
    QChar* uc = str.data();
    for (int i = 0; i < (int)str.length(); ++i) {
        if ((uc[i] < 0x20 && uc[i] != 0x09)
            || uc[i] == QChar::LineSeparator
            || uc[i] == QChar::ParagraphSeparator
            || uc[i] == QChar::ObjectReplacementCharacter)
            uc[i] = QChar(0x0020);
    }

    if (str != orig || forceUpdate) {
        m_textLayout.setText(str);
        updateLayout(); // polish?
        emit q_func()->displayTextChanged();
    }
}

void QQuickTextInputPrivate::updateLayout()
{
    Q_Q(QQuickTextInput);

    if (!q->isComponentComplete())
        return;

    const QRectF previousRect = boundingRect;

    QTextOption option = m_textLayout.textOption();
    option.setTextDirection(layoutDirection());
    option.setFlags(QTextOption::IncludeTrailingSpaces);
    option.setWrapMode(QTextOption::WrapMode(wrapMode));
    option.setAlignment(Qt::Alignment(q->effectiveHAlign()));
    m_textLayout.setTextOption(option);
    m_textLayout.setFont(font);

    boundingRect = QRectF();
    m_textLayout.beginLayout();
    QTextLine line = m_textLayout.createLine();
    qreal lineWidth = q->widthValid() ? q->width() : INT_MAX;
    qreal height = 0;
    QTextLine firstLine = line;
    do {
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(line.position().x(), height));
        boundingRect = boundingRect.united(line.naturalTextRect());

        height += line.height();
        line = m_textLayout.createLine();
    } while (line.isValid());
    m_textLayout.endLayout();

    option.setWrapMode(QTextOption::NoWrap);
    m_textLayout.setTextOption(option);

    m_ascent = qRound(firstLine.ascent());
    textLayoutDirty = true;

    updateType = UpdatePaintNode;
    q->update();
    q->setImplicitSize(qCeil(boundingRect.width()), qCeil(boundingRect.height()));

    if (previousRect != boundingRect)
        emit q->contentSizeChanged();
}

#ifndef QT_NO_CLIPBOARD
/*!
    \internal

    Copies the currently selected text into the clipboard using the given
    \a mode.

    \note If the echo mode is set to a mode other than Normal then copy
    will not work.  This is to prevent using copy as a method of bypassing
    password features of the line control.
*/
void QQuickTextInputPrivate::copy(QClipboard::Mode mode) const
{
    QString t = selectedText();
    if (!t.isEmpty() && m_echoMode == QQuickTextInput::Normal) {
        QGuiApplication::clipboard()->setText(t, mode);
    }
}

/*!
    \internal

    Inserts the text stored in the application clipboard into the line
    control.

    \sa insert()
*/
void QQuickTextInputPrivate::paste(QClipboard::Mode clipboardMode)
{
    QString clip = QGuiApplication::clipboard()->text(clipboardMode);
    if (!clip.isEmpty() || hasSelectedText()) {
        separate(); //make it a separate undo/redo command
        insert(clip);
        separate();
    }
}

#endif // !QT_NO_CLIPBOARD

/*!
    \internal

    Exits preedit mode and commits parts marked as tentative commit
*/
void QQuickTextInputPrivate::commitPreedit()
{
    if (!composeMode())
        return;

    qApp->inputPanel()->reset();

    if (!m_tentativeCommit.isEmpty()) {
        internalInsert(m_tentativeCommit);
        m_tentativeCommit.clear();
        finishChange(-1, true/*not used, not documented*/, false);
    }

    m_preeditCursor = 0;
    m_textLayout.setPreeditArea(-1, QString());
    m_textLayout.clearAdditionalFormats();
    updateLayout();
}

/*!
    \internal

    Handles the behavior for the backspace key or function.
    Removes the current selection if there is a selection, otherwise
    removes the character prior to the cursor position.

    \sa del()
*/
void QQuickTextInputPrivate::backspace()
{
    int priorState = m_undoState;
    if (hasSelectedText()) {
        removeSelectedText();
    } else if (m_cursor) {
            --m_cursor;
            if (m_maskData)
                m_cursor = prevMaskBlank(m_cursor);
            QChar uc = m_text.at(m_cursor);
            if (m_cursor > 0 && uc.unicode() >= 0xdc00 && uc.unicode() < 0xe000) {
                // second half of a surrogate, check if we have the first half as well,
                // if yes delete both at once
                uc = m_text.at(m_cursor - 1);
                if (uc.unicode() >= 0xd800 && uc.unicode() < 0xdc00) {
                    internalDelete(true);
                    --m_cursor;
                }
            }
            internalDelete(true);
    }
    finishChange(priorState);
}

/*!
    \internal

    Handles the behavior for the delete key or function.
    Removes the current selection if there is a selection, otherwise
    removes the character after the cursor position.

    \sa del()
*/
void QQuickTextInputPrivate::del()
{
    int priorState = m_undoState;
    if (hasSelectedText()) {
        removeSelectedText();
    } else {
        int n = m_textLayout.nextCursorPosition(m_cursor) - m_cursor;
        while (n--)
            internalDelete();
    }
    finishChange(priorState);
}

/*!
    \internal

    Inserts the given \a newText at the current cursor position.
    If there is any selected text it is removed prior to insertion of
    the new text.
*/
void QQuickTextInputPrivate::insert(const QString &newText)
{
    int priorState = m_undoState;
    removeSelectedText();
    internalInsert(newText);
    finishChange(priorState);
}

/*!
    \internal

    Clears the line control text.
*/
void QQuickTextInputPrivate::clear()
{
    int priorState = m_undoState;
    m_selstart = 0;
    m_selend = m_text.length();
    removeSelectedText();
    separate();
    finishChange(priorState, /*update*/false, /*edited*/false);
}

/*!
    \internal

    Sets \a length characters from the given \a start position as selected.
    The given \a start position must be within the current text for
    the line control.  If \a length characters cannot be selected, then
    the selection will extend to the end of the current text.
*/
void QQuickTextInputPrivate::setSelection(int start, int length)
{
    Q_Q(QQuickTextInput);
    commitPreedit();

    if (start < 0 || start > (int)m_text.length()){
        qWarning("QQuickTextInputPrivate::setSelection: Invalid start position");
        return;
    }

    if (length > 0) {
        if (start == m_selstart && start + length == m_selend && m_cursor == m_selend)
            return;
        m_selstart = start;
        m_selend = qMin(start + length, (int)m_text.length());
        m_cursor = m_selend;
    } else if (length < 0){
        if (start == m_selend && start + length == m_selstart && m_cursor == m_selstart)
            return;
        m_selstart = qMax(start + length, 0);
        m_selend = start;
        m_cursor = m_selstart;
    } else if (m_selstart != m_selend) {
        m_selstart = 0;
        m_selend = 0;
        m_cursor = start;
    } else {
        m_cursor = start;
        emitCursorPositionChanged();
        return;
    }
    emit q->selectionChanged();
    emitCursorPositionChanged();
    q->updateInputMethod(Qt::ImCursorRectangle | Qt::ImAnchorPosition
                        | Qt::ImCursorPosition | Qt::ImCurrentSelection);
}

/*!
    \internal

    Initializes the line control with a starting text value of \a txt.
*/
void QQuickTextInputPrivate::init(const QString &txt)
{
    m_text = txt;

    updateDisplayText();
    m_cursor = m_text.length();
}

/*!
    \internal

    Sets the password echo editing to \a editing.  If password echo editing
    is true, then the text of the password is displayed even if the echo
    mode is set to QLineEdit::PasswordEchoOnEdit.  Password echoing editing
    does not affect other echo modes.
*/
void QQuickTextInputPrivate::updatePasswordEchoEditing(bool editing)
{
    cancelPasswordEchoTimer();
    m_passwordEchoEditing = editing;
    updateDisplayText();
}

/*!
    \internal

    Fixes the current text so that it is valid given any set validators.

    Returns true if the text was changed.  Otherwise returns false.
*/
bool QQuickTextInputPrivate::fixup() // this function assumes that validate currently returns != Acceptable
{
#ifndef QT_NO_VALIDATOR
    if (m_validator) {
        QString textCopy = m_text;
        int cursorCopy = m_cursor;
        m_validator->fixup(textCopy);
        if (m_validator->validate(textCopy, cursorCopy) == QValidator::Acceptable) {
            if (textCopy != m_text || cursorCopy != m_cursor)
                internalSetText(textCopy, cursorCopy);
            return true;
        }
    }
#endif
    return false;
}

/*!
    \internal

    Moves the cursor to the given position \a pos.   If \a mark is true will
    adjust the currently selected text.
*/
void QQuickTextInputPrivate::moveCursor(int pos, bool mark)
{
    Q_Q(QQuickTextInput);
    commitPreedit();

    if (pos != m_cursor) {
        separate();
        if (m_maskData)
            pos = pos > m_cursor ? nextMaskBlank(pos) : prevMaskBlank(pos);
    }
    if (mark) {
        int anchor;
        if (m_selend > m_selstart && m_cursor == m_selstart)
            anchor = m_selend;
        else if (m_selend > m_selstart && m_cursor == m_selend)
            anchor = m_selstart;
        else
            anchor = m_cursor;
        m_selstart = qMin(anchor, pos);
        m_selend = qMax(anchor, pos);
    } else {
        internalDeselect();
    }
    m_cursor = pos;
    if (mark || m_selDirty) {
        m_selDirty = false;
        emit q->selectionChanged();
    }
    emitCursorPositionChanged();
    q->updateInputMethod();
}

/*!
    \internal

    Applies the given input method event \a event to the text of the line
    control
*/
void QQuickTextInputPrivate::processInputMethodEvent(QInputMethodEvent *event)
{
    Q_Q(QQuickTextInput);

    int priorState = -1;
    bool isGettingInput = !event->commitString().isEmpty()
            || event->preeditString() != preeditAreaText()
            || event->replacementLength() > 0;
    bool cursorPositionChanged = false;
    bool selectionChange = false;
    m_preeditDirty = event->preeditString() != preeditAreaText();

    if (isGettingInput) {
        // If any text is being input, remove selected text.
        priorState = m_undoState;
        if (m_echoMode == QQuickTextInput::PasswordEchoOnEdit && !m_passwordEchoEditing) {
            updatePasswordEchoEditing(true);
            m_selstart = 0;
            m_selend = m_text.length();
        }
        removeSelectedText();
    }

    int c = m_cursor; // cursor position after insertion of commit string
    if (event->replacementStart() <= 0)
        c += event->commitString().length() - qMin(-event->replacementStart(), event->replacementLength());

    m_cursor += event->replacementStart();
    if (m_cursor < 0)
        m_cursor = 0;

    // insert commit string
    if (event->replacementLength()) {
        m_selstart = m_cursor;
        m_selend = m_selstart + event->replacementLength();
        m_selend = qMin(m_selend, m_text.length());
        removeSelectedText();
    }
    if (!event->commitString().isEmpty()) {
        internalInsert(event->commitString());
        cursorPositionChanged = true;
    }

    m_cursor = qBound(0, c, m_text.length());

    for (int i = 0; i < event->attributes().size(); ++i) {
        const QInputMethodEvent::Attribute &a = event->attributes().at(i);
        if (a.type == QInputMethodEvent::Selection) {
            m_cursor = qBound(0, a.start + a.length, m_text.length());
            if (a.length) {
                m_selstart = qMax(0, qMin(a.start, m_text.length()));
                m_selend = m_cursor;
                if (m_selend < m_selstart) {
                    qSwap(m_selstart, m_selend);
                }
                selectionChange = true;
            } else {
                m_selstart = m_selend = 0;
            }
            cursorPositionChanged = true;
        }
    }
#ifndef QT_NO_IM
    m_textLayout.setPreeditArea(m_cursor, event->preeditString());
#endif //QT_NO_IM
    const int oldPreeditCursor = m_preeditCursor;
    m_preeditCursor = event->preeditString().length();
    m_hideCursor = false;
    QList<QTextLayout::FormatRange> formats;
    for (int i = 0; i < event->attributes().size(); ++i) {
        const QInputMethodEvent::Attribute &a = event->attributes().at(i);
        if (a.type == QInputMethodEvent::Cursor) {
            m_preeditCursor = a.start;
            m_hideCursor = !a.length;
        } else if (a.type == QInputMethodEvent::TextFormat) {
            QTextCharFormat f = qvariant_cast<QTextFormat>(a.value).toCharFormat();
            if (f.isValid()) {
                QTextLayout::FormatRange o;
                o.start = a.start + m_cursor;
                o.length = a.length;
                o.format = f;
                formats.append(o);
            }
        }
    }
    m_textLayout.setAdditionalFormats(formats);

    updateDisplayText(/*force*/ true);
    if (cursorPositionChanged) {
        emitCursorPositionChanged();
    } else if (m_preeditCursor != oldPreeditCursor) {
        q->updateCursorRectangle();
    }

    bool tentativeCommitChanged = m_tentativeCommit != event->tentativeCommitString();

    if (tentativeCommitChanged) {
        m_textDirty = true;
        m_tentativeCommit = event->tentativeCommitString();
    }

    if (isGettingInput || tentativeCommitChanged)
        finishChange(priorState);

    if (selectionChange) {
        emit q->selectionChanged();
        q->updateInputMethod(Qt::ImCursorRectangle | Qt::ImAnchorPosition
                            | Qt::ImCursorPosition | Qt::ImCurrentSelection);
    }
}

/*!
    \internal

    Sets the selection to cover the word at the given cursor position.
    The word boundaries are defined by the behavior of QTextLayout::SkipWords
    cursor mode.
*/
void QQuickTextInputPrivate::selectWordAtPos(int cursor)
{
    int next = cursor + 1;
    if (next > end())
        --next;
    int c = m_textLayout.previousCursorPosition(next, QTextLayout::SkipWords);
    moveCursor(c, false);
    // ## text layout should support end of words.
    int end = m_textLayout.nextCursorPosition(c, QTextLayout::SkipWords);
    while (end > cursor && m_text[end-1].isSpace())
        --end;
    moveCursor(end, true);
}

/*!
    \internal

    Completes a change to the line control text.  If the change is not valid
    will undo the line control state back to the given \a validateFromState.

    If \a edited is true and the change is valid, will emit textEdited() in
    addition to textChanged().  Otherwise only emits textChanged() on a valid
    change.

    The \a update value is currently unused.
*/
bool QQuickTextInputPrivate::finishChange(int validateFromState, bool update, bool /*edited*/)
{
    Q_Q(QQuickTextInput);

    Q_UNUSED(update)
    bool inputMethodAttributesChanged = m_textDirty || m_selDirty;
    bool alignmentChanged = false;

    if (m_textDirty) {
        // do validation
        bool wasValidInput = m_validInput;
        bool wasAcceptable = m_acceptableInput;
        m_validInput = true;
        m_acceptableInput = true;
#ifndef QT_NO_VALIDATOR
        if (m_validator) {
            QString textCopy = m_text;
            int cursorCopy = m_cursor;
            QValidator::State state = m_validator->validate(textCopy, cursorCopy);
            m_validInput = state != QValidator::Invalid;
            m_acceptableInput = state == QValidator::Acceptable;
            if (m_validInput) {
                if (m_text != textCopy) {
                    internalSetText(textCopy, cursorCopy);
                    return true;
                }
                m_cursor = cursorCopy;

                if (!m_tentativeCommit.isEmpty()) {
                    textCopy.insert(m_cursor, m_tentativeCommit);
                    bool validInput = m_validator->validate(textCopy, cursorCopy) != QValidator::Invalid;
                    if (!validInput)
                        m_tentativeCommit.clear();
                }
            } else {
                m_tentativeCommit.clear();
            }
        }
#endif
        if (validateFromState >= 0 && wasValidInput && !m_validInput) {
            if (m_transactions.count())
                return false;
            internalUndo(validateFromState);
            m_history.resize(m_undoState);
            if (m_modifiedState > m_undoState)
                m_modifiedState = -1;
            m_validInput = true;
            m_acceptableInput = wasAcceptable;
            m_textDirty = false;
        }

        if (m_textDirty) {
            m_textDirty = false;
            m_preeditDirty = false;
            alignmentChanged = determineHorizontalAlignment();
            emit q->textChanged();
        }

        updateDisplayText(alignmentChanged);

        if (m_acceptableInput != wasAcceptable)
            emit q->acceptableInputChanged();
    }
    if (m_preeditDirty) {
        m_preeditDirty = false;
        if (determineHorizontalAlignment()) {
            alignmentChanged = true;
            updateLayout();
        }
    }

    if (m_selDirty) {
        m_selDirty = false;
        emit q->selectionChanged();
    }

    inputMethodAttributesChanged |= (m_cursor == m_lastCursorPos);
    if (inputMethodAttributesChanged)
        q->updateInputMethod();
    emitUndoRedoChanged();

    if (!emitCursorPositionChanged() && alignmentChanged)
        q->updateCursorRectangle();

    return true;
}

/*!
    \internal

    An internal function for setting the text of the line control.
*/
void QQuickTextInputPrivate::internalSetText(const QString &txt, int pos, bool edited)
{
    Q_Q(QQuickTextInput);
    internalDeselect();
    QString oldText = m_text;
    if (m_maskData) {
        m_text = maskString(0, txt, true);
        m_text += clearString(m_text.length(), m_maxLength - m_text.length());
    } else {
        m_text = txt.isEmpty() ? txt : txt.left(m_maxLength);
    }
    m_history.clear();
    m_modifiedState =  m_undoState = 0;
    m_cursor = (pos < 0 || pos > m_text.length()) ? m_text.length() : pos;
    m_textDirty = (oldText != m_text);

    bool changed = finishChange(-1, true, edited);
#ifdef QT_NO_ACCESSIBILITY
    Q_UNUSED(changed)
#else
    if (changed)
        QAccessible::updateAccessibility(QAccessibleEvent(QAccessible::TextUpdated, q, 0));
#endif
}


/*!
    \internal

    Adds the given \a command to the undo history
    of the line control.  Does not apply the command.
*/
void QQuickTextInputPrivate::addCommand(const Command &cmd)
{
    if (m_separator && m_undoState && m_history[m_undoState - 1].type != Separator) {
        m_history.resize(m_undoState + 2);
        m_history[m_undoState++] = Command(Separator, m_cursor, 0, m_selstart, m_selend);
    } else {
        m_history.resize(m_undoState + 1);
    }
    m_separator = false;
    m_history[m_undoState++] = cmd;
}

/*!
    \internal

    Inserts the given string \a s into the line
    control.

    Also adds the appropriate commands into the undo history.
    This function does not call finishChange(), and may leave the text
    in an invalid state.
*/
void QQuickTextInputPrivate::internalInsert(const QString &s)
{
#ifdef QT_GUI_PASSWORD_ECHO_DELAY
    Q_Q(QQuickTextInput);
    if (m_echoMode == QQuickTextInput::Password)
        m_passwordEchoTimer.start(qt_passwordEchoDelay, q);
#endif
    if (hasSelectedText())
        addCommand(Command(SetSelection, m_cursor, 0, m_selstart, m_selend));
    if (m_maskData) {
        QString ms = maskString(m_cursor, s);
        for (int i = 0; i < (int) ms.length(); ++i) {
            addCommand (Command(DeleteSelection, m_cursor + i, m_text.at(m_cursor + i), -1, -1));
            addCommand(Command(Insert, m_cursor + i, ms.at(i), -1, -1));
        }
        m_text.replace(m_cursor, ms.length(), ms);
        m_cursor += ms.length();
        m_cursor = nextMaskBlank(m_cursor);
        m_textDirty = true;
    } else {
        int remaining = m_maxLength - m_text.length();
        if (remaining != 0) {
            m_text.insert(m_cursor, s.left(remaining));
            for (int i = 0; i < (int) s.left(remaining).length(); ++i)
               addCommand(Command(Insert, m_cursor++, s.at(i), -1, -1));
            m_textDirty = true;
        }
    }
}

/*!
    \internal

    deletes a single character from the current text.  If \a wasBackspace,
    the character prior to the cursor is removed.  Otherwise the character
    after the cursor is removed.

    Also adds the appropriate commands into the undo history.
    This function does not call finishChange(), and may leave the text
    in an invalid state.
*/
void QQuickTextInputPrivate::internalDelete(bool wasBackspace)
{
    if (m_cursor < (int) m_text.length()) {
        cancelPasswordEchoTimer();
        if (hasSelectedText())
            addCommand(Command(SetSelection, m_cursor, 0, m_selstart, m_selend));
        addCommand(Command((CommandType)((m_maskData ? 2 : 0) + (wasBackspace ? Remove : Delete)),
                   m_cursor, m_text.at(m_cursor), -1, -1));
        if (m_maskData) {
            m_text.replace(m_cursor, 1, clearString(m_cursor, 1));
            addCommand(Command(Insert, m_cursor, m_text.at(m_cursor), -1, -1));
        } else {
            m_text.remove(m_cursor, 1);
        }
        m_textDirty = true;
    }
}

/*!
    \internal

    removes the currently selected text from the line control.

    Also adds the appropriate commands into the undo history.
    This function does not call finishChange(), and may leave the text
    in an invalid state.
*/
void QQuickTextInputPrivate::removeSelectedText()
{
    if (m_selstart < m_selend && m_selend <= (int) m_text.length()) {
        cancelPasswordEchoTimer();
        separate();
        int i ;
        addCommand(Command(SetSelection, m_cursor, 0, m_selstart, m_selend));
        if (m_selstart <= m_cursor && m_cursor < m_selend) {
            // cursor is within the selection. Split up the commands
            // to be able to restore the correct cursor position
            for (i = m_cursor; i >= m_selstart; --i)
                addCommand (Command(DeleteSelection, i, m_text.at(i), -1, 1));
            for (i = m_selend - 1; i > m_cursor; --i)
                addCommand (Command(DeleteSelection, i - m_cursor + m_selstart - 1, m_text.at(i), -1, -1));
        } else {
            for (i = m_selend-1; i >= m_selstart; --i)
                addCommand (Command(RemoveSelection, i, m_text.at(i), -1, -1));
        }
        if (m_maskData) {
            m_text.replace(m_selstart, m_selend - m_selstart,  clearString(m_selstart, m_selend - m_selstart));
            for (int i = 0; i < m_selend - m_selstart; ++i)
                addCommand(Command(Insert, m_selstart + i, m_text.at(m_selstart + i), -1, -1));
        } else {
            m_text.remove(m_selstart, m_selend - m_selstart);
        }
        if (m_cursor > m_selstart)
            m_cursor -= qMin(m_cursor, m_selend) - m_selstart;
        internalDeselect();
        m_textDirty = true;
    }
}

/*!
    \internal

    Parses the input mask specified by \a maskFields to generate
    the mask data used to handle input masks.
*/
void QQuickTextInputPrivate::parseInputMask(const QString &maskFields)
{
    int delimiter = maskFields.indexOf(QLatin1Char(';'));
    if (maskFields.isEmpty() || delimiter == 0) {
        if (m_maskData) {
            delete [] m_maskData;
            m_maskData = 0;
            m_maxLength = 32767;
            internalSetText(QString());
        }
        return;
    }

    if (delimiter == -1) {
        m_blank = QLatin1Char(' ');
        m_inputMask = maskFields;
    } else {
        m_inputMask = maskFields.left(delimiter);
        m_blank = (delimiter + 1 < maskFields.length()) ? maskFields[delimiter + 1] : QLatin1Char(' ');
    }

    // calculate m_maxLength / m_maskData length
    m_maxLength = 0;
    QChar c = 0;
    for (int i=0; i<m_inputMask.length(); i++) {
        c = m_inputMask.at(i);
        if (i > 0 && m_inputMask.at(i-1) == QLatin1Char('\\')) {
            m_maxLength++;
            continue;
        }
        if (c != QLatin1Char('\\') && c != QLatin1Char('!') &&
             c != QLatin1Char('<') && c != QLatin1Char('>') &&
             c != QLatin1Char('{') && c != QLatin1Char('}') &&
             c != QLatin1Char('[') && c != QLatin1Char(']'))
            m_maxLength++;
    }

    delete [] m_maskData;
    m_maskData = new MaskInputData[m_maxLength];

    MaskInputData::Casemode m = MaskInputData::NoCaseMode;
    c = 0;
    bool s;
    bool escape = false;
    int index = 0;
    for (int i = 0; i < m_inputMask.length(); i++) {
        c = m_inputMask.at(i);
        if (escape) {
            s = true;
            m_maskData[index].maskChar = c;
            m_maskData[index].separator = s;
            m_maskData[index].caseMode = m;
            index++;
            escape = false;
        } else if (c == QLatin1Char('<')) {
                m = MaskInputData::Lower;
        } else if (c == QLatin1Char('>')) {
            m = MaskInputData::Upper;
        } else if (c == QLatin1Char('!')) {
            m = MaskInputData::NoCaseMode;
        } else if (c != QLatin1Char('{') && c != QLatin1Char('}') && c != QLatin1Char('[') && c != QLatin1Char(']')) {
            switch (c.unicode()) {
            case 'A':
            case 'a':
            case 'N':
            case 'n':
            case 'X':
            case 'x':
            case '9':
            case '0':
            case 'D':
            case 'd':
            case '#':
            case 'H':
            case 'h':
            case 'B':
            case 'b':
                s = false;
                break;
            case '\\':
                escape = true;
            default:
                s = true;
                break;
            }

            if (!escape) {
                m_maskData[index].maskChar = c;
                m_maskData[index].separator = s;
                m_maskData[index].caseMode = m;
                index++;
            }
        }
    }
    internalSetText(m_text);
}


/*!
    \internal

    checks if the key is valid compared to the inputMask
*/
bool QQuickTextInputPrivate::isValidInput(QChar key, QChar mask) const
{
    switch (mask.unicode()) {
    case 'A':
        if (key.isLetter())
            return true;
        break;
    case 'a':
        if (key.isLetter() || key == m_blank)
            return true;
        break;
    case 'N':
        if (key.isLetterOrNumber())
            return true;
        break;
    case 'n':
        if (key.isLetterOrNumber() || key == m_blank)
            return true;
        break;
    case 'X':
        if (key.isPrint())
            return true;
        break;
    case 'x':
        if (key.isPrint() || key == m_blank)
            return true;
        break;
    case '9':
        if (key.isNumber())
            return true;
        break;
    case '0':
        if (key.isNumber() || key == m_blank)
            return true;
        break;
    case 'D':
        if (key.isNumber() && key.digitValue() > 0)
            return true;
        break;
    case 'd':
        if ((key.isNumber() && key.digitValue() > 0) || key == m_blank)
            return true;
        break;
    case '#':
        if (key.isNumber() || key == QLatin1Char('+') || key == QLatin1Char('-') || key == m_blank)
            return true;
        break;
    case 'B':
        if (key == QLatin1Char('0') || key == QLatin1Char('1'))
            return true;
        break;
    case 'b':
        if (key == QLatin1Char('0') || key == QLatin1Char('1') || key == m_blank)
            return true;
        break;
    case 'H':
        if (key.isNumber() || (key >= QLatin1Char('a') && key <= QLatin1Char('f')) || (key >= QLatin1Char('A') && key <= QLatin1Char('F')))
            return true;
        break;
    case 'h':
        if (key.isNumber() || (key >= QLatin1Char('a') && key <= QLatin1Char('f')) || (key >= QLatin1Char('A') && key <= QLatin1Char('F')) || key == m_blank)
            return true;
        break;
    default:
        break;
    }
    return false;
}

/*!
    \internal

    Returns true if the given text \a str is valid for any
    validator or input mask set for the line control.

    Otherwise returns false
*/
QQuickTextInputPrivate::ValidatorState QQuickTextInputPrivate::hasAcceptableInput(const QString &str) const
{
#ifndef QT_NO_VALIDATOR
    QString textCopy = str;
    int cursorCopy = m_cursor;
    if (m_validator) {
        QValidator::State state = m_validator->validate(textCopy, cursorCopy);
        if (state != QValidator::Acceptable)
            return ValidatorState(state);
    }
#endif

    if (!m_maskData)
        return AcceptableInput;

    if (str.length() != m_maxLength)
        return InvalidInput;

    for (int i=0; i < m_maxLength; ++i) {
        if (m_maskData[i].separator) {
            if (str.at(i) != m_maskData[i].maskChar)
                return InvalidInput;
        } else {
            if (!isValidInput(str.at(i), m_maskData[i].maskChar))
                return InvalidInput;
        }
    }
    return AcceptableInput;
}

/*!
    \internal

    Applies the inputMask on \a str starting from position \a pos in the mask. \a clear
    specifies from where characters should be gotten when a separator is met in \a str - true means
    that blanks will be used, false that previous input is used.
    Calling this when no inputMask is set is undefined.
*/
QString QQuickTextInputPrivate::maskString(uint pos, const QString &str, bool clear) const
{
    if (pos >= (uint)m_maxLength)
        return QString::fromLatin1("");

    QString fill;
    fill = clear ? clearString(0, m_maxLength) : m_text;

    int strIndex = 0;
    QString s = QString::fromLatin1("");
    int i = pos;
    while (i < m_maxLength) {
        if (strIndex < str.length()) {
            if (m_maskData[i].separator) {
                s += m_maskData[i].maskChar;
                if (str[(int)strIndex] == m_maskData[i].maskChar)
                    strIndex++;
                ++i;
            } else {
                if (isValidInput(str[(int)strIndex], m_maskData[i].maskChar)) {
                    switch (m_maskData[i].caseMode) {
                    case MaskInputData::Upper:
                        s += str[(int)strIndex].toUpper();
                        break;
                    case MaskInputData::Lower:
                        s += str[(int)strIndex].toLower();
                        break;
                    default:
                        s += str[(int)strIndex];
                    }
                    ++i;
                } else {
                    // search for separator first
                    int n = findInMask(i, true, true, str[(int)strIndex]);
                    if (n != -1) {
                        if (str.length() != 1 || i == 0 || (i > 0 && (!m_maskData[i-1].separator || m_maskData[i-1].maskChar != str[(int)strIndex]))) {
                            s += fill.mid(i, n-i+1);
                            i = n + 1; // update i to find + 1
                        }
                    } else {
                        // search for valid m_blank if not
                        n = findInMask(i, true, false, str[(int)strIndex]);
                        if (n != -1) {
                            s += fill.mid(i, n-i);
                            switch (m_maskData[n].caseMode) {
                            case MaskInputData::Upper:
                                s += str[(int)strIndex].toUpper();
                                break;
                            case MaskInputData::Lower:
                                s += str[(int)strIndex].toLower();
                                break;
                            default:
                                s += str[(int)strIndex];
                            }
                            i = n + 1; // updates i to find + 1
                        }
                    }
                }
                ++strIndex;
            }
        } else
            break;
    }

    return s;
}



/*!
    \internal

    Returns a "cleared" string with only separators and blank chars.
    Calling this when no inputMask is set is undefined.
*/
QString QQuickTextInputPrivate::clearString(uint pos, uint len) const
{
    if (pos >= (uint)m_maxLength)
        return QString();

    QString s;
    int end = qMin((uint)m_maxLength, pos + len);
    for (int i = pos; i < end; ++i)
        if (m_maskData[i].separator)
            s += m_maskData[i].maskChar;
        else
            s += m_blank;

    return s;
}

/*!
    \internal

    Strips blank parts of the input in a QQuickTextInputPrivate when an inputMask is set,
    separators are still included. Typically "127.0__.0__.1__" becomes "127.0.0.1".
*/
QString QQuickTextInputPrivate::stripString(const QString &str) const
{
    if (!m_maskData)
        return str;

    QString s;
    int end = qMin(m_maxLength, (int)str.length());
    for (int i = 0; i < end; ++i) {
        if (m_maskData[i].separator)
            s += m_maskData[i].maskChar;
        else if (str[i] != m_blank)
            s += str[i];
    }

    return s;
}

/*!
    \internal
    searches forward/backward in m_maskData for either a separator or a m_blank
*/
int QQuickTextInputPrivate::findInMask(int pos, bool forward, bool findSeparator, QChar searchChar) const
{
    if (pos >= m_maxLength || pos < 0)
        return -1;

    int end = forward ? m_maxLength : -1;
    int step = forward ? 1 : -1;
    int i = pos;

    while (i != end) {
        if (findSeparator) {
            if (m_maskData[i].separator && m_maskData[i].maskChar == searchChar)
                return i;
        } else {
            if (!m_maskData[i].separator) {
                if (searchChar.isNull())
                    return i;
                else if (isValidInput(searchChar, m_maskData[i].maskChar))
                    return i;
            }
        }
        i += step;
    }
    return -1;
}

void QQuickTextInputPrivate::internalUndo(int until)
{
    if (!isUndoAvailable())
        return;
    cancelPasswordEchoTimer();
    internalDeselect();
    while (m_undoState && m_undoState > until) {
        Command& cmd = m_history[--m_undoState];
        switch (cmd.type) {
        case Insert:
            m_text.remove(cmd.pos, 1);
            m_cursor = cmd.pos;
            break;
        case SetSelection:
            m_selstart = cmd.selStart;
            m_selend = cmd.selEnd;
            m_cursor = cmd.pos;
            break;
        case Remove:
        case RemoveSelection:
            m_text.insert(cmd.pos, cmd.uc);
            m_cursor = cmd.pos + 1;
            break;
        case Delete:
        case DeleteSelection:
            m_text.insert(cmd.pos, cmd.uc);
            m_cursor = cmd.pos;
            break;
        case Separator:
            continue;
        }
        if (until < 0 && m_undoState) {
            Command& next = m_history[m_undoState-1];
            if (next.type != cmd.type && next.type < RemoveSelection
                 && (cmd.type < RemoveSelection || next.type == Separator))
                break;
        }
    }
    m_textDirty = true;
}

void QQuickTextInputPrivate::internalRedo()
{
    if (!isRedoAvailable())
        return;
    internalDeselect();
    while (m_undoState < (int)m_history.size()) {
        Command& cmd = m_history[m_undoState++];
        switch (cmd.type) {
        case Insert:
            m_text.insert(cmd.pos, cmd.uc);
            m_cursor = cmd.pos + 1;
            break;
        case SetSelection:
            m_selstart = cmd.selStart;
            m_selend = cmd.selEnd;
            m_cursor = cmd.pos;
            break;
        case Remove:
        case Delete:
        case RemoveSelection:
        case DeleteSelection:
            m_text.remove(cmd.pos, 1);
            m_selstart = cmd.selStart;
            m_selend = cmd.selEnd;
            m_cursor = cmd.pos;
            break;
        case Separator:
            m_selstart = cmd.selStart;
            m_selend = cmd.selEnd;
            m_cursor = cmd.pos;
            break;
        }
        if (m_undoState < (int)m_history.size()) {
            Command& next = m_history[m_undoState];
            if (next.type != cmd.type && cmd.type < RemoveSelection && next.type != Separator
                 && (next.type < RemoveSelection || cmd.type == Separator))
                break;
        }
    }
    m_textDirty = true;
}

void QQuickTextInputPrivate::emitUndoRedoChanged()
{
    Q_Q(QQuickTextInput);
    const bool previousUndo = canUndo;
    const bool previousRedo = canRedo;

    canUndo = isUndoAvailable();
    canRedo = isRedoAvailable();

    if (previousUndo != canUndo)
        emit q->canUndoChanged();
    if (previousRedo != canRedo)
        emit q->canRedoChanged();
}

/*!
    \internal

    If the current cursor position differs from the last emitted cursor
    position, emits cursorPositionChanged().
*/
bool QQuickTextInputPrivate::emitCursorPositionChanged()
{
    Q_Q(QQuickTextInput);
    if (m_cursor != m_lastCursorPos) {
        m_lastCursorPos = m_cursor;

        q->updateCursorRectangle();
        emit q->cursorPositionChanged();
        // XXX todo - not in 4.8?
    #if 0
        resetCursorBlinkTimer();
    #endif

        if (!hasSelectedText()) {
            if (lastSelectionStart != m_cursor) {
                lastSelectionStart = m_cursor;
                emit q->selectionStartChanged();
            }
            if (lastSelectionEnd != m_cursor) {
                lastSelectionEnd = m_cursor;
                emit q->selectionEndChanged();
            }
        }

#ifndef QT_NO_ACCESSIBILITY
        QAccessible::updateAccessibility(QAccessibleEvent(QAccessible::TextCaretMoved, q, 0));
#endif

        return true;
    }
    return false;
}


void QQuickTextInputPrivate::setCursorBlinkPeriod(int msec)
{
    Q_Q(QQuickTextInput);
    if (msec == m_blinkPeriod)
        return;
    if (m_blinkTimer) {
        q->killTimer(m_blinkTimer);
    }
    if (msec) {
        m_blinkTimer = q->startTimer(msec / 2);
        m_blinkStatus = 1;
    } else {
        m_blinkTimer = 0;
        if (m_blinkStatus == 1) {
            updateType = UpdatePaintNode;
            q->update();
        }
    }
    m_blinkPeriod = msec;
}

void QQuickTextInputPrivate::resetCursorBlinkTimer()
{
    Q_Q(QQuickTextInput);
    if (m_blinkPeriod == 0 || m_blinkTimer == 0)
        return;
    q->killTimer(m_blinkTimer);
    m_blinkTimer = q->startTimer(m_blinkPeriod / 2);
    m_blinkStatus = 1;
}

void QQuickTextInput::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickTextInput);
    if (event->timerId() == d->m_blinkTimer) {
        d->m_blinkStatus = !d->m_blinkStatus;
        d->updateType = QQuickTextInputPrivate::UpdatePaintNode;
        update();
    } else if (event->timerId() == d->m_deleteAllTimer) {
        killTimer(d->m_deleteAllTimer);
        d->m_deleteAllTimer = 0;
        d->clear();
#ifdef QT_GUI_PASSWORD_ECHO_DELAY
    } else if (event->timerId() == d->m_passwordEchoTimer.timerId()) {
        d->m_passwordEchoTimer.stop();
        d->updateDisplayText();
#endif
    }
}

void QQuickTextInputPrivate::processKeyEvent(QKeyEvent* event)
{
    Q_Q(QQuickTextInput);
    bool inlineCompletionAccepted = false;

    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        if (hasAcceptableInput(m_text) || fixup()) {
            emit q->accepted();
        }
        if (inlineCompletionAccepted)
            event->accept();
        else
            event->ignore();
        return;
    }

    if (m_echoMode == QQuickTextInput::PasswordEchoOnEdit
        && !m_passwordEchoEditing
        && !m_readOnly
        && !event->text().isEmpty()
        && !(event->modifiers() & Qt::ControlModifier)) {
        // Clear the edit and reset to normal echo mode while editing; the
        // echo mode switches back when the edit loses focus
        // ### resets current content.  dubious code; you can
        // navigate with keys up, down, back, and select(?), but if you press
        // "left" or "right" it clears?
        updatePasswordEchoEditing(true);
        clear();
    }

    bool unknown = false;
    bool visual = cursorMoveStyle() == Qt::VisualMoveStyle;

    if (false) {
    }
#ifndef QT_NO_SHORTCUT
    else if (event == QKeySequence::Undo) {
        if (!m_readOnly)
            q->undo();
    }
    else if (event == QKeySequence::Redo) {
        if (!m_readOnly)
            q->redo();
    }
    else if (event == QKeySequence::SelectAll) {
        selectAll();
    }
#ifndef QT_NO_CLIPBOARD
    else if (event == QKeySequence::Copy) {
        copy();
    }
    else if (event == QKeySequence::Paste) {
        if (!m_readOnly) {
            QClipboard::Mode mode = QClipboard::Clipboard;
            paste(mode);
        }
    }
    else if (event == QKeySequence::Cut) {
        if (!m_readOnly) {
            copy();
            del();
        }
    }
    else if (event == QKeySequence::DeleteEndOfLine) {
        if (!m_readOnly) {
            setSelection(m_cursor, end());
            copy();
            del();
        }
    }
#endif //QT_NO_CLIPBOARD
    else if (event == QKeySequence::MoveToStartOfLine || event == QKeySequence::MoveToStartOfBlock) {
        home(0);
    }
    else if (event == QKeySequence::MoveToEndOfLine || event == QKeySequence::MoveToEndOfBlock) {
        end(0);
    }
    else if (event == QKeySequence::SelectStartOfLine || event == QKeySequence::SelectStartOfBlock) {
        home(1);
    }
    else if (event == QKeySequence::SelectEndOfLine || event == QKeySequence::SelectEndOfBlock) {
        end(1);
    }
    else if (event == QKeySequence::MoveToNextChar) {
        if (hasSelectedText()) {
            moveCursor(selectionEnd(), false);
        } else {
            cursorForward(0, visual ? 1 : (layoutDirection() == Qt::LeftToRight ? 1 : -1));
        }
    }
    else if (event == QKeySequence::SelectNextChar) {
        cursorForward(1, visual ? 1 : (layoutDirection() == Qt::LeftToRight ? 1 : -1));
    }
    else if (event == QKeySequence::MoveToPreviousChar) {
        if (hasSelectedText()) {
            moveCursor(selectionStart(), false);
        } else {
            cursorForward(0, visual ? -1 : (layoutDirection() == Qt::LeftToRight ? -1 : 1));
        }
    }
    else if (event == QKeySequence::SelectPreviousChar) {
        cursorForward(1, visual ? -1 : (layoutDirection() == Qt::LeftToRight ? -1 : 1));
    }
    else if (event == QKeySequence::MoveToNextWord) {
        if (m_echoMode == QQuickTextInput::Normal)
            layoutDirection() == Qt::LeftToRight ? cursorWordForward(0) : cursorWordBackward(0);
        else
            layoutDirection() == Qt::LeftToRight ? end(0) : home(0);
    }
    else if (event == QKeySequence::MoveToPreviousWord) {
        if (m_echoMode == QQuickTextInput::Normal)
            layoutDirection() == Qt::LeftToRight ? cursorWordBackward(0) : cursorWordForward(0);
        else if (!m_readOnly) {
            layoutDirection() == Qt::LeftToRight ? home(0) : end(0);
        }
    }
    else if (event == QKeySequence::SelectNextWord) {
        if (m_echoMode == QQuickTextInput::Normal)
            layoutDirection() == Qt::LeftToRight ? cursorWordForward(1) : cursorWordBackward(1);
        else
            layoutDirection() == Qt::LeftToRight ? end(1) : home(1);
    }
    else if (event == QKeySequence::SelectPreviousWord) {
        if (m_echoMode == QQuickTextInput::Normal)
            layoutDirection() == Qt::LeftToRight ? cursorWordBackward(1) : cursorWordForward(1);
        else
            layoutDirection() == Qt::LeftToRight ? home(1) : end(1);
    }
    else if (event == QKeySequence::Delete) {
        if (!m_readOnly)
            del();
    }
    else if (event == QKeySequence::DeleteEndOfWord) {
        if (!m_readOnly) {
            cursorWordForward(true);
            del();
        }
    }
    else if (event == QKeySequence::DeleteStartOfWord) {
        if (!m_readOnly) {
            cursorWordBackward(true);
            del();
        }
    }
#endif // QT_NO_SHORTCUT
    else {
        bool handled = false;
        if (event->modifiers() & Qt::ControlModifier) {
            switch (event->key()) {
            case Qt::Key_Backspace:
                if (!m_readOnly) {
                    cursorWordBackward(true);
                    del();
                }
                break;
            default:
                if (!handled)
                    unknown = true;
            }
        } else { // ### check for *no* modifier
            switch (event->key()) {
            case Qt::Key_Backspace:
                if (!m_readOnly) {
                    backspace();
                }
                break;
            default:
                if (!handled)
                    unknown = true;
            }
        }
    }

    if (event->key() == Qt::Key_Direction_L || event->key() == Qt::Key_Direction_R) {
        setLayoutDirection((event->key() == Qt::Key_Direction_L) ? Qt::LeftToRight : Qt::RightToLeft);
        unknown = false;
    }

    if (unknown && !m_readOnly) {
        QString t = event->text();
        if (!t.isEmpty() && t.at(0).isPrint()) {
            insert(t);
            event->accept();
            return;
        }
    }

    if (unknown)
        event->ignore();
    else
        event->accept();
}


QT_END_NAMESPACE

