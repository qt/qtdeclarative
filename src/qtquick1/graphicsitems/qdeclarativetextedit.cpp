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

#include "private/qdeclarativetextedit_p.h"
#include "private/qdeclarativetextedit_p_p.h"

#include "private/qdeclarativeevents_p_p.h"
#include <private/qdeclarativeglobal_p.h>
#include <qdeclarativeinfo.h>

#include <QtCore/qmath.h>

#include <private/qtextengine_p.h>
#include <QTextLayout>
#include <QTextLine>
#include <QTextDocument>
#include <QTextObject>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QPainter>
#include <QtGui/QInputPanel>

#include <private/qwidgettextcontrol_p.h>

QT_BEGIN_NAMESPACE



/*!
    \qmlclass TextEdit QDeclarative1TextEdit
    \inqmlmodule QtQuick 1
    \ingroup qml-basic-visual-elements
    \since QtQuick 1.0
    \brief The TextEdit item displays multiple lines of editable formatted text.
    \inherits Item

    The TextEdit item displays a block of editable, formatted text.

    It can display both plain and rich text. For example:

    \qml
TextEdit {
    width: 240
    text: "<b>Hello</b> <i>World!</i>"
    font.family: "Helvetica"
    font.pointSize: 20
    color: "blue"
    focus: true
}
    \endqml

    \image declarative-textedit.gif

    Setting \l {Item::focus}{focus} to \c true enables the TextEdit item to receive keyboard focus.

    Note that the TextEdit does not implement scrolling, following the cursor, or other behaviors specific
    to a look-and-feel. For example, to add flickable scrolling that follows the cursor:

    \snippet snippets/declarative/texteditor.qml 0

    A particular look-and-feel might use smooth scrolling (eg. using SmoothedFollow), might have a visible
    scrollbar, or a scrollbar that fades in to show location, etc.

    Clipboard support is provided by the cut(), copy(), and paste() functions, and the selection can
    be handled in a traditional "mouse" mechanism by setting selectByMouse, or handled completely
    from QML by manipulating selectionStart and selectionEnd, or using selectAll() or selectWord().

    You can translate between cursor positions (characters from the start of the document) and pixel
    points using positionAt() and positionToRectangle().

    \sa Text, TextInput, {declarative/text/textselection}{Text Selection example}
*/

/*!
    \qmlsignal QtQuick1::TextEdit::onLinkActivated(string link)
    \since Quick 1.1

    This handler is called when the user clicks on a link embedded in the text.
    The link must be in rich text or HTML format and the
    \a link string provides access to the particular link.
*/
QDeclarative1TextEdit::QDeclarative1TextEdit(QDeclarativeItem *parent)
: QDeclarative1ImplicitSizePaintedItem(*(new QDeclarative1TextEditPrivate), parent)
{
    Q_D(QDeclarative1TextEdit);
    d->init();
}

QString QDeclarative1TextEdit::text() const
{
    Q_D(const QDeclarative1TextEdit);

#ifndef QT_NO_TEXTHTMLPARSER
    if (d->richText)
        return d->document->toHtml();
    else
#endif
        return d->document->toPlainText();
}

/*!
    \qmlproperty string QtQuick1::TextEdit::font.family

    Sets the family name of the font.

    The family name is case insensitive and may optionally include a foundry name, e.g. "Helvetica [Cronyx]".
    If the family is available from more than one foundry and the foundry isn't specified, an arbitrary foundry is chosen.
    If the family isn't available a family will be set using the font matching algorithm.
*/

/*!
    \qmlproperty bool QtQuick1::TextEdit::font.bold

    Sets whether the font weight is bold.
*/

/*!
    \qmlproperty enumeration QtQuick1::TextEdit::font.weight

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
    TextEdit { text: "Hello"; font.weight: Font.DemiBold }
    \endqml
*/

/*!
    \qmlproperty bool QtQuick1::TextEdit::font.italic

    Sets whether the font has an italic style.
*/

/*!
    \qmlproperty bool QtQuick1::TextEdit::font.underline

    Sets whether the text is underlined.
*/

/*!
    \qmlproperty bool QtQuick1::TextEdit::font.strikeout

    Sets whether the font has a strikeout style.
*/

/*!
    \qmlproperty real QtQuick1::TextEdit::font.pointSize

    Sets the font size in points. The point size must be greater than zero.
*/

/*!
    \qmlproperty int QtQuick1::TextEdit::font.pixelSize

    Sets the font size in pixels.

    Using this function makes the font device dependent.  Use
    \l{TextEdit::font.pointSize} to set the size of the font in a
    device independent manner.
*/

/*!
    \qmlproperty real QtQuick1::TextEdit::font.letterSpacing

    Sets the letter spacing for the font.

    Letter spacing changes the default spacing between individual letters in the font.
    A positive value increases the letter spacing by the corresponding pixels; a negative value decreases the spacing.
*/

/*!
    \qmlproperty real QtQuick1::TextEdit::font.wordSpacing

    Sets the word spacing for the font.

    Word spacing changes the default spacing between individual words.
    A positive value increases the word spacing by a corresponding amount of pixels,
    while a negative value decreases the inter-word spacing accordingly.
*/

/*!
    \qmlproperty enumeration QtQuick1::TextEdit::font.capitalization

    Sets the capitalization for the text.

    \list
    \o Font.MixedCase - This is the normal text rendering option where no capitalization change is applied.
    \o Font.AllUppercase - This alters the text to be rendered in all uppercase type.
    \o Font.AllLowercase	 - This alters the text to be rendered in all lowercase type.
    \o Font.SmallCaps -	This alters the text to be rendered in small-caps type.
    \o Font.Capitalize - This alters the text to be rendered with the first character of each word as an uppercase character.
    \endlist

    \qml
    TextEdit { text: "Hello"; font.capitalization: Font.AllLowercase }
    \endqml
*/

/*!
    \qmlproperty string QtQuick1::TextEdit::text

    The text to display.  If the text format is AutoText the text edit will
    automatically determine whether the text should be treated as
    rich text.  This determination is made using Qt::mightBeRichText().
*/
void QDeclarative1TextEdit::setText(const QString &text)
{
    Q_D(QDeclarative1TextEdit);
    if (QDeclarative1TextEdit::text() == text)
        return;

    d->richText = d->format == RichText || (d->format == AutoText && Qt::mightBeRichText(text));
    if (d->richText) {
#ifndef QT_NO_TEXTHTMLPARSER
        d->control->setHtml(text);
#else
        d->control->setPlainText(text);
#endif
    } else {
        d->control->setPlainText(text);
    }
    q_textChanged();
}

/*!
    \qmlproperty enumeration QtQuick1::TextEdit::textFormat

    The way the text property should be displayed.

    \list
    \o TextEdit.AutoText
    \o TextEdit.PlainText
    \o TextEdit.RichText
    \endlist

    The default is TextEdit.AutoText.  If the text format is TextEdit.AutoText the text edit
    will automatically determine whether the text should be treated as
    rich text.  This determination is made using Qt::mightBeRichText().

    \table
    \row
    \o
    \qml
Column {
    TextEdit {
        font.pointSize: 24
        text: "<b>Hello</b> <i>World!</i>"
    }
    TextEdit {
        font.pointSize: 24
        textFormat: TextEdit.RichText
        text: "<b>Hello</b> <i>World!</i>"
    }
    TextEdit {
        font.pointSize: 24
        textFormat: TextEdit.PlainText
        text: "<b>Hello</b> <i>World!</i>"
    }
}
    \endqml
    \o \image declarative-textformat.png
    \endtable
*/
QDeclarative1TextEdit::TextFormat QDeclarative1TextEdit::textFormat() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->format;
}

void QDeclarative1TextEdit::setTextFormat(TextFormat format)
{
    Q_D(QDeclarative1TextEdit);
    if (format == d->format)
        return;
    bool wasRich = d->richText;
    d->richText = format == RichText || (format == AutoText && Qt::mightBeRichText(d->text));

    if (wasRich && !d->richText) {
        d->control->setPlainText(d->text);
        updateSize();
    } else if (!wasRich && d->richText) {
#ifndef QT_NO_TEXTHTMLPARSER
        d->control->setHtml(d->text);
#else
        d->control->setPlainText(d->text);
#endif
        updateSize();
    }
    d->format = format;
    d->control->setAcceptRichText(d->format != PlainText);
    emit textFormatChanged(d->format);
}

QFont QDeclarative1TextEdit::font() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->sourceFont;
}

void QDeclarative1TextEdit::setFont(const QFont &font)
{
    Q_D(QDeclarative1TextEdit);
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
        clearCache();
        d->document->setDefaultFont(d->font);
        if(d->cursor){
            d->cursor->setHeight(QFontMetrics(d->font).height());
            moveCursorDelegate();
        }
        updateSize();
        update();
    }
    emit fontChanged(d->sourceFont);
}

/*!
    \qmlproperty color QtQuick1::TextEdit::color

    The text color.

    \qml
    // green text using hexadecimal notation
    TextEdit { color: "#00FF00" }
    \endqml

    \qml
    // steelblue text using SVG color name
    TextEdit { color: "steelblue" }
    \endqml
*/
QColor QDeclarative1TextEdit::color() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->color;
}

void QDeclarative1TextEdit::setColor(const QColor &color)
{
    Q_D(QDeclarative1TextEdit);
    if (d->color == color)
        return;

    clearCache();
    d->color = color;
    QPalette pal = d->control->palette();
    pal.setColor(QPalette::Text, color);
    d->control->setPalette(pal);
    update();
    emit colorChanged(d->color);
}

/*!
    \qmlproperty color QtQuick1::TextEdit::selectionColor

    The text highlight color, used behind selections.
*/
QColor QDeclarative1TextEdit::selectionColor() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->selectionColor;
}

void QDeclarative1TextEdit::setSelectionColor(const QColor &color)
{
    Q_D(QDeclarative1TextEdit);
    if (d->selectionColor == color)
        return;

    clearCache();
    d->selectionColor = color;
    QPalette pal = d->control->palette();
    pal.setColor(QPalette::Highlight, color);
    d->control->setPalette(pal);
    update();
    emit selectionColorChanged(d->selectionColor);
}

/*!
    \qmlproperty color QtQuick1::TextEdit::selectedTextColor

    The selected text color, used in selections.
*/
QColor QDeclarative1TextEdit::selectedTextColor() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->selectedTextColor;
}

void QDeclarative1TextEdit::setSelectedTextColor(const QColor &color)
{
    Q_D(QDeclarative1TextEdit);
    if (d->selectedTextColor == color)
        return;

    clearCache();
    d->selectedTextColor = color;
    QPalette pal = d->control->palette();
    pal.setColor(QPalette::HighlightedText, color);
    d->control->setPalette(pal);
    update();
    emit selectedTextColorChanged(d->selectedTextColor);
}

/*!
    \qmlproperty enumeration QtQuick1::TextEdit::horizontalAlignment
    \qmlproperty enumeration QtQuick1::TextEdit::verticalAlignment
    \qmlproperty enumeration QtQuick1::TextEdit::effectiveHorizontalAlignment

    Sets the horizontal and vertical alignment of the text within the TextEdit item's
    width and height. By default, the text alignment follows the natural alignment
    of the text, for example text that is read from left to right will be aligned to
    the left.

    Valid values for \c horizontalAlignment are:
    \list
    \o TextEdit.AlignLeft (default)
    \o TextEdit.AlignRight 
    \o TextEdit.AlignHCenter
    \o TextEdit.AlignJustify
    \endlist
    
    Valid values for \c verticalAlignment are:
    \list
    \o TextEdit.AlignTop (default)
    \o TextEdit.AlignBottom
    \o TextEdit.AlignVCenter
    \endlist

    When using the attached property LayoutMirroring::enabled to mirror application
    layouts, the horizontal alignment of text will also be mirrored. However, the property
    \c horizontalAlignment will remain unchanged. To query the effective horizontal alignment
    of TextEdit, use the read-only property \c effectiveHorizontalAlignment.
*/
QDeclarative1TextEdit::HAlignment QDeclarative1TextEdit::hAlign() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->hAlign;
}

void QDeclarative1TextEdit::setHAlign(HAlignment align)
{
    Q_D(QDeclarative1TextEdit);
    bool forceAlign = d->hAlignImplicit && d->effectiveLayoutMirror;
    d->hAlignImplicit = false;
    if (d->setHAlign(align, forceAlign) && isComponentComplete()) {
        d->updateDefaultTextOption();
        updateSize();
    }
}

void QDeclarative1TextEdit::resetHAlign()
{
    Q_D(QDeclarative1TextEdit);
    d->hAlignImplicit = true;
    if (d->determineHorizontalAlignment() && isComponentComplete()) {
        d->updateDefaultTextOption();
        updateSize();
    }
}

QDeclarative1TextEdit::HAlignment QDeclarative1TextEdit::effectiveHAlign() const
{
    Q_D(const QDeclarative1TextEdit);
    QDeclarative1TextEdit::HAlignment effectiveAlignment = d->hAlign;
    if (!d->hAlignImplicit && d->effectiveLayoutMirror) {
        switch (d->hAlign) {
        case QDeclarative1TextEdit::AlignLeft:
            effectiveAlignment = QDeclarative1TextEdit::AlignRight;
            break;
        case QDeclarative1TextEdit::AlignRight:
            effectiveAlignment = QDeclarative1TextEdit::AlignLeft;
            break;
        default:
            break;
        }
    }
    return effectiveAlignment;
}

bool QDeclarative1TextEditPrivate::setHAlign(QDeclarative1TextEdit::HAlignment alignment, bool forceAlign)
{
    Q_Q(QDeclarative1TextEdit);
    if (hAlign != alignment || forceAlign) {
        QDeclarative1TextEdit::HAlignment oldEffectiveHAlign = q->effectiveHAlign();
        hAlign = alignment;
        emit q->horizontalAlignmentChanged(alignment);
        if (oldEffectiveHAlign != q->effectiveHAlign())
            emit q->effectiveHorizontalAlignmentChanged();
        return true;
    }
    return false;
}

bool QDeclarative1TextEditPrivate::determineHorizontalAlignment()
{
    Q_Q(QDeclarative1TextEdit);
    if (hAlignImplicit && q->isComponentComplete()) {
        bool alignToRight;
        if (text.isEmpty()) {
            QTextCursor cursor = control->textCursor();
            const QString preeditText = cursor.block().isValid()
                    ? control->textCursor().block().layout()->preeditAreaText()
                    : QString();
            alignToRight = preeditText.isEmpty()
                    ? QApplication::keyboardInputDirection() == Qt::RightToLeft
                    : preeditText.isRightToLeft();
        } else {
            alignToRight = rightToLeftText;
        }
        return setHAlign(alignToRight ? QDeclarative1TextEdit::AlignRight : QDeclarative1TextEdit::AlignLeft);
    }
    return false;
}

void QDeclarative1TextEditPrivate::mirrorChange()
{
    Q_Q(QDeclarative1TextEdit);
    if (q->isComponentComplete()) {
        if (!hAlignImplicit && (hAlign == QDeclarative1TextEdit::AlignRight || hAlign == QDeclarative1TextEdit::AlignLeft)) {
            updateDefaultTextOption();
            q->updateSize();
            emit q->effectiveHorizontalAlignmentChanged();
        }
    }
}

QDeclarative1TextEdit::VAlignment QDeclarative1TextEdit::vAlign() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->vAlign;
}

void QDeclarative1TextEdit::setVAlign(QDeclarative1TextEdit::VAlignment alignment)
{
    Q_D(QDeclarative1TextEdit);
    if (alignment == d->vAlign)
        return;
    d->vAlign = alignment;
    d->updateDefaultTextOption();
    updateSize();
    moveCursorDelegate();
    emit verticalAlignmentChanged(d->vAlign);
}

/*!
    \qmlproperty enumeration QtQuick1::TextEdit::wrapMode

    Set this property to wrap the text to the TextEdit item's width.
    The text will only wrap if an explicit width has been set.

    \list
    \o TextEdit.NoWrap - no wrapping will be performed. If the text contains insufficient newlines, then implicitWidth will exceed a set width.
    \o TextEdit.WordWrap - wrapping is done on word boundaries only. If a word is too long, implicitWidth will exceed a set width.
    \o TextEdit.WrapAnywhere - wrapping is done at any point on a line, even if it occurs in the middle of a word.
    \o TextEdit.Wrap - if possible, wrapping occurs at a word boundary; otherwise it will occur at the appropriate point on the line, even in the middle of a word.
    \endlist

    The default is TextEdit.NoWrap. If you set a width, consider using TextEdit.Wrap.
*/
QDeclarative1TextEdit::WrapMode QDeclarative1TextEdit::wrapMode() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->wrapMode;
}

void QDeclarative1TextEdit::setWrapMode(WrapMode mode)
{
    Q_D(QDeclarative1TextEdit);
    if (mode == d->wrapMode)
        return;
    d->wrapMode = mode;
    d->updateDefaultTextOption();
    updateSize();
    emit wrapModeChanged();
}

/*!
    \qmlproperty int QtQuick1::TextEdit::lineCount
    \since Quick 1.1

    Returns the total number of lines in the textEdit item.
*/
int QDeclarative1TextEdit::lineCount() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->lineCount;
}

/*!
    \qmlproperty real QtQuick1::TextEdit::paintedWidth

    Returns the width of the text, including the width past the width
    which is covered due to insufficient wrapping if \l wrapMode is set.
*/
qreal QDeclarative1TextEdit::paintedWidth() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->paintedSize.width();
}

/*!
    \qmlproperty real QtQuick1::TextEdit::paintedHeight

    Returns the height of the text, including the height past the height
    that is covered if the text does not fit within the set height.
*/
qreal QDeclarative1TextEdit::paintedHeight() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->paintedSize.height();
}

/*!
    \qmlmethod rectangle QtQuick1::TextEdit::positionToRectangle(position)

    Returns the rectangle at the given \a position in the text. The x, y,
    and height properties correspond to the cursor that would describe
    that position.
*/
QRectF QDeclarative1TextEdit::positionToRectangle(int pos) const
{
    Q_D(const QDeclarative1TextEdit);
    QTextCursor c(d->document);
    c.setPosition(pos);
    return d->control->cursorRect(c);

}

/*!
    \qmlmethod int QtQuick1::TextEdit::positionAt(int x, int y)

    Returns the text position closest to pixel position (\a x, \a y).

    Position 0 is before the first character, position 1 is after the first character
    but before the second, and so on until position \l {text}.length, which is after all characters.
*/
int QDeclarative1TextEdit::positionAt(int x, int y) const
{
    Q_D(const QDeclarative1TextEdit);
    int r = d->document->documentLayout()->hitTest(QPoint(x,y-d->yoff), Qt::FuzzyHit);
    QTextCursor cursor = d->control->textCursor();
    if (r > cursor.position()) {
        // The cursor position includes positions within the preedit text, but only positions in the
        // same text block are offset so it is possible to get a position that is either part of the
        // preedit or the next text block.
        QTextLayout *layout = cursor.block().layout();
        const int preeditLength = layout
                ? layout->preeditAreaText().length()
                : 0;
        if (preeditLength > 0
                && d->document->documentLayout()->blockBoundingRect(cursor.block()).contains(x,y-d->yoff)) {
            r = r > cursor.position() + preeditLength
                    ? r - preeditLength
                    : cursor.position();
        }
    }
    return r;
}

void QDeclarative1TextEdit::moveCursorSelection(int pos)
{
    //Note that this is the same as setCursorPosition but with the KeepAnchor flag set
    Q_D(QDeclarative1TextEdit);
    QTextCursor cursor = d->control->textCursor();
    if (cursor.position() == pos)
        return;
    cursor.setPosition(pos, QTextCursor::KeepAnchor);
    d->control->setTextCursor(cursor);
}

/*!
    \qmlmethod void QtQuick1::TextEdit::moveCursorSelection(int position, SelectionMode mode = TextEdit.SelectCharacters)
    \since Quick 1.1

    Moves the cursor to \a position and updates the selection according to the optional \a mode
    parameter. (To only move the cursor, set the \l cursorPosition property.)

    When this method is called it additionally sets either the
    selectionStart or the selectionEnd (whichever was at the previous cursor position)
    to the specified position. This allows you to easily extend and contract the selected
    text range.

    The selection mode specifies whether the selection is updated on a per character or a per word
    basis.  If not specified the selection mode will default to TextEdit.SelectCharacters.

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
        moveCursorSelection(9, TextEdit.SelectCharacters)
        moveCursorSelection(7, TextEdit.SelectCharacters)
    \endcode

    This moves the cursor to position 5, extend the selection end from 5 to 9
    and then retract the selection end from 9 to 7, leaving the text from position 5 to 7
    selected (the 6th and 7th characters).

    The same sequence with TextEdit.SelectWords will extend the selection start to a word boundary
    before or on position 5 and extend the selection end to a word boundary on or past position 9.
*/
void QDeclarative1TextEdit::moveCursorSelection(int pos, SelectionMode mode)
{
    Q_D(QDeclarative1TextEdit);
    QTextCursor cursor = d->control->textCursor();
    if (cursor.position() == pos)
        return;
    if (mode == SelectCharacters) {
        cursor.setPosition(pos, QTextCursor::KeepAnchor);
    } else if (cursor.anchor() < pos || (cursor.anchor() == pos && cursor.position() < pos)) {
        if (cursor.anchor() > cursor.position()) {
            cursor.setPosition(cursor.anchor(), QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
            if (cursor.position() == cursor.anchor())
                cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor);
            else
                cursor.setPosition(cursor.position(), QTextCursor::MoveAnchor);
        } else {
            cursor.setPosition(cursor.anchor(), QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
        }

        cursor.setPosition(pos, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
        if (cursor.position() != pos)
            cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    } else if (cursor.anchor() > pos || (cursor.anchor() == pos && cursor.position() > pos)) {
        if (cursor.anchor() < cursor.position()) {
            cursor.setPosition(cursor.anchor(), QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::MoveAnchor);
        } else {
            cursor.setPosition(cursor.anchor(), QTextCursor::MoveAnchor);
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            if (cursor.position() != cursor.anchor()) {
                cursor.setPosition(cursor.anchor(), QTextCursor::MoveAnchor);
                cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::MoveAnchor);
            }
        }

        cursor.setPosition(pos, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        if (cursor.position() != pos) {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
        }
    }
    d->control->setTextCursor(cursor);
}

/*!
    \qmlproperty bool QtQuick1::TextEdit::cursorVisible
    If true the text edit shows a cursor.

    This property is set and unset when the text edit gets active focus, but it can also
    be set directly (useful, for example, if a KeyProxy might forward keys to it).
*/
bool QDeclarative1TextEdit::isCursorVisible() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->cursorVisible;
}

void QDeclarative1TextEdit::setCursorVisible(bool on)
{
    Q_D(QDeclarative1TextEdit);
    if (d->cursorVisible == on)
        return;
    d->cursorVisible = on;
    QFocusEvent focusEvent(on ? QEvent::FocusIn : QEvent::FocusOut);
    if (!on && !d->persistentSelection)
        d->control->setCursorIsFocusIndicator(true);
    d->control->processEvent(&focusEvent, QPointF(0, -d->yoff));
    emit cursorVisibleChanged(d->cursorVisible);
}

/*!
    \qmlproperty int QtQuick1::TextEdit::cursorPosition
    The position of the cursor in the TextEdit.
*/
int QDeclarative1TextEdit::cursorPosition() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->control->textCursor().position();
}

void QDeclarative1TextEdit::setCursorPosition(int pos)
{
    Q_D(QDeclarative1TextEdit);
    if (pos < 0 || pos > d->text.length())
        return;
    QTextCursor cursor = d->control->textCursor();
    if (cursor.position() == pos && cursor.anchor() == pos)
        return;
    cursor.setPosition(pos);
    d->control->setTextCursor(cursor);
}

/*!
    \qmlproperty Component QtQuick1::TextEdit::cursorDelegate
    The delegate for the cursor in the TextEdit.

    If you set a cursorDelegate for a TextEdit, this delegate will be used for
    drawing the cursor instead of the standard cursor. An instance of the
    delegate will be created and managed by the text edit when a cursor is
    needed, and the x and y properties of delegate instance will be set so as
    to be one pixel before the top left of the current character.

    Note that the root item of the delegate component must be a QDeclarativeItem or
    QDeclarativeItem derived item.
*/
QDeclarativeComponent* QDeclarative1TextEdit::cursorDelegate() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->cursorComponent;
}

void QDeclarative1TextEdit::setCursorDelegate(QDeclarativeComponent* c)
{
    Q_D(QDeclarative1TextEdit);
    if(d->cursorComponent){
        if(d->cursor){
            d->control->setCursorWidth(-1);
            dirtyCache(cursorRectangle());
            delete d->cursor;
            d->cursor = 0;
        }
    }
    d->cursorComponent = c;
    if(c && c->isReady()){
        loadCursorDelegate();
    }else{
        if(c)
            connect(c, SIGNAL(statusChanged()),
                    this, SLOT(loadCursorDelegate()));
    }

    emit cursorDelegateChanged();
}

void QDeclarative1TextEdit::loadCursorDelegate()
{
    Q_D(QDeclarative1TextEdit);
    if(d->cursorComponent->isLoading())
        return;
    d->cursor = qobject_cast<QDeclarativeItem*>(d->cursorComponent->create(qmlContext(this)));
    if(d->cursor){
        d->control->setCursorWidth(0);
        dirtyCache(cursorRectangle());
        QDeclarative_setParent_noEvent(d->cursor, this);
        d->cursor->setParentItem(this);
        d->cursor->setHeight(QFontMetrics(d->font).height());
        moveCursorDelegate();
    }else{
        qmlInfo(this) << "Error loading cursor delegate.";
    }
}

/*!
    \qmlproperty int QtQuick1::TextEdit::selectionStart

    The cursor position before the first character in the current selection.

    This property is read-only. To change the selection, use select(start,end),
    selectAll(), or selectWord().

    \sa selectionEnd, cursorPosition, selectedText
*/
int QDeclarative1TextEdit::selectionStart() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->control->textCursor().selectionStart();
}

/*!
    \qmlproperty int QtQuick1::TextEdit::selectionEnd

    The cursor position after the last character in the current selection.

    This property is read-only. To change the selection, use select(start,end),
    selectAll(), or selectWord().

    \sa selectionStart, cursorPosition, selectedText
*/
int QDeclarative1TextEdit::selectionEnd() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->control->textCursor().selectionEnd();
}

/*!
    \qmlproperty string QtQuick1::TextEdit::selectedText

    This read-only property provides the text currently selected in the
    text edit.

    It is equivalent to the following snippet, but is faster and easier
    to use.
    \code
    //myTextEdit is the id of the TextEdit
    myTextEdit.text.toString().substring(myTextEdit.selectionStart,
            myTextEdit.selectionEnd);
    \endcode
*/
QString QDeclarative1TextEdit::selectedText() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->control->textCursor().selectedText();
}

/*!
    \qmlproperty bool QtQuick1::TextEdit::activeFocusOnPress

    Whether the TextEdit should gain active focus on a mouse press. By default this is
    set to true.
*/
bool QDeclarative1TextEdit::focusOnPress() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->focusOnPress;
}

void QDeclarative1TextEdit::setFocusOnPress(bool on)
{
    Q_D(QDeclarative1TextEdit);
    if (d->focusOnPress == on)
        return;
    d->focusOnPress = on;
    emit activeFocusOnPressChanged(d->focusOnPress);
}

/*!
    \qmlproperty bool QtQuick1::TextEdit::persistentSelection

    Whether the TextEdit should keep the selection visible when it loses active focus to another
    item in the scene. By default this is set to true;
*/
bool QDeclarative1TextEdit::persistentSelection() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->persistentSelection;
}

void QDeclarative1TextEdit::setPersistentSelection(bool on)
{
    Q_D(QDeclarative1TextEdit);
    if (d->persistentSelection == on)
        return;
    d->persistentSelection = on;
    emit persistentSelectionChanged(d->persistentSelection);
}

/*
   \qmlproperty real QtQuick1::TextEdit::textMargin

   The margin, in pixels, around the text in the TextEdit.
*/
qreal QDeclarative1TextEdit::textMargin() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->textMargin;
}

void QDeclarative1TextEdit::setTextMargin(qreal margin)
{
    Q_D(QDeclarative1TextEdit);
    if (d->textMargin == margin)
        return;
    d->textMargin = margin;
    d->document->setDocumentMargin(d->textMargin);
    emit textMarginChanged(d->textMargin);
}

void QDeclarative1TextEdit::geometryChanged(const QRectF &newGeometry,
                                  const QRectF &oldGeometry)
{
    if (newGeometry.width() != oldGeometry.width())
        updateSize();
    QDeclarative1PaintedItem::geometryChanged(newGeometry, oldGeometry);
}

/*!
    Ensures any delayed caching or data loading the class
    needs to performed is complete.
*/
void QDeclarative1TextEdit::componentComplete()
{
    Q_D(QDeclarative1TextEdit);
    QDeclarative1PaintedItem::componentComplete();
    if (d->dirty) {
        d->determineHorizontalAlignment();
        d->updateDefaultTextOption();
        updateSize();
        d->dirty = false;
    }
}

/*!
    \qmlproperty bool QtQuick1::TextEdit::selectByMouse

    Defaults to false.

    If true, the user can use the mouse to select text in some
    platform-specific way. Note that for some platforms this may
    not be an appropriate interaction (eg. may conflict with how
    the text needs to behave inside a Flickable.
*/
bool QDeclarative1TextEdit::selectByMouse() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->selectByMouse;
}

void QDeclarative1TextEdit::setSelectByMouse(bool on)
{
    Q_D(QDeclarative1TextEdit);
    if (d->selectByMouse != on) {
        d->selectByMouse = on;
        setKeepMouseGrab(on);
        if (on)
            setTextInteractionFlags(d->control->textInteractionFlags() | Qt::TextSelectableByMouse);
        else
            setTextInteractionFlags(d->control->textInteractionFlags() & ~Qt::TextSelectableByMouse);
        emit selectByMouseChanged(on);
    }
}


/*!
    \qmlproperty enum QtQuick1::TextEdit::mouseSelectionMode
    \since Quick 1.1

    Specifies how text should be selected using a mouse.

    \list
    \o TextEdit.SelectCharacters - The selection is updated with individual characters. (Default)
    \o TextEdit.SelectWords - The selection is updated with whole words.
    \endlist

    This property only applies when \l selectByMouse is true.
*/

QDeclarative1TextEdit::SelectionMode QDeclarative1TextEdit::mouseSelectionMode() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->mouseSelectionMode;
}

void QDeclarative1TextEdit::setMouseSelectionMode(SelectionMode mode)
{
    Q_D(QDeclarative1TextEdit);
    if (d->mouseSelectionMode != mode) {
        d->mouseSelectionMode = mode;
        d->control->setWordSelectionEnabled(mode == SelectWords);
        emit mouseSelectionModeChanged(mode);
    }
}

/*!
    \qmlproperty bool QtQuick1::TextEdit::readOnly

    Whether the user can interact with the TextEdit item. If this
    property is set to true the text cannot be edited by user interaction.

    By default this property is false.
*/
void QDeclarative1TextEdit::setReadOnly(bool r)
{
    Q_D(QDeclarative1TextEdit);
    if (r == isReadOnly())
        return;

    setFlag(QGraphicsItem::ItemAcceptsInputMethod, !r);

    Qt::TextInteractionFlags flags = Qt::LinksAccessibleByMouse;
    if (d->selectByMouse)
        flags = flags | Qt::TextSelectableByMouse;
    if (!r)
        flags = flags | Qt::TextSelectableByKeyboard | Qt::TextEditable;
    d->control->setTextInteractionFlags(flags);
    if (!r)
        d->control->moveCursor(QTextCursor::End);

    emit readOnlyChanged(r);
}

bool QDeclarative1TextEdit::isReadOnly() const
{
    Q_D(const QDeclarative1TextEdit);
    return !(d->control->textInteractionFlags() & Qt::TextEditable);
}

/*!
    Sets how the text edit should interact with user input to the given
    \a flags.
*/
void QDeclarative1TextEdit::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    Q_D(QDeclarative1TextEdit);
    d->control->setTextInteractionFlags(flags);
}

/*!
    Returns the flags specifying how the text edit should interact
    with user input.
*/
Qt::TextInteractionFlags QDeclarative1TextEdit::textInteractionFlags() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->control->textInteractionFlags();
}

/*!
    \qmlproperty rectangle QtQuick1::TextEdit::cursorRectangle

    The rectangle where the text cursor is rendered
    within the text edit. Read-only.
*/
QRect QDeclarative1TextEdit::cursorRectangle() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->control->cursorRect().toRect().translated(0,d->yoff);
}


/*!
\overload
Handles the given \a event.
*/
bool QDeclarative1TextEdit::event(QEvent *event)
{
    Q_D(QDeclarative1TextEdit);
    if (event->type() == QEvent::ShortcutOverride) {
        d->control->processEvent(event, QPointF(0, -d->yoff));
        return event->isAccepted();
    }
    return QDeclarative1PaintedItem::event(event);
}

/*!
\overload
Handles the given key \a event.
*/
void QDeclarative1TextEdit::keyPressEvent(QKeyEvent *event)
{
    Q_D(QDeclarative1TextEdit);
    keyPressPreHandler(event);
    if (!event->isAccepted())
        d->control->processEvent(event, QPointF(0, -d->yoff));
    if (!event->isAccepted())
        QDeclarative1PaintedItem::keyPressEvent(event);
}

/*!
\overload
Handles the given key \a event.
*/
void QDeclarative1TextEdit::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QDeclarative1TextEdit);
    keyReleasePreHandler(event);
    if (!event->isAccepted())
        d->control->processEvent(event, QPointF(0, -d->yoff));
    if (!event->isAccepted())
        QDeclarative1PaintedItem::keyReleaseEvent(event);
}

void QDeclarative1TextEditPrivate::focusChanged(bool hasFocus)
{
    Q_Q(QDeclarative1TextEdit);
    q->setCursorVisible(hasFocus && scene && scene->hasFocus());
    QDeclarativeItemPrivate::focusChanged(hasFocus);
}

/*!
    \qmlmethod void QtQuick1::TextEdit::deselect()
    \since Quick 1.1

    Removes active text selection.
*/
void QDeclarative1TextEdit::deselect()
{
    Q_D(QDeclarative1TextEdit);
    QTextCursor c = d->control->textCursor();
    c.clearSelection();
    d->control->setTextCursor(c);
}

/*!
    \qmlmethod void QtQuick1::TextEdit::selectAll()

    Causes all text to be selected.
*/
void QDeclarative1TextEdit::selectAll()
{
    Q_D(QDeclarative1TextEdit);
    d->control->selectAll();
}

/*!
    \qmlmethod void QtQuick1::TextEdit::selectWord()

    Causes the word closest to the current cursor position to be selected.
*/
void QDeclarative1TextEdit::selectWord()
{
    Q_D(QDeclarative1TextEdit);
    QTextCursor c = d->control->textCursor();
    c.select(QTextCursor::WordUnderCursor);
    d->control->setTextCursor(c);
}

/*!
    \qmlmethod void QtQuick1::TextEdit::select(int start, int end)

    Causes the text from \a start to \a end to be selected.

    If either start or end is out of range, the selection is not changed.

    After calling this, selectionStart will become the lesser
    and selectionEnd will become the greater (regardless of the order passed
    to this method).

    \sa selectionStart, selectionEnd
*/
void QDeclarative1TextEdit::select(int start, int end)
{
    Q_D(QDeclarative1TextEdit);
    if (start < 0 || end < 0 || start > d->text.length() || end > d->text.length())
        return;
    QTextCursor cursor = d->control->textCursor();
    cursor.beginEditBlock();
    cursor.setPosition(start, QTextCursor::MoveAnchor);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.endEditBlock();
    d->control->setTextCursor(cursor);

    // QTBUG-11100
    updateSelectionMarkers();
}

/*!
    \qmlmethod void QtQuick1::TextEdit::isRightToLeft(int start, int end)

    Returns true if the natural reading direction of the editor text
    found between positions \a start and \a end is right to left.
*/
bool QDeclarative1TextEdit::isRightToLeft(int start, int end)
{
    Q_D(QDeclarative1TextEdit);
    if (start > end) {
        qmlInfo(this) << "isRightToLeft(start, end) called with the end property being smaller than the start.";
        return false;
    } else {
        return d->text.mid(start, end - start).isRightToLeft();
    }
}

#ifndef QT_NO_CLIPBOARD
/*!
    \qmlmethod QtQuick1::TextEdit::cut()

    Moves the currently selected text to the system clipboard.
*/
void QDeclarative1TextEdit::cut()
{
    Q_D(QDeclarative1TextEdit);
    d->control->cut();
}

/*!
    \qmlmethod QtQuick1::TextEdit::copy()

    Copies the currently selected text to the system clipboard.
*/
void QDeclarative1TextEdit::copy()
{
    Q_D(QDeclarative1TextEdit);
    d->control->copy();
}

/*!
    \qmlmethod QtQuick1::TextEdit::paste()

    Replaces the currently selected text by the contents of the system clipboard.
*/
void QDeclarative1TextEdit::paste()
{
    Q_D(QDeclarative1TextEdit);
    d->control->paste();
}
#endif // QT_NO_CLIPBOARD

/*!
\overload
Handles the given mouse \a event.
*/
void QDeclarative1TextEdit::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1TextEdit);
    if (d->focusOnPress){
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

    d->control->processEvent(event, QPointF(0, -d->yoff));
    if (!event->isAccepted())
        QDeclarative1PaintedItem::mousePressEvent(event);
}

/*!
\overload
Handles the given mouse \a event.
*/
void QDeclarative1TextEdit::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1TextEdit);
    d->control->processEvent(event, QPointF(0, -d->yoff));
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

    if (!event->isAccepted())
        QDeclarative1PaintedItem::mouseReleaseEvent(event);
}

/*!
\overload
Handles the given mouse \a event.
*/
void QDeclarative1TextEdit::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1TextEdit);

    d->control->processEvent(event, QPointF(0, -d->yoff));
    if (!event->isAccepted())
        QDeclarative1PaintedItem::mouseDoubleClickEvent(event);

}

/*!
\overload
Handles the given mouse \a event.
*/
void QDeclarative1TextEdit::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(QDeclarative1TextEdit);
    d->control->processEvent(event, QPointF(0, -d->yoff));
    if (!event->isAccepted())
        QDeclarative1PaintedItem::mouseMoveEvent(event);
}

/*!
\overload
Handles the given input method \a event.
*/
void QDeclarative1TextEdit::inputMethodEvent(QInputMethodEvent *event)
{
    Q_D(QDeclarative1TextEdit);
    const bool wasComposing = isInputMethodComposing();
    d->control->processEvent(event, QPointF(0, -d->yoff));
    if (wasComposing != isInputMethodComposing())
        emit inputMethodComposingChanged();
}

/*!
\overload
Returns the value of the given \a property.
*/
QVariant QDeclarative1TextEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    Q_D(const QDeclarative1TextEdit);
    return d->control->inputMethodQuery(property);
}

/*!
Draws the contents of the text edit using the given \a painter within
the given \a bounds.
*/
void QDeclarative1TextEdit::drawContents(QPainter *painter, const QRect &bounds)
{
    Q_D(QDeclarative1TextEdit);

    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->translate(0,d->yoff);

    d->control->drawContents(painter, bounds.translated(0,-d->yoff));

    painter->translate(0,-d->yoff);
}

void QDeclarative1TextEdit::updateImgCache(const QRectF &rf)
{
    Q_D(const QDeclarative1TextEdit);
    QRect r;
    if (!rf.isValid()) {
        r = QRect(0,0,INT_MAX,INT_MAX);
    } else {
        r = rf.toRect();
        if (r.height() > INT_MAX/2) {
            // Take care of overflow when translating "everything"
            r.setTop(r.y() + d->yoff);
            r.setBottom(INT_MAX/2);
        } else {
            r = r.translated(0,d->yoff);
        }
    }
    dirtyCache(r);
    emit update();
}

/*!
    \qmlproperty bool QtQuick1::TextEdit::smooth

    This property holds whether the text is smoothly scaled or transformed.

    Smooth filtering gives better visual quality, but is slower.  If
    the item is displayed at its natural size, this property has no visual or
    performance effect.

    \note Generally scaling artifacts are only visible if the item is stationary on
    the screen.  A common pattern when animating an item is to disable smooth
    filtering at the beginning of the animation and reenable it at the conclusion.
*/

/*!
    \qmlproperty bool QtQuick1::TextEdit::canPaste
    \since QtQuick 1.1

    Returns true if the TextEdit is writable and the content of the clipboard is
    suitable for pasting into the TextEdit.
*/
bool QDeclarative1TextEdit::canPaste() const
{
    Q_D(const QDeclarative1TextEdit);
    return d->canPaste;
}

/*!
    \qmlproperty bool QtQuick1::TextEdit::inputMethodComposing

    \since QtQuick 1.1

    This property holds whether the TextEdit has partial text input from an
    input method.

    While it is composing an input method may rely on mouse or key events from
    the TextEdit to edit or commit the partial text.  This property can be used
    to determine when to disable events handlers that may interfere with the
    correct operation of an input method.
*/
bool QDeclarative1TextEdit::isInputMethodComposing() const
{
    Q_D(const QDeclarative1TextEdit);
    if (QTextLayout *layout = d->control->textCursor().block().layout())
        return layout->preeditAreaText().length() > 0;
    return false;
}

void QDeclarative1TextEditPrivate::init()
{
    Q_Q(QDeclarative1TextEdit);

    q->setSmooth(smooth);
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFlag(QGraphicsItem::ItemHasNoContents, false);
    q->setFlag(QGraphicsItem::ItemAcceptsInputMethod);

    control = new QWidgetTextControl(q);
    control->setIgnoreUnusedNavigationEvents(true);
    control->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByKeyboard | Qt::TextEditable));
    control->setDragEnabled(false);

    // QWidgetTextControl follows the default text color
    // defined by the platform, declarative text
    // should be black by default
    QPalette pal = control->palette();
    if (pal.color(QPalette::Text) != color) {
        pal.setColor(QPalette::Text, color);
        control->setPalette(pal);
    }

    QObject::connect(control, SIGNAL(updateRequest(QRectF)), q, SLOT(updateImgCache(QRectF)));

    QObject::connect(control, SIGNAL(textChanged()), q, SLOT(q_textChanged()));
    QObject::connect(control, SIGNAL(selectionChanged()), q, SIGNAL(selectionChanged()));
    QObject::connect(control, SIGNAL(selectionChanged()), q, SLOT(updateSelectionMarkers()));
    QObject::connect(control, SIGNAL(cursorPositionChanged()), q, SLOT(updateSelectionMarkers()));
    QObject::connect(control, SIGNAL(cursorPositionChanged()), q, SIGNAL(cursorPositionChanged()));
    QObject::connect(control, SIGNAL(microFocusChanged()), q, SLOT(moveCursorDelegate()));
    QObject::connect(control, SIGNAL(linkActivated(QString)), q, SIGNAL(linkActivated(QString)));
#ifndef QT_NO_CLIPBOARD
    QObject::connect(q, SIGNAL(readOnlyChanged(bool)), q, SLOT(q_canPasteChanged()));
    QObject::connect(QApplication::clipboard(), SIGNAL(dataChanged()), q, SLOT(q_canPasteChanged()));
    canPaste = control->canPaste();
#endif

    document = control->document();
    document->setDefaultFont(font);
    document->setDocumentMargin(textMargin);
    document->setUndoRedoEnabled(false); // flush undo buffer.
    document->setUndoRedoEnabled(true);
    updateDefaultTextOption();
}

void QDeclarative1TextEdit::q_textChanged()
{
    Q_D(QDeclarative1TextEdit);
    d->text = text();
    d->rightToLeftText = d->document->begin().layout()->engine()->isRightToLeft();
    d->determineHorizontalAlignment();
    d->updateDefaultTextOption();
    updateSize();
    updateTotalLines();
    emit textChanged(d->text);
}

void QDeclarative1TextEdit::moveCursorDelegate()
{
    Q_D(QDeclarative1TextEdit);
    d->determineHorizontalAlignment();
    updateMicroFocus();
    emit cursorRectangleChanged();
    if(!d->cursor)
        return;
    QRectF cursorRect = cursorRectangle();
    d->cursor->setX(cursorRect.x());
    d->cursor->setY(cursorRect.y());
}

void QDeclarative1TextEditPrivate::updateSelection()
{
    Q_Q(QDeclarative1TextEdit);
    QTextCursor cursor = control->textCursor();
    bool startChange = (lastSelectionStart != cursor.selectionStart());
    bool endChange = (lastSelectionEnd != cursor.selectionEnd());
    cursor.beginEditBlock();
    cursor.setPosition(lastSelectionStart, QTextCursor::MoveAnchor);
    cursor.setPosition(lastSelectionEnd, QTextCursor::KeepAnchor);
    cursor.endEditBlock();
    control->setTextCursor(cursor);
    if(startChange)
        q->selectionStartChanged();
    if(endChange)
        q->selectionEndChanged();
}

void QDeclarative1TextEdit::updateSelectionMarkers()
{
    Q_D(QDeclarative1TextEdit);
    if(d->lastSelectionStart != d->control->textCursor().selectionStart()){
        d->lastSelectionStart = d->control->textCursor().selectionStart();
        emit selectionStartChanged();
    }
    if(d->lastSelectionEnd != d->control->textCursor().selectionEnd()){
        d->lastSelectionEnd = d->control->textCursor().selectionEnd();
        emit selectionEndChanged();
    }
}

QRectF QDeclarative1TextEdit::boundingRect() const
{
    Q_D(const QDeclarative1TextEdit);
    QRectF r = QDeclarative1PaintedItem::boundingRect();
    int cursorWidth = 1;
    if(d->cursor)
        cursorWidth = d->cursor->width();
    if(!d->document->isEmpty())
        cursorWidth += 3;// ### Need a better way of accounting for space between char and cursor

    // Could include font max left/right bearings to either side of rectangle.

    r.setRight(r.right() + cursorWidth);
    return r.translated(0,d->yoff);
}

qreal QDeclarative1TextEditPrivate::implicitWidth() const
{
    Q_Q(const QDeclarative1TextEdit);
    if (!requireImplicitWidth) {
        // We don't calculate implicitWidth unless it is required.
        // We need to force a size update now to ensure implicitWidth is calculated
        const_cast<QDeclarative1TextEditPrivate*>(this)->requireImplicitWidth = true;
        const_cast<QDeclarative1TextEdit*>(q)->updateSize();
    }
    return mImplicitWidth;
}

//### we should perhaps be a bit smarter here -- depending on what has changed, we shouldn't
//    need to do all the calculations each time
void QDeclarative1TextEdit::updateSize()
{
    Q_D(QDeclarative1TextEdit);
    if (isComponentComplete()) {
        qreal naturalWidth = d->mImplicitWidth;
        // ### assumes that if the width is set, the text will fill to edges
        // ### (unless wrap is false, then clipping will occur)
        if (widthValid()) {
            if (!d->requireImplicitWidth) {
                emit implicitWidthChanged();
                // if the implicitWidth is used, then updateSize() has already been called (recursively)
                if (d->requireImplicitWidth)
                    return;
            }
            if (d->requireImplicitWidth) {
                d->document->setTextWidth(-1);
                naturalWidth = d->document->idealWidth();
            }
            if (d->document->textWidth() != width())
                d->document->setTextWidth(width());
        } else {
            d->document->setTextWidth(-1);
        }
        QFontMetrics fm = QFontMetrics(d->font);
        int dy = height();
        dy -= (int)d->document->size().height();

        int nyoff;
        if (heightValid()) {
            if (d->vAlign == AlignBottom)
                nyoff = dy;
            else if (d->vAlign == AlignVCenter)
                nyoff = dy/2;
            else
                nyoff = 0;
        } else {
            nyoff = 0;
        }
        if (nyoff != d->yoff) {
            prepareGeometryChange();
            d->yoff = nyoff;
        }
        setBaselineOffset(fm.ascent() + d->yoff + d->textMargin);

        //### need to comfirm cost of always setting these
        int newWidth = qCeil(d->document->idealWidth());
        if (!widthValid() && d->document->textWidth() != newWidth)
            d->document->setTextWidth(newWidth); // ### Text does not align if width is not set (QTextDoc bug)
        // ### Setting the implicitWidth triggers another updateSize(), and unless there are bindings nothing has changed.
        if (!widthValid())
            setImplicitWidth(newWidth);
        else if (d->requireImplicitWidth)
            setImplicitWidth(naturalWidth);
        qreal newHeight = d->document->isEmpty() ? fm.height() : (int)d->document->size().height();
        setImplicitHeight(newHeight);

        d->paintedSize = QSize(newWidth, newHeight);
        setContentsSize(d->paintedSize);
        emit paintedSizeChanged();
    } else {
        d->dirty = true;
    }
    emit update();
}

void QDeclarative1TextEdit::updateTotalLines()
{
    Q_D(QDeclarative1TextEdit);

    int subLines = 0;

    for (QTextBlock it = d->document->begin(); it != d->document->end(); it = it.next()) {
        QTextLayout *layout = it.layout();
        if (!layout)
            continue;
        subLines += layout->lineCount()-1;
    }

    int newTotalLines = d->document->lineCount() + subLines;
    if (d->lineCount != newTotalLines) {
        d->lineCount = newTotalLines;
        emit lineCountChanged();
    }
}

void QDeclarative1TextEditPrivate::updateDefaultTextOption()
{
    Q_Q(QDeclarative1TextEdit);
    QTextOption opt = document->defaultTextOption();
    int oldAlignment = opt.alignment();

    QDeclarative1TextEdit::HAlignment horizontalAlignment = q->effectiveHAlign();
    if (rightToLeftText) {
        if (horizontalAlignment == QDeclarative1TextEdit::AlignLeft)
            horizontalAlignment = QDeclarative1TextEdit::AlignRight;
        else if (horizontalAlignment == QDeclarative1TextEdit::AlignRight)
            horizontalAlignment = QDeclarative1TextEdit::AlignLeft;
    }
    opt.setAlignment((Qt::Alignment)(int)(horizontalAlignment | vAlign));

    QTextOption::WrapMode oldWrapMode = opt.wrapMode();
    opt.setWrapMode(QTextOption::WrapMode(wrapMode));

    if (oldWrapMode == opt.wrapMode() && oldAlignment == opt.alignment())
        return;
    document->setDefaultTextOption(opt);
}


/*!
    \qmlmethod void QtQuick1::TextEdit::openSoftwareInputPanel()

    Opens software input panels like virtual keyboards for typing, useful for
    customizing when you want the input keyboard to be shown and hidden in
    your application.

    By default the opening of input panels follows the platform style. On Symbian^1 and
    Symbian^3 -based devices the panels are opened by clicking TextEdit. On other platforms
    the panels are automatically opened when TextEdit element gains active focus. Input panels are
    always closed if no editor has active focus.

    You can disable the automatic behavior by setting the property \c activeFocusOnPress to false
    and use functions openSoftwareInputPanel() and closeSoftwareInputPanel() to implement
    the behavior you want.

    Only relevant on platforms, which provide virtual keyboards.

    \code
        import QtQuick 1.0
        TextEdit {
            id: textEdit
            text: "Hello world!"
            activeFocusOnPress: false
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (!textEdit.activeFocus) {
                        textEdit.forceActiveFocus();
                        textEdit.openSoftwareInputPanel();
                    } else {
                        textEdit.focus = false;
                    }
                }
                onPressAndHold: textEdit.closeSoftwareInputPanel();
            }
        }
    \endcode
*/
void QDeclarative1TextEdit::openSoftwareInputPanel()
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
    \qmlmethod void QtQuick1::TextEdit::closeSoftwareInputPanel()

    Closes a software input panel like a virtual keyboard shown on the screen, useful
    for customizing when you want the input keyboard to be shown and hidden in
    your application.

    By default the opening of input panels follows the platform style. On Symbian^1 and
    Symbian^3 -based devices the panels are opened by clicking TextEdit. On other platforms
    the panels are automatically opened when TextEdit element gains active focus. Input panels are
    always closed if no editor has active focus.

    You can disable the automatic behavior by setting the property \c activeFocusOnPress to false
    and use functions openSoftwareInputPanel() and closeSoftwareInputPanel() to implement
    the behavior you want.

    Only relevant on platforms, which provide virtual keyboards.

    \code
        import QtQuick 1.0
        TextEdit {
            id: textEdit
            text: "Hello world!"
            activeFocusOnPress: false
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (!textEdit.activeFocus) {
                        textEdit.forceActiveFocus();
                        textEdit.openSoftwareInputPanel();
                    } else {
                        textEdit.focus = false;
                    }
                }
                onPressAndHold: textEdit.closeSoftwareInputPanel();
            }
        }
    \endcode
*/
void QDeclarative1TextEdit::closeSoftwareInputPanel()
{
    if (qApp) {
        if (QGraphicsView * view = qobject_cast<QGraphicsView*>(qApp->focusWidget())) {
            if (view->scene() && view->scene() == scene()) {
                qApp->inputPanel()->hide();
            }
        }
    }
}

void QDeclarative1TextEdit::focusInEvent(QFocusEvent *event)
{
    Q_D(const QDeclarative1TextEdit);
    if (d->showInputPanelOnFocus) {
        if (d->focusOnPress && !isReadOnly()) {
            openSoftwareInputPanel();
        }
    }
    QDeclarative1PaintedItem::focusInEvent(event);
}

void QDeclarative1TextEdit::q_canPasteChanged()
{
    Q_D(QDeclarative1TextEdit);
    bool old = d->canPaste;
    d->canPaste = d->control->canPaste();
    if(old!=d->canPaste)
        emit canPasteChanged();
}



QT_END_NAMESPACE
