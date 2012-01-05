/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qquicktextedit_p.h"
#include "qquicktextedit_p_p.h"
#include "qquicktextcontrol_p.h"
#include "qquicktext_p_p.h"
#include "qquickevents_p_p.h"
#include "qquickcanvas.h"
#include "qquicktextnode_p.h"
#include <QtQuick/qsgsimplerectnode.h>

#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qevent.h>
#include <QtGui/qpainter.h>
#include <QtGui/qtextobject.h>
#include <QtCore/qmath.h>

#include <private/qdeclarativeglobal_p.h>
#include <private/qtextengine_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <private/qsgadaptationlayer_p.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)
DEFINE_BOOL_CONFIG_OPTION(qmlEnableImageCache, QML_ENABLE_TEXT_IMAGE_CACHE)

/*!
    \qmlclass TextEdit QQuickTextEdit
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-visual-elements
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
    \qmlsignal QtQuick2::TextEdit::onLinkActivated(string link)

    This handler is called when the user clicks on a link embedded in the text.
    The link must be in rich text or HTML format and the
    \a link string provides access to the particular link.
*/
QQuickTextEdit::QQuickTextEdit(QQuickItem *parent)
: QQuickImplicitSizeItem(*(new QQuickTextEditPrivate), parent)
{
    Q_D(QQuickTextEdit);
    d->init();
}

QString QQuickTextEdit::text() const
{
    Q_D(const QQuickTextEdit);

#ifndef QT_NO_TEXTHTMLPARSER
    if (d->richText)
        return d->control->toHtml();
    else
#endif
        return d->control->toPlainText();
}

/*!
    \qmlproperty string QtQuick2::TextEdit::font.family

    Sets the family name of the font.

    The family name is case insensitive and may optionally include a foundry name, e.g. "Helvetica [Cronyx]".
    If the family is available from more than one foundry and the foundry isn't specified, an arbitrary foundry is chosen.
    If the family isn't available a family will be set using the font matching algorithm.
*/

/*!
    \qmlproperty bool QtQuick2::TextEdit::font.bold

    Sets whether the font weight is bold.
*/

/*!
    \qmlproperty enumeration QtQuick2::TextEdit::font.weight

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
    \qmlproperty bool QtQuick2::TextEdit::font.italic

    Sets whether the font has an italic style.
*/

/*!
    \qmlproperty bool QtQuick2::TextEdit::font.underline

    Sets whether the text is underlined.
*/

/*!
    \qmlproperty bool QtQuick2::TextEdit::font.strikeout

    Sets whether the font has a strikeout style.
*/

/*!
    \qmlproperty real QtQuick2::TextEdit::font.pointSize

    Sets the font size in points. The point size must be greater than zero.
*/

/*!
    \qmlproperty int QtQuick2::TextEdit::font.pixelSize

    Sets the font size in pixels.

    Using this function makes the font device dependent.  Use
    \l{TextEdit::font.pointSize} to set the size of the font in a
    device independent manner.
*/

/*!
    \qmlproperty real QtQuick2::TextEdit::font.letterSpacing

    Sets the letter spacing for the font.

    Letter spacing changes the default spacing between individual letters in the font.
    A positive value increases the letter spacing by the corresponding pixels; a negative value decreases the spacing.
*/

/*!
    \qmlproperty real QtQuick2::TextEdit::font.wordSpacing

    Sets the word spacing for the font.

    Word spacing changes the default spacing between individual words.
    A positive value increases the word spacing by a corresponding amount of pixels,
    while a negative value decreases the inter-word spacing accordingly.
*/

/*!
    \qmlproperty enumeration QtQuick2::TextEdit::font.capitalization

    Sets the capitalization for the text.

    \list
    \o Font.MixedCase - This is the normal text rendering option where no capitalization change is applied.
    \o Font.AllUppercase - This alters the text to be rendered in all uppercase type.
    \o Font.AllLowercase - This alters the text to be rendered in all lowercase type.
    \o Font.SmallCaps - This alters the text to be rendered in small-caps type.
    \o Font.Capitalize - This alters the text to be rendered with the first character of each word as an uppercase character.
    \endlist

    \qml
    TextEdit { text: "Hello"; font.capitalization: Font.AllLowercase }
    \endqml
*/

/*!
    \qmlproperty string QtQuick2::TextEdit::text

    The text to display.  If the text format is AutoText the text edit will
    automatically determine whether the text should be treated as
    rich text.  This determination is made using Qt::mightBeRichText().
*/
void QQuickTextEdit::setText(const QString &text)
{
    Q_D(QQuickTextEdit);
    if (QQuickTextEdit::text() == text)
        return;

    d->document->clearResources();
    d->richText = d->format == RichText || (d->format == AutoText && Qt::mightBeRichText(text));
    if (d->richText) {
#ifndef QT_NO_TEXTHTMLPARSER
        d->control->setHtml(text);
#else
        d->control->setPlainText(text);
#endif
        d->useImageFallback = qmlEnableImageCache();
    } else {
        d->control->setPlainText(text);
    }
    q_textChanged();
}

/*!
    \qmlproperty enumeration QtQuick2::TextEdit::textFormat

    The way the text property should be displayed.

    \list
    \o TextEdit.AutoText
    \o TextEdit.PlainText
    \o TextEdit.RichText
    \endlist

    The default is TextEdit.PlainText.  If the text format is TextEdit.AutoText the text edit
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
QQuickTextEdit::TextFormat QQuickTextEdit::textFormat() const
{
    Q_D(const QQuickTextEdit);
    return d->format;
}

void QQuickTextEdit::setTextFormat(TextFormat format)
{
    Q_D(QQuickTextEdit);
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
        d->useImageFallback = qmlEnableImageCache();
    }
    d->format = format;
    d->control->setAcceptRichText(d->format != PlainText);
    emit textFormatChanged(d->format);
}

QFont QQuickTextEdit::font() const
{
    Q_D(const QQuickTextEdit);
    return d->sourceFont;
}

void QQuickTextEdit::setFont(const QFont &font)
{
    Q_D(QQuickTextEdit);
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
        d->document->setDefaultFont(d->font);
        if (d->cursor) {
            d->cursor->setHeight(QFontMetrics(d->font).height());
            moveCursorDelegate();
        }
        updateSize();
        updateDocument();
    }
    emit fontChanged(d->sourceFont);
}

/*!
    \qmlproperty color QtQuick2::TextEdit::color

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
QColor QQuickTextEdit::color() const
{
    Q_D(const QQuickTextEdit);
    return d->color;
}

void QQuickTextEdit::setColor(const QColor &color)
{
    Q_D(QQuickTextEdit);
    if (d->color == color)
        return;

    d->color = color;
    QPalette pal = d->control->palette();
    pal.setColor(QPalette::Text, color);
    d->control->setPalette(pal);
    updateDocument();
    emit colorChanged(d->color);
}

/*!
    \qmlproperty color QtQuick2::TextEdit::selectionColor

    The text highlight color, used behind selections.
*/
QColor QQuickTextEdit::selectionColor() const
{
    Q_D(const QQuickTextEdit);
    return d->selectionColor;
}

void QQuickTextEdit::setSelectionColor(const QColor &color)
{
    Q_D(QQuickTextEdit);
    if (d->selectionColor == color)
        return;

    d->selectionColor = color;
    QPalette pal = d->control->palette();
    pal.setColor(QPalette::Highlight, color);
    d->control->setPalette(pal);
    updateDocument();
    emit selectionColorChanged(d->selectionColor);
}

/*!
    \qmlproperty color QtQuick2::TextEdit::selectedTextColor

    The selected text color, used in selections.
*/
QColor QQuickTextEdit::selectedTextColor() const
{
    Q_D(const QQuickTextEdit);
    return d->selectedTextColor;
}

void QQuickTextEdit::setSelectedTextColor(const QColor &color)
{
    Q_D(QQuickTextEdit);
    if (d->selectedTextColor == color)
        return;

    d->selectedTextColor = color;
    QPalette pal = d->control->palette();
    pal.setColor(QPalette::HighlightedText, color);
    d->control->setPalette(pal);
    updateDocument();
    emit selectedTextColorChanged(d->selectedTextColor);
}

/*!
    \qmlproperty enumeration QtQuick2::TextEdit::horizontalAlignment
    \qmlproperty enumeration QtQuick2::TextEdit::verticalAlignment
    \qmlproperty enumeration QtQuick2::TextEdit::effectiveHorizontalAlignment

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
QQuickTextEdit::HAlignment QQuickTextEdit::hAlign() const
{
    Q_D(const QQuickTextEdit);
    return d->hAlign;
}

void QQuickTextEdit::setHAlign(HAlignment align)
{
    Q_D(QQuickTextEdit);
    bool forceAlign = d->hAlignImplicit && d->effectiveLayoutMirror;
    d->hAlignImplicit = false;
    if (d->setHAlign(align, forceAlign) && isComponentComplete()) {
        d->updateDefaultTextOption();
        updateSize();
    }
}

void QQuickTextEdit::resetHAlign()
{
    Q_D(QQuickTextEdit);
    d->hAlignImplicit = true;
    if (d->determineHorizontalAlignment() && isComponentComplete()) {
        d->updateDefaultTextOption();
        updateSize();
    }
}

QQuickTextEdit::HAlignment QQuickTextEdit::effectiveHAlign() const
{
    Q_D(const QQuickTextEdit);
    QQuickTextEdit::HAlignment effectiveAlignment = d->hAlign;
    if (!d->hAlignImplicit && d->effectiveLayoutMirror) {
        switch (d->hAlign) {
        case QQuickTextEdit::AlignLeft:
            effectiveAlignment = QQuickTextEdit::AlignRight;
            break;
        case QQuickTextEdit::AlignRight:
            effectiveAlignment = QQuickTextEdit::AlignLeft;
            break;
        default:
            break;
        }
    }
    return effectiveAlignment;
}

bool QQuickTextEditPrivate::setHAlign(QQuickTextEdit::HAlignment alignment, bool forceAlign)
{
    Q_Q(QQuickTextEdit);
    if (hAlign != alignment || forceAlign) {
        QQuickTextEdit::HAlignment oldEffectiveHAlign = q->effectiveHAlign();
        hAlign = alignment;
        emit q->horizontalAlignmentChanged(alignment);
        if (oldEffectiveHAlign != q->effectiveHAlign())
            emit q->effectiveHorizontalAlignmentChanged();
        return true;
    }
    return false;
}

bool QQuickTextEditPrivate::determineHorizontalAlignment()
{
    Q_Q(QQuickTextEdit);
    if (hAlignImplicit && q->isComponentComplete()) {
        bool alignToRight;
        if (text.isEmpty()) {
            const QString preeditText = control->textCursor().block().layout()->preeditAreaText();
            alignToRight = preeditText.isEmpty()
                    ? QGuiApplication::keyboardInputDirection() == Qt::RightToLeft
                    : preeditText.isRightToLeft();
        } else {
            alignToRight = rightToLeftText;
        }
        return setHAlign(alignToRight ? QQuickTextEdit::AlignRight : QQuickTextEdit::AlignLeft);
    }
    return false;
}

void QQuickTextEditPrivate::mirrorChange()
{
    Q_Q(QQuickTextEdit);
    if (q->isComponentComplete()) {
        if (!hAlignImplicit && (hAlign == QQuickTextEdit::AlignRight || hAlign == QQuickTextEdit::AlignLeft)) {
            updateDefaultTextOption();
            q->updateSize();
            emit q->effectiveHorizontalAlignmentChanged();
        }
    }
}

QQuickTextEdit::VAlignment QQuickTextEdit::vAlign() const
{
    Q_D(const QQuickTextEdit);
    return d->vAlign;
}

void QQuickTextEdit::setVAlign(QQuickTextEdit::VAlignment alignment)
{
    Q_D(QQuickTextEdit);
    if (alignment == d->vAlign)
        return;
    d->vAlign = alignment;
    d->updateDefaultTextOption();
    updateSize();
    moveCursorDelegate();
    emit verticalAlignmentChanged(d->vAlign);
}
/*!
    \qmlproperty enumeration QtQuick2::TextEdit::wrapMode

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
QQuickTextEdit::WrapMode QQuickTextEdit::wrapMode() const
{
    Q_D(const QQuickTextEdit);
    return d->wrapMode;
}

void QQuickTextEdit::setWrapMode(WrapMode mode)
{
    Q_D(QQuickTextEdit);
    if (mode == d->wrapMode)
        return;
    d->wrapMode = mode;
    d->updateDefaultTextOption();
    updateSize();
    emit wrapModeChanged();
}

/*!
    \qmlproperty int QtQuick2::TextEdit::lineCount

    Returns the total number of lines in the textEdit item.
*/
int QQuickTextEdit::lineCount() const
{
    Q_D(const QQuickTextEdit);
    return d->lineCount;
}

/*!
    \qmlproperty int QtQuick2::TextEdit::length

    Returns the total number of plain text characters in the TextEdit item.

    As this number doesn't include any formatting markup it may not be the same as the
    length of the string returned by the \l text property.

    This property can be faster than querying the length the \l text property as it doesn't
    require any copying or conversion of the TextEdit's internal string data.
*/

int QQuickTextEdit::length() const
{
    Q_D(const QQuickTextEdit);
    // QTextDocument::characterCount() includes the terminating null character.
    return qMax(0, d->document->characterCount() - 1);
}

/*!
    \qmlproperty real QtQuick2::TextEdit::paintedWidth

    Returns the width of the text, including the width past the width
    which is covered due to insufficient wrapping if \l wrapMode is set.
*/
qreal QQuickTextEdit::paintedWidth() const
{
    Q_D(const QQuickTextEdit);
    return d->paintedSize.width();
}

/*!
    \qmlproperty real QtQuick2::TextEdit::paintedHeight

    Returns the height of the text, including the height past the height
    that is covered if the text does not fit within the set height.
*/
qreal QQuickTextEdit::paintedHeight() const
{
    Q_D(const QQuickTextEdit);
    return d->paintedSize.height();
}

/*!
    \qmlmethod rectangle QtQuick2::TextEdit::positionToRectangle(position)

    Returns the rectangle at the given \a position in the text. The x, y,
    and height properties correspond to the cursor that would describe
    that position.
*/
QRectF QQuickTextEdit::positionToRectangle(int pos) const
{
    Q_D(const QQuickTextEdit);
    QTextCursor c(d->document);
    c.setPosition(pos);
    return d->control->cursorRect(c);

}

/*!
    \qmlmethod int QtQuick2::TextEdit::positionAt(int x, int y)

    Returns the text position closest to pixel position (\a x, \a y).

    Position 0 is before the first character, position 1 is after the first character
    but before the second, and so on until position \l {text}.length, which is after all characters.
*/
int QQuickTextEdit::positionAt(int x, int y) const
{
    Q_D(const QQuickTextEdit);
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

/*!
    \qmlmethod void QtQuick2::TextEdit::moveCursorSelection(int position, SelectionMode mode = TextEdit.SelectCharacters)

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
    words between the specified position and the previous cursor position.  Words partially in the
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
void QQuickTextEdit::moveCursorSelection(int pos)
{
    //Note that this is the same as setCursorPosition but with the KeepAnchor flag set
    Q_D(QQuickTextEdit);
    QTextCursor cursor = d->control->textCursor();
    if (cursor.position() == pos)
        return;
    cursor.setPosition(pos, QTextCursor::KeepAnchor);
    d->control->setTextCursor(cursor);
}

void QQuickTextEdit::moveCursorSelection(int pos, SelectionMode mode)
{
    Q_D(QQuickTextEdit);
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
    \qmlproperty bool QtQuick2::TextEdit::cursorVisible
    If true the text edit shows a cursor.

    This property is set and unset when the text edit gets active focus, but it can also
    be set directly (useful, for example, if a KeyProxy might forward keys to it).
*/
bool QQuickTextEdit::isCursorVisible() const
{
    Q_D(const QQuickTextEdit);
    return d->cursorVisible;
}

void QQuickTextEdit::setCursorVisible(bool on)
{
    Q_D(QQuickTextEdit);
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
    \qmlproperty int QtQuick2::TextEdit::cursorPosition
    The position of the cursor in the TextEdit.
*/
int QQuickTextEdit::cursorPosition() const
{
    Q_D(const QQuickTextEdit);
    return d->control->textCursor().position();
}

void QQuickTextEdit::setCursorPosition(int pos)
{
    Q_D(QQuickTextEdit);
    if (pos < 0 || pos > d->text.length())
        return;
    QTextCursor cursor = d->control->textCursor();
    if (cursor.position() == pos && cursor.anchor() == pos)
        return;
    cursor.setPosition(pos);
    d->control->setTextCursor(cursor);
}

/*!
    \qmlproperty Component QtQuick2::TextEdit::cursorDelegate
    The delegate for the cursor in the TextEdit.

    If you set a cursorDelegate for a TextEdit, this delegate will be used for
    drawing the cursor instead of the standard cursor. An instance of the
    delegate will be created and managed by the text edit when a cursor is
    needed, and the x and y properties of delegate instance will be set so as
    to be one pixel before the top left of the current character.

    Note that the root item of the delegate component must be a QDeclarativeItem or
    QDeclarativeItem derived item.
*/
QDeclarativeComponent* QQuickTextEdit::cursorDelegate() const
{
    Q_D(const QQuickTextEdit);
    return d->cursorComponent;
}

void QQuickTextEdit::setCursorDelegate(QDeclarativeComponent* c)
{
    Q_D(QQuickTextEdit);
    if (d->cursorComponent) {
        if (d->cursor) {
            d->control->setCursorWidth(-1);
            updateCursor();
            delete d->cursor;
            d->cursor = 0;
        }
    }
    d->cursorComponent = c;
    if (c && c->isReady()) {
        loadCursorDelegate();
    } else {
        if (c)
            connect(c, SIGNAL(statusChanged()),
                    this, SLOT(loadCursorDelegate()));
    }

    emit cursorDelegateChanged();
}

void QQuickTextEdit::loadCursorDelegate()
{
    Q_D(QQuickTextEdit);
    if (d->cursorComponent->isLoading())
        return;
    QDeclarativeContext *creationContext = d->cursorComponent->creationContext();
    QObject *object = d->cursorComponent->create(creationContext ? creationContext : qmlContext(this));
    d->cursor = qobject_cast<QQuickItem*>(object);
    if (d->cursor) {
        d->control->setCursorWidth(0);
        updateCursor();
        QDeclarative_setParent_noEvent(d->cursor, this);
        d->cursor->setParentItem(this);
        d->cursor->setHeight(QFontMetrics(d->font).height());
        moveCursorDelegate();
    }else{
        delete object;
        qmlInfo(this) << "Error loading cursor delegate.";
    }
}

/*!
    \qmlproperty int QtQuick2::TextEdit::selectionStart

    The cursor position before the first character in the current selection.

    This property is read-only. To change the selection, use select(start,end),
    selectAll(), or selectWord().

    \sa selectionEnd, cursorPosition, selectedText
*/
int QQuickTextEdit::selectionStart() const
{
    Q_D(const QQuickTextEdit);
    return d->control->textCursor().selectionStart();
}

/*!
    \qmlproperty int QtQuick2::TextEdit::selectionEnd

    The cursor position after the last character in the current selection.

    This property is read-only. To change the selection, use select(start,end),
    selectAll(), or selectWord().

    \sa selectionStart, cursorPosition, selectedText
*/
int QQuickTextEdit::selectionEnd() const
{
    Q_D(const QQuickTextEdit);
    return d->control->textCursor().selectionEnd();
}

/*!
    \qmlproperty string QtQuick2::TextEdit::selectedText

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
QString QQuickTextEdit::selectedText() const
{
    Q_D(const QQuickTextEdit);
    return d->control->textCursor().selectedText();
}

/*!
    \qmlproperty bool QtQuick2::TextEdit::activeFocusOnPress

    Whether the TextEdit should gain active focus on a mouse press. By default this is
    set to true.
*/
bool QQuickTextEdit::focusOnPress() const
{
    Q_D(const QQuickTextEdit);
    return d->focusOnPress;
}

void QQuickTextEdit::setFocusOnPress(bool on)
{
    Q_D(QQuickTextEdit);
    if (d->focusOnPress == on)
        return;
    d->focusOnPress = on;
    emit activeFocusOnPressChanged(d->focusOnPress);
}

/*!
    \qmlproperty bool QtQuick2::TextEdit::persistentSelection

    Whether the TextEdit should keep the selection visible when it loses active focus to another
    item in the scene. By default this is set to true;
*/
bool QQuickTextEdit::persistentSelection() const
{
    Q_D(const QQuickTextEdit);
    return d->persistentSelection;
}

void QQuickTextEdit::setPersistentSelection(bool on)
{
    Q_D(QQuickTextEdit);
    if (d->persistentSelection == on)
        return;
    d->persistentSelection = on;
    emit persistentSelectionChanged(d->persistentSelection);
}

/*!
   \qmlproperty real QtQuick2::TextEdit::textMargin

   The margin, in pixels, around the text in the TextEdit.
*/
qreal QQuickTextEdit::textMargin() const
{
    Q_D(const QQuickTextEdit);
    return d->textMargin;
}

void QQuickTextEdit::setTextMargin(qreal margin)
{
    Q_D(QQuickTextEdit);
    if (d->textMargin == margin)
        return;
    d->textMargin = margin;
    d->document->setDocumentMargin(d->textMargin);
    emit textMarginChanged(d->textMargin);
}

/*!
    \qmlproperty enumeration QtQuick2::TextEdit::inputMethodHints

    Provides hints to the input method about the expected content of the text edit and how it
    should operate.

    The value is a bit-wise combination of flags or Qt.ImhNone if no hints are set.

    Flags that alter behaviour are:

    \list
    \o Qt.ImhHiddenText - Characters should be hidden, as is typically used when entering passwords.
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

void QQuickTextEdit::geometryChanged(const QRectF &newGeometry,
                                  const QRectF &oldGeometry)
{
    if (newGeometry.width() != oldGeometry.width())
        updateSize();
    QQuickImplicitSizeItem::geometryChanged(newGeometry, oldGeometry);
}

/*!
    Ensures any delayed caching or data loading the class
    needs to performed is complete.
*/
void QQuickTextEdit::componentComplete()
{
    Q_D(QQuickTextEdit);
    QQuickImplicitSizeItem::componentComplete();

    if (d->richText)
        d->useImageFallback = qmlEnableImageCache();

    if (d->dirty) {
        d->determineHorizontalAlignment();
        d->updateDefaultTextOption();
        updateSize();
        d->dirty = false;
    }

}
/*!
    \qmlproperty bool QtQuick2::TextEdit::selectByMouse

    Defaults to false.

    If true, the user can use the mouse to select text in some
    platform-specific way. Note that for some platforms this may
    not be an appropriate interaction (eg. may conflict with how
    the text needs to behave inside a Flickable.
*/
bool QQuickTextEdit::selectByMouse() const
{
    Q_D(const QQuickTextEdit);
    return d->selectByMouse;
}

void QQuickTextEdit::setSelectByMouse(bool on)
{
    Q_D(QQuickTextEdit);
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
    \qmlproperty enum QtQuick2::TextEdit::mouseSelectionMode

    Specifies how text should be selected using a mouse.

    \list
    \o TextEdit.SelectCharacters - The selection is updated with individual characters. (Default)
    \o TextEdit.SelectWords - The selection is updated with whole words.
    \endlist

    This property only applies when \l selectByMouse is true.
*/
QQuickTextEdit::SelectionMode QQuickTextEdit::mouseSelectionMode() const
{
    Q_D(const QQuickTextEdit);
    return d->mouseSelectionMode;
}

void QQuickTextEdit::setMouseSelectionMode(SelectionMode mode)
{
    Q_D(QQuickTextEdit);
    if (d->mouseSelectionMode != mode) {
        d->mouseSelectionMode = mode;
        d->control->setWordSelectionEnabled(mode == SelectWords);
        emit mouseSelectionModeChanged(mode);
    }
}

/*!
    \qmlproperty bool QtQuick2::TextEdit::readOnly

    Whether the user can interact with the TextEdit item. If this
    property is set to true the text cannot be edited by user interaction.

    By default this property is false.
*/
void QQuickTextEdit::setReadOnly(bool r)
{
    Q_D(QQuickTextEdit);
    if (r == isReadOnly())
        return;

    setFlag(QQuickItem::ItemAcceptsInputMethod, !r);
    Qt::TextInteractionFlags flags = Qt::LinksAccessibleByMouse;
    if (d->selectByMouse)
        flags = flags | Qt::TextSelectableByMouse;
    if (!r)
        flags = flags | Qt::TextSelectableByKeyboard | Qt::TextEditable;
    d->control->setTextInteractionFlags(flags);
    if (!r)
        d->control->moveCursor(QTextCursor::End);

    q_canPasteChanged();
    emit readOnlyChanged(r);
}

bool QQuickTextEdit::isReadOnly() const
{
    Q_D(const QQuickTextEdit);
    return !(d->control->textInteractionFlags() & Qt::TextEditable);
}

/*!
    Sets how the text edit should interact with user input to the given
    \a flags.
*/
void QQuickTextEdit::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    Q_D(QQuickTextEdit);
    d->control->setTextInteractionFlags(flags);
}

/*!
    Returns the flags specifying how the text edit should interact
    with user input.
*/
Qt::TextInteractionFlags QQuickTextEdit::textInteractionFlags() const
{
    Q_D(const QQuickTextEdit);
    return d->control->textInteractionFlags();
}

/*!
    \qmlproperty rectangle QtQuick2::TextEdit::cursorRectangle

    The rectangle where the standard text cursor is rendered
    within the text edit. Read-only.

    The position and height of a custom cursorDelegate are updated to follow the cursorRectangle
    automatically when it changes.  The width of the delegate is unaffected by changes in the
    cursor rectangle.
*/
QRect QQuickTextEdit::cursorRectangle() const
{
    Q_D(const QQuickTextEdit);
    return d->control->cursorRect().toRect().translated(0,d->yoff);
}

bool QQuickTextEdit::event(QEvent *event)
{
    Q_D(QQuickTextEdit);
    if (event->type() == QEvent::ShortcutOverride) {
        d->control->processEvent(event, QPointF(0, -d->yoff));
        return event->isAccepted();
    }
    return QQuickImplicitSizeItem::event(event);
}

/*!
\overload
Handles the given key \a event.
*/
void QQuickTextEdit::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickTextEdit);
    d->control->processEvent(event, QPointF(0, -d->yoff));
    if (!event->isAccepted())
        QQuickImplicitSizeItem::keyPressEvent(event);
}

/*!
\overload
Handles the given key \a event.
*/
void QQuickTextEdit::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickTextEdit);
    d->control->processEvent(event, QPointF(0, -d->yoff));
    if (!event->isAccepted())
        QQuickImplicitSizeItem::keyReleaseEvent(event);
}

/*!
    \qmlmethod void QtQuick2::TextEdit::deselect()

    Removes active text selection.
*/
void QQuickTextEdit::deselect()
{
    Q_D(QQuickTextEdit);
    QTextCursor c = d->control->textCursor();
    c.clearSelection();
    d->control->setTextCursor(c);
}

/*!
    \qmlmethod void QtQuick2::TextEdit::selectAll()

    Causes all text to be selected.
*/
void QQuickTextEdit::selectAll()
{
    Q_D(QQuickTextEdit);
    d->control->selectAll();
}

/*!
    \qmlmethod void QtQuick2::TextEdit::selectWord()

    Causes the word closest to the current cursor position to be selected.
*/
void QQuickTextEdit::selectWord()
{
    Q_D(QQuickTextEdit);
    QTextCursor c = d->control->textCursor();
    c.select(QTextCursor::WordUnderCursor);
    d->control->setTextCursor(c);
}

/*!
    \qmlmethod void QtQuick2::TextEdit::select(int start, int end)

    Causes the text from \a start to \a end to be selected.

    If either start or end is out of range, the selection is not changed.

    After calling this, selectionStart will become the lesser
    and selectionEnd will become the greater (regardless of the order passed
    to this method).

    \sa selectionStart, selectionEnd
*/
void QQuickTextEdit::select(int start, int end)
{
    Q_D(QQuickTextEdit);
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
    \qmlmethod void QtQuick2::TextEdit::isRightToLeft(int start, int end)

    Returns true if the natural reading direction of the editor text
    found between positions \a start and \a end is right to left.
*/
bool QQuickTextEdit::isRightToLeft(int start, int end)
{
    Q_D(QQuickTextEdit);
    if (start > end) {
        qmlInfo(this) << "isRightToLeft(start, end) called with the end property being smaller than the start.";
        return false;
    } else {
        return d->text.mid(start, end - start).isRightToLeft();
    }
}

#ifndef QT_NO_CLIPBOARD
/*!
    \qmlmethod QtQuick2::TextEdit::cut()

    Moves the currently selected text to the system clipboard.
*/
void QQuickTextEdit::cut()
{
    Q_D(QQuickTextEdit);
    d->control->cut();
}

/*!
    \qmlmethod QtQuick2::TextEdit::copy()

    Copies the currently selected text to the system clipboard.
*/
void QQuickTextEdit::copy()
{
    Q_D(QQuickTextEdit);
    d->control->copy();
}

/*!
    \qmlmethod QtQuick2::TextEdit::paste()

    Replaces the currently selected text by the contents of the system clipboard.
*/
void QQuickTextEdit::paste()
{
    Q_D(QQuickTextEdit);
    d->control->paste();
}
#endif // QT_NO_CLIPBOARD

/*!
\overload
Handles the given mouse \a event.
*/
void QQuickTextEdit::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickTextEdit);
    if (d->focusOnPress){
        bool hadActiveFocus = hasActiveFocus();
        forceActiveFocus();
        // re-open input panel on press if already focused
        if (hasActiveFocus() && hadActiveFocus && !isReadOnly())
            openSoftwareInputPanel();
    }
    d->control->processEvent(event, QPointF(0, -d->yoff));
    if (!event->isAccepted())
        QQuickImplicitSizeItem::mousePressEvent(event);
}

/*!
\overload
Handles the given mouse \a event.
*/
void QQuickTextEdit::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickTextEdit);
    d->control->processEvent(event, QPointF(0, -d->yoff));

    if (!event->isAccepted())
        QQuickImplicitSizeItem::mouseReleaseEvent(event);
}

/*!
\overload
Handles the given mouse \a event.
*/
void QQuickTextEdit::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QQuickTextEdit);
    d->control->processEvent(event, QPointF(0, -d->yoff));
    if (!event->isAccepted())
        QQuickImplicitSizeItem::mouseDoubleClickEvent(event);
}

/*!
\overload
Handles the given mouse \a event.
*/
void QQuickTextEdit::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickTextEdit);
    d->control->processEvent(event, QPointF(0, -d->yoff));
    if (!event->isAccepted())
        QQuickImplicitSizeItem::mouseMoveEvent(event);
}

/*!
\overload
Handles the given input method \a event.
*/
void QQuickTextEdit::inputMethodEvent(QInputMethodEvent *event)
{
    Q_D(QQuickTextEdit);
    const bool wasComposing = isInputMethodComposing();
    d->control->processEvent(event, QPointF(0, -d->yoff));
    if (wasComposing != isInputMethodComposing())
        emit inputMethodComposingChanged();
}

void QQuickTextEdit::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == ItemActiveFocusHasChanged) {
        setCursorVisible(value.boolValue); // ### refactor: focus handling && d->canvas && d->canvas->hasFocus());
    }
    QQuickItem::itemChange(change, value);
}

/*!
\overload
Returns the value of the given \a property.
*/
QVariant QQuickTextEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    Q_D(const QQuickTextEdit);

    QVariant v;
    switch (property) {
    case Qt::ImEnabled:
        v = (bool)(flags() & ItemAcceptsInputMethod);
        break;
    case Qt::ImHints:
        v = (int)inputMethodHints();
        break;
    default:
        v = d->control->inputMethodQuery(property);
        break;
    }
    return v;

}

void QQuickTextEdit::updateImageCache(const QRectF &)
{
    Q_D(QQuickTextEdit);

    // Do we really need the image cache?
    if (!d->richText || !d->useImageFallback) {
        if (!d->pixmapCache.isNull())
            d->pixmapCache = QPixmap();
        return;
    }

    if (width() != d->pixmapCache.width() || height() != d->pixmapCache.height())
        d->pixmapCache = QPixmap(width(), height());

    if (d->pixmapCache.isNull())
        return;

    // ### Use supplied rect, clear area and update only this part (for cursor updates)
    QRectF bounds = QRectF(0, 0, width(), height());
    d->pixmapCache.fill(Qt::transparent);
    {
        QPainter painter(&d->pixmapCache);

        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.translate(0, d->yoff);

        d->control->drawContents(&painter, bounds);
    }

}

QSGNode *QQuickTextEdit::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData);
    Q_D(QQuickTextEdit);

    QSGNode *currentNode = oldNode;
    if (d->richText && d->useImageFallback) {
        QSGImageNode *node = 0;
        if (oldNode == 0 || d->nodeType != QQuickTextEditPrivate::NodeIsTexture) {
            delete oldNode;
            node = QQuickItemPrivate::get(this)->sceneGraphContext()->createImageNode();
            d->texture = new QSGPlainTexture();
            d->nodeType = QQuickTextEditPrivate::NodeIsTexture;
            currentNode = node;
        } else {
            node = static_cast<QSGImageNode *>(oldNode);
        }

        qobject_cast<QSGPlainTexture *>(d->texture)->setImage(d->pixmapCache.toImage());
        node->setTexture(0);
        node->setTexture(d->texture);

        node->setTargetRect(QRectF(0, 0, d->pixmapCache.width(), d->pixmapCache.height()));
        node->setSourceRect(QRectF(0, 0, 1, 1));
        node->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        node->setVerticalWrapMode(QSGTexture::ClampToEdge);
        node->setFiltering(QSGTexture::Linear); // Nonsmooth text just ugly, so don't do that..
        node->update();

    } else if (oldNode == 0 || d->documentDirty) {
        d->documentDirty = false;

#if defined(Q_OS_MAC)
        // Make sure document is relayouted in the paint node on Mac
        // to avoid crashes due to the font engines created in the
        // shaping process
        d->document->markContentsDirty(0, d->document->characterCount());
#endif

        QQuickTextNode *node = 0;
        if (oldNode == 0 || d->nodeType != QQuickTextEditPrivate::NodeIsText) {
            delete oldNode;
            node = new QQuickTextNode(QQuickItemPrivate::get(this)->sceneGraphContext());
            d->nodeType = QQuickTextEditPrivate::NodeIsText;
            currentNode = node;
        } else {
            node = static_cast<QQuickTextNode *>(oldNode);
        }

        node->deleteContent();
        node->setMatrix(QMatrix4x4());

        QRectF bounds = boundingRect();

        QColor selectionColor = d->control->palette().color(QPalette::Highlight);
        QColor selectedTextColor = d->control->palette().color(QPalette::HighlightedText);
        node->addTextDocument(bounds.topLeft(), d->document, d->color, QQuickText::Normal, QColor(),
                              selectionColor, selectedTextColor, selectionStart(),
                              selectionEnd() - 1);  // selectionEnd() returns first char after
                                                    // selection

#if defined(Q_OS_MAC)
        // We also need to make sure the document layout is redone when
        // control is returned to the main thread, as all the font engines
        // are now owned by the rendering thread
        d->document->markContentsDirty(0, d->document->characterCount());
#endif
    }

    if (d->nodeType == QQuickTextEditPrivate::NodeIsText && d->cursorComponent == 0 && !isReadOnly()) {
        QQuickTextNode *node = static_cast<QQuickTextNode *>(currentNode);

        QColor color = (!d->cursorVisible || !d->control->cursorOn())
                ? QColor(0, 0, 0, 0)
                : d->color;

        if (node->cursorNode() == 0) {
            node->setCursor(cursorRectangle(), color);
        } else {
            node->cursorNode()->setRect(cursorRectangle());
            node->cursorNode()->setColor(color);
        }

    }

    return currentNode;
}

/*!
    \qmlproperty bool QtQuick2::TextEdit::smooth

    This property holds whether the text is smoothly scaled or transformed.

    Smooth filtering gives better visual quality, but is slower.  If
    the item is displayed at its natural size, this property has no visual or
    performance effect.

    \note Generally scaling artifacts are only visible if the item is stationary on
    the screen.  A common pattern when animating an item is to disable smooth
    filtering at the beginning of the animation and reenable it at the conclusion.
*/

/*!
    \qmlproperty bool QtQuick2::TextEdit::canPaste

    Returns true if the TextEdit is writable and the content of the clipboard is
    suitable for pasting into the TextEdit.
*/
bool QQuickTextEdit::canPaste() const
{
    Q_D(const QQuickTextEdit);
    if (!d->canPasteValid) {
        const_cast<QQuickTextEditPrivate *>(d)->canPaste = d->control->canPaste();
        const_cast<QQuickTextEditPrivate *>(d)->canPasteValid = true;
    }
    return d->canPaste;
}

/*!
    \qmlproperty bool QtQuick2::TextEdit::inputMethodComposing


    This property holds whether the TextEdit has partial text input from an
    input method.

    While it is composing an input method may rely on mouse or key events from
    the TextEdit to edit or commit the partial text.  This property can be used
    to determine when to disable events handlers that may interfere with the
    correct operation of an input method.
*/
bool QQuickTextEdit::isInputMethodComposing() const
{
    Q_D(const QQuickTextEdit);
    if (QTextLayout *layout = d->control->textCursor().block().layout())
        return layout->preeditAreaText().length() > 0;
    return false;
}

void QQuickTextEditPrivate::init()
{
    Q_Q(QQuickTextEdit);

    q->setSmooth(smooth);
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFlag(QQuickItem::ItemAcceptsInputMethod);
    q->setFlag(QQuickItem::ItemHasContents);

    document = new QQuickTextDocumentWithImageResources(q);

    control = new QQuickTextControl(document, q);
    control->setView(q);
    control->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByKeyboard | Qt::TextEditable);
    control->setAcceptRichText(false);

    // QQuickTextControl follows the default text color
    // defined by the platform, declarative text
    // should be black by default
    QPalette pal = control->palette();
    if (pal.color(QPalette::Text) != color) {
        pal.setColor(QPalette::Text, color);
        control->setPalette(pal);
    }

    QObject::connect(control, SIGNAL(updateRequest(QRectF)), q, SLOT(updateDocument()));
    QObject::connect(control, SIGNAL(updateCursorRequest()), q, SLOT(updateCursor()));
    QObject::connect(control, SIGNAL(textChanged()), q, SLOT(q_textChanged()));
    QObject::connect(control, SIGNAL(selectionChanged()), q, SIGNAL(selectionChanged()));
    QObject::connect(control, SIGNAL(selectionChanged()), q, SLOT(updateSelectionMarkers()));
    QObject::connect(control, SIGNAL(cursorPositionChanged()), q, SLOT(updateSelectionMarkers()));
    QObject::connect(control, SIGNAL(cursorPositionChanged()), q, SIGNAL(cursorPositionChanged()));
    QObject::connect(control, SIGNAL(cursorRectangleChanged()), q, SLOT(moveCursorDelegate()));
    QObject::connect(control, SIGNAL(linkActivated(QString)), q, SIGNAL(linkActivated(QString)));
#ifndef QT_NO_CLIPBOARD
    QObject::connect(QGuiApplication::clipboard(), SIGNAL(dataChanged()), q, SLOT(q_canPasteChanged()));
#endif

    document->setDefaultFont(font);
    document->setDocumentMargin(textMargin);
    document->setUndoRedoEnabled(false); // flush undo buffer.
    document->setUndoRedoEnabled(true);
    updateDefaultTextOption();
}

void QQuickTextEdit::q_textChanged()
{
    Q_D(QQuickTextEdit);
    d->text = text();
    d->rightToLeftText = d->document->begin().layout()->engine()->isRightToLeft();
    d->determineHorizontalAlignment();
    d->updateDefaultTextOption();
    updateSize();
    updateTotalLines();
    emit textChanged(d->text);
}

void QQuickTextEdit::moveCursorDelegate()
{
    Q_D(QQuickTextEdit);
    d->determineHorizontalAlignment();
    updateMicroFocus();
    emit cursorRectangleChanged();
    if (!d->cursor)
        return;
    QRectF cursorRect = cursorRectangle();
    d->cursor->setX(cursorRect.x());
    d->cursor->setY(cursorRect.y());
}

void QQuickTextEdit::updateSelectionMarkers()
{
    Q_D(QQuickTextEdit);
    if (d->lastSelectionStart != d->control->textCursor().selectionStart()) {
        d->lastSelectionStart = d->control->textCursor().selectionStart();
        emit selectionStartChanged();
    }
    if (d->lastSelectionEnd != d->control->textCursor().selectionEnd()) {
        d->lastSelectionEnd = d->control->textCursor().selectionEnd();
        emit selectionEndChanged();
    }
}

QRectF QQuickTextEdit::boundingRect() const
{
    Q_D(const QQuickTextEdit);
    QRectF r = QQuickImplicitSizeItem::boundingRect();
    int cursorWidth = 1;
    if (d->cursor)
        cursorWidth = d->cursor->width();
    if (!d->document->isEmpty())
        cursorWidth += 3;// ### Need a better way of accounting for space between char and cursor

    // Could include font max left/right bearings to either side of rectangle.

    r.setRight(r.right() + cursorWidth);
    return r.translated(0,d->yoff);
}

qreal QQuickTextEditPrivate::getImplicitWidth() const
{
    Q_Q(const QQuickTextEdit);
    if (!requireImplicitWidth) {
        // We don't calculate implicitWidth unless it is required.
        // We need to force a size update now to ensure implicitWidth is calculated
        const_cast<QQuickTextEditPrivate*>(this)->requireImplicitWidth = true;
        const_cast<QQuickTextEdit*>(q)->updateSize();
    }
    return implicitWidth;
}

//### we should perhaps be a bit smarter here -- depending on what has changed, we shouldn't
//    need to do all the calculations each time
void QQuickTextEdit::updateSize()
{
    Q_D(QQuickTextEdit);
    if (isComponentComplete()) {
        qreal naturalWidth = d->implicitWidth;
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
        if (nyoff != d->yoff)
            d->yoff = nyoff;
        setBaselineOffset(fm.ascent() + d->yoff + d->textMargin);

        //### need to comfirm cost of always setting these
        int newWidth = qCeil(d->document->idealWidth());
        if (!widthValid() && d->document->textWidth() != newWidth)
            d->document->setTextWidth(newWidth); // ### Text does not align if width is not set (QTextDoc bug)
        // ### Setting the implicitWidth triggers another updateSize(), and unless there are bindings nothing has changed.
        qreal iWidth = -1;
        if (!widthValid())
            iWidth = newWidth;
        else if (d->requireImplicitWidth)
            iWidth = naturalWidth;
        qreal newHeight = d->document->isEmpty() ? fm.height() : (int)d->document->size().height();
        if (iWidth > -1)
            setImplicitSize(iWidth, newHeight);
        else
            setImplicitHeight(newHeight);

        d->paintedSize = QSize(newWidth, newHeight);
        emit paintedSizeChanged();
    } else {
        d->dirty = true;
    }
    updateDocument();
}

void QQuickTextEdit::updateDocument()
{
    Q_D(QQuickTextEdit);
    d->documentDirty = true;

    if (isComponentComplete()) {
        updateImageCache();
        update();
    }
}

void QQuickTextEdit::updateCursor()
{
    Q_D(QQuickTextEdit);
    if (isComponentComplete()) {
        updateImageCache(d->control->cursorRect());
        update();
    }
}

void QQuickTextEdit::updateTotalLines()
{
    Q_D(QQuickTextEdit);

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

void QQuickTextEditPrivate::updateDefaultTextOption()
{
    Q_Q(QQuickTextEdit);
    QTextOption opt = document->defaultTextOption();
    int oldAlignment = opt.alignment();

    QQuickTextEdit::HAlignment horizontalAlignment = q->effectiveHAlign();
    if (rightToLeftText) {
        if (horizontalAlignment == QQuickTextEdit::AlignLeft)
            horizontalAlignment = QQuickTextEdit::AlignRight;
        else if (horizontalAlignment == QQuickTextEdit::AlignRight)
            horizontalAlignment = QQuickTextEdit::AlignLeft;
    }
    opt.setAlignment((Qt::Alignment)(int)(horizontalAlignment | vAlign));

    QTextOption::WrapMode oldWrapMode = opt.wrapMode();
    opt.setWrapMode(QTextOption::WrapMode(wrapMode));

    bool oldUseDesignMetrics = opt.useDesignMetrics();
    bool useDesignMetrics = !qmlDisableDistanceField();
    opt.setUseDesignMetrics(useDesignMetrics);

    if (oldWrapMode == opt.wrapMode()
            && oldAlignment == opt.alignment()
            && oldUseDesignMetrics == useDesignMetrics) {
        return;
    }
    document->setDefaultTextOption(opt);
}



/*!
    \qmlmethod void QtQuick2::TextEdit::openSoftwareInputPanel()

    Opens software input panels like virtual keyboards for typing, useful for
    customizing when you want the input keyboard to be shown and hidden in
    your application.

    By default the opening of input panels follows the platform style. Input panels are
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
void QQuickTextEdit::openSoftwareInputPanel()
{
    if (qGuiApp)
        qGuiApp->inputPanel()->show();
}

/*!
    \qmlmethod void QtQuick2::TextEdit::closeSoftwareInputPanel()

    Closes a software input panel like a virtual keyboard shown on the screen, useful
    for customizing when you want the input keyboard to be shown and hidden in
    your application.

    By default the opening of input panels follows the platform style. Input panels are
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
void QQuickTextEdit::closeSoftwareInputPanel()
{
    if (qGuiApp)
        qGuiApp->inputPanel()->hide();
}

void QQuickTextEdit::focusInEvent(QFocusEvent *event)
{
    Q_D(const QQuickTextEdit);
    if (d->focusOnPress && !isReadOnly())
        openSoftwareInputPanel();
    QQuickImplicitSizeItem::focusInEvent(event);
}

void QQuickTextEdit::q_canPasteChanged()
{
    Q_D(QQuickTextEdit);
    bool old = d->canPaste;
    d->canPaste = d->control->canPaste();
    bool changed = old!=d->canPaste || !d->canPasteValid;
    d->canPasteValid = true;
    if (changed)
        emit canPasteChanged();
}

/*!
    \qmlmethod string QtQuick2::TextEdit::getText(int start, int end)

    Returns the section of text that is between the \a start and \a end positions.

    The returned text does not include any rich text formatting.
*/

QString QQuickTextEdit::getText(int start, int end) const
{
    Q_D(const QQuickTextEdit);
    start = qBound(0, start, d->document->characterCount() - 1);
    end = qBound(0, end, d->document->characterCount() - 1);
    QTextCursor cursor(d->document);
    cursor.setPosition(start, QTextCursor::MoveAnchor);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    return cursor.selectedText();
}

/*!
    \qmlmethod string QtQuick2::TextEdit::getFormattedText(int start, int end)

    Returns the section of text that is between the \a start and \a end positions.

    The returned text will be formatted according the \l textFormat property.
*/

QString QQuickTextEdit::getFormattedText(int start, int end) const
{
    Q_D(const QQuickTextEdit);

    start = qBound(0, start, d->document->characterCount() - 1);
    end = qBound(0, end, d->document->characterCount() - 1);

    QTextCursor cursor(d->document);
    cursor.setPosition(start, QTextCursor::MoveAnchor);
    cursor.setPosition(end, QTextCursor::KeepAnchor);

    if (d->richText) {
#ifndef QT_NO_TEXTHTMLPARSER
        return cursor.selection().toHtml();
#else
        return cursor.selection().toPlainText();
#endif
    } else {
        return cursor.selection().toPlainText();
    }
}

/*!
    \qmlmethod void QtQuick2::TextEdit::insert(int position, string text)

    Inserts \a text into the TextEdit at position.
*/
void QQuickTextEdit::insert(int position, const QString &text)
{
    Q_D(QQuickTextEdit);
    if (position < 0 || position >= d->document->characterCount())
        return;
    QTextCursor cursor(d->document);
    cursor.setPosition(position);
    d->richText = d->richText || (d->format == AutoText && Qt::mightBeRichText(text));
    if (d->richText) {
#ifndef QT_NO_TEXTHTMLPARSER
        cursor.insertHtml(text);
#else
        cursor.insertText(text);
#endif
    } else {
        cursor.insertText(text);
    }
}

/*!
    \qmlmethod string QtQuick2::TextEdit::getText(int start, int end)

    Removes the section of text that is between the \a start and \a end positions from the TextEdit.
*/

void QQuickTextEdit::remove(int start, int end)
{
    Q_D(QQuickTextEdit);
    start = qBound(0, start, d->document->characterCount() - 1);
    end = qBound(0, end, d->document->characterCount() - 1);
    QTextCursor cursor(d->document);
    cursor.setPosition(start, QTextCursor::MoveAnchor);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
}

QT_END_NAMESPACE
