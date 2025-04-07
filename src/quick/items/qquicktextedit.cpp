// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qquicktextedit_p.h"
#include "qquicktextedit_p_p.h"
#include "qquicktextcontrol_p.h"
#include "qquicktextdocument_p.h"
#include "qquickwindow.h"
#include "qsginternaltextnode_p.h"
#include "qquicktextnodeengine_p.h"

#include <QtCore/qmath.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qevent.h>
#include <QtGui/qpainter.h>
#include <QtGui/qtextobject.h>
#include <QtGui/qtexttable.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick/qsgsimplerectnode.h>

#include <private/qqmlglobal_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qtextengine_p.h>
#include <private/qsgadaptationlayer_p.h>
#include <QtQuick/private/qquickpixmapcache_p.h>

#if QT_CONFIG(accessibility)
#include <private/qquickaccessibleattached_p.h>
#endif

#include "qquicktextdocument.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcTextEdit, "qt.quick.textedit")

using namespace Qt::StringLiterals;

/*!
    \qmltype TextEdit
    \nativetype QQuickTextEdit
    \inqmlmodule QtQuick
    \ingroup qtquick-visual
    \ingroup qtquick-input
    \inherits Item
    \brief Displays multiple lines of editable formatted text.

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
    to a look and feel. For example, to add flickable scrolling that follows the cursor:

    \snippet qml/texteditor.qml 0

    A particular look and feel might use smooth scrolling (eg. using SmoothedAnimation), might have a visible
    scrollbar, or a scrollbar that fades in to show location, etc.

    Clipboard support is provided by the cut(), copy(), and paste() functions.
    Text can be selected by mouse in the usual way, unless \l selectByMouse is
    set to \c false; and by keyboard with the \c {Shift+arrow} key
    combinations, unless \l selectByKeyboard is set to \c false. To select text
    programmatically, you can set the \l selectionStart and \l selectionEnd
    properties, or use \l selectAll() or \l selectWord().

    You can translate between cursor positions (characters from the start of the document) and pixel
    points using positionAt() and positionToRectangle().

    \sa Text, TextInput, TextArea, {Qt Quick Controls - Text Editor}
*/

/*!
    \qmlsignal QtQuick::TextEdit::linkActivated(string link)

    This signal is emitted when the user clicks on a link embedded in the text.
    The link must be in rich text or HTML format and the
    \a link string provides access to the particular link.
*/

// This is a pretty arbitrary figure. The idea is that we don't want to break down the document
// into text nodes corresponding to a text block each so that the glyph node grouping doesn't become pointless.
static const int nodeBreakingSize = 300;

#if !defined(QQUICKTEXT_LARGETEXT_THRESHOLD)
  #define QQUICKTEXT_LARGETEXT_THRESHOLD 10000
#endif
// if QString::size() > largeTextSizeThreshold, we render more often, but only visible lines
const int QQuickTextEditPrivate::largeTextSizeThreshold = QQUICKTEXT_LARGETEXT_THRESHOLD;

namespace {
    class RootNode : public QSGTransformNode
    {
    public:
        RootNode() : cursorNode(nullptr), frameDecorationsNode(nullptr)
        { }

        void resetFrameDecorations(QSGInternalTextNode* newNode)
        {
            if (frameDecorationsNode) {
                removeChildNode(frameDecorationsNode);
                delete frameDecorationsNode;
            }
            frameDecorationsNode = newNode;
            newNode->setFlag(QSGNode::OwnedByParent);
        }

        void resetCursorNode(QSGInternalRectangleNode* newNode)
        {
            if (cursorNode)
                removeChildNode(cursorNode);
            delete cursorNode;
            cursorNode = newNode;
            if (cursorNode) {
                appendChildNode(cursorNode);
                cursorNode->setFlag(QSGNode::OwnedByParent);
            }
        }

        QSGInternalRectangleNode *cursorNode;
        QSGInternalTextNode* frameDecorationsNode;

    };
}

QQuickTextEdit::QQuickTextEdit(QQuickItem *parent)
: QQuickImplicitSizeItem(*(new QQuickTextEditPrivate), parent)
{
    Q_D(QQuickTextEdit);
    d->init();
}

QQuickTextEdit::~QQuickTextEdit()
{
    Q_D(QQuickTextEdit);
    qDeleteAll(d->pixmapsInProgress);
}

QQuickTextEdit::QQuickTextEdit(QQuickTextEditPrivate &dd, QQuickItem *parent)
: QQuickImplicitSizeItem(dd, parent)
{
    Q_D(QQuickTextEdit);
    d->init();
}

QString QQuickTextEdit::text() const
{
    Q_D(const QQuickTextEdit);
    if (!d->textCached && isComponentComplete()) {
        QQuickTextEditPrivate *d = const_cast<QQuickTextEditPrivate *>(d_func());
#if QT_CONFIG(texthtmlparser)
        if (d->richText)
            d->text = d->control->toHtml();
        else
#endif
#if QT_CONFIG(textmarkdownwriter)
        if (d->markdownText)
            d->text = d->control->toMarkdown();
        else
#endif
            d->text = d->control->toPlainText();
        d->textCached = true;
    }
    return d->text;
}

/*!
    \qmlproperty string QtQuick::TextEdit::font.family

    Sets the family name of the font.

    The family name is case insensitive and may optionally include a foundry name, e.g. "Helvetica [Cronyx]".
    If the family is available from more than one foundry and the foundry isn't specified, an arbitrary foundry is chosen.
    If the family isn't available a family will be set using the font matching algorithm.
*/

/*!
    \qmlproperty string QtQuick::TextEdit::font.styleName
    \since 5.6

    Sets the style name of the font.

    The style name is case insensitive. If set, the font will be matched against style name instead
    of the font properties \l font.weight, \l font.bold and \l font.italic.
*/


/*!
    \qmlproperty bool QtQuick::TextEdit::font.bold

    Sets whether the font weight is bold.
*/

/*!
    \qmlproperty int QtQuick::TextEdit::font.weight

    The requested weight of the font. The weight requested must be an integer
    between 1 and 1000, or one of the predefined values:

    \value Font.Thin        100
    \value Font.ExtraLight  200
    \value Font.Light       300
    \value Font.Normal      400 (default)
    \value Font.Medium      500
    \value Font.DemiBold    600
    \value Font.Bold        700
    \value Font.ExtraBold   800
    \value Font.Black       900

    \qml
    TextEdit { text: "Hello"; font.weight: Font.DemiBold }
    \endqml
*/

/*!
    \qmlproperty bool QtQuick::TextEdit::font.italic

    Sets whether the font has an italic style.
*/

/*!
    \qmlproperty bool QtQuick::TextEdit::font.underline

    Sets whether the text is underlined.
*/

/*!
    \qmlproperty bool QtQuick::TextEdit::font.strikeout

    Sets whether the font has a strikeout style.
*/

/*!
    \qmlproperty real QtQuick::TextEdit::font.pointSize

    Sets the font size in points. The point size must be greater than zero.
*/

/*!
    \qmlproperty int QtQuick::TextEdit::font.pixelSize

    Sets the font size in pixels.

    Using this function makes the font device dependent.  Use
    \l{TextEdit::font.pointSize} to set the size of the font in a
    device independent manner.
*/

/*!
    \qmlproperty real QtQuick::TextEdit::font.letterSpacing

    Sets the letter spacing for the font.

    Letter spacing changes the default spacing between individual letters in the font.
    A positive value increases the letter spacing by the corresponding pixels; a negative value decreases the spacing.
*/

/*!
    \qmlproperty real QtQuick::TextEdit::font.wordSpacing

    Sets the word spacing for the font.

    Word spacing changes the default spacing between individual words.
    A positive value increases the word spacing by a corresponding amount of pixels,
    while a negative value decreases the inter-word spacing accordingly.
*/

/*!
    \qmlproperty enumeration QtQuick::TextEdit::font.capitalization

    Sets the capitalization for the text.

    \value Font.MixedCase       no capitalization change is applied
    \value Font.AllUppercase    alters the text to be rendered in all uppercase type
    \value Font.AllLowercase    alters the text to be rendered in all lowercase type
    \value Font.SmallCaps       alters the text to be rendered in small-caps type
    \value Font.Capitalize      alters the text to be rendered with the first character of
                                each word as an uppercase character

    \qml
    TextEdit { text: "Hello"; font.capitalization: Font.AllLowercase }
    \endqml
*/

/*!
    \qmlproperty enumeration QtQuick::TextEdit::font.hintingPreference
    \since 5.8

    Sets the preferred hinting on the text. This is a hint to the underlying text rendering system
    to use a certain level of hinting, and has varying support across platforms. See the table in
    the documentation for QFont::HintingPreference for more details.

    \note This property only has an effect when used together with render type TextEdit.NativeRendering.

    \value Font.PreferDefaultHinting    Use the default hinting level for the target platform.
    \value Font.PreferNoHinting         If possible, render text without hinting the outlines
           of the glyphs. The text layout will be typographically accurate, using the same metrics
           as are used e.g. when printing.
    \value Font.PreferVerticalHinting   If possible, render text with no horizontal hinting,
           but align glyphs to the pixel grid in the vertical direction. The text will appear
           crisper on displays where the density is too low to give an accurate rendering
           of the glyphs. But since the horizontal metrics of the glyphs are unhinted, the text's
           layout will be scalable to higher density devices (such as printers) without impacting
           details such as line breaks.
    \value Font.PreferFullHinting       If possible, render text with hinting in both horizontal and
           vertical directions. The text will be altered to optimize legibility on the target
           device, but since the metrics will depend on the target size of the text, the positions
           of glyphs, line breaks, and other typographical detail will not scale, meaning that a
           text layout may look different on devices with different pixel densities.

    \qml
    TextEdit { text: "Hello"; renderType: TextEdit.NativeRendering; font.hintingPreference: Font.PreferVerticalHinting }
    \endqml
*/

/*!
    \qmlproperty bool QtQuick::TextEdit::font.kerning
    \since 5.10

    Enables or disables the kerning OpenType feature when shaping the text. Disabling this may
    improve performance when creating or changing the text, at the expense of some cosmetic
    features. The default value is true.

    \qml
    TextEdit { text: "OATS FLAVOUR WAY"; kerning: font.false }
    \endqml
*/

/*!
    \qmlproperty bool QtQuick::TextEdit::font.preferShaping
    \since 5.10

    Sometimes, a font will apply complex rules to a set of characters in order to
    display them correctly. In some writing systems, such as Brahmic scripts, this is
    required in order for the text to be legible, but in e.g. Latin script, it is merely
    a cosmetic feature. Setting the \c preferShaping property to false will disable all
    such features when they are not required, which will improve performance in most cases.

    The default value is true.

    \qml
    TextEdit { text: "Some text"; font.preferShaping: false }
    \endqml
*/

/*!
    \qmlproperty object QtQuick::TextEdit::font.variableAxes
    \since 6.7

    \include qquicktext.cpp qml-font-variable-axes
*/

/*!
    \qmlproperty object QtQuick::TextEdit::font.features
    \since 6.6

    \include qquicktext.cpp qml-font-features
*/

/*!
    \qmlproperty bool QtQuick::TextEdit::font.contextFontMerging
    \since 6.8

    \include qquicktext.cpp qml-font-context-font-merging
*/

/*!
    \qmlproperty bool QtQuick::TextEdit::font.preferTypoLineMetrics
    \since 6.8

    \include qquicktext.cpp qml-font-prefer-typo-line-metrics
*/

/*!
    \qmlproperty string QtQuick::TextEdit::text

    The text to display.  If the text format is AutoText the text edit will
    automatically determine whether the text should be treated as
    rich text.  This determination is made using Qt::mightBeRichText().
    However, detection of Markdown is not automatic.

    The text-property is mostly suitable for setting the initial content and
    handling modifications to relatively small text content. The append(),
    insert() and remove() methods provide more fine-grained control and
    remarkably better performance for modifying especially large rich text
    content.

    Note that some keyboards use a predictive function. In this case,
    the text being composed by the input method is not part of this property.
    The part of the text related to the predictions is underlined and stored in
    the \l preeditText property.

    If you used \l TextDocument::source to load text, you can retrieve the
    loaded text from this property. In that case, you can then change
    \l textFormat to do format conversions that will change the value of the
    \c text property. For example, if \c textFormat is \c RichText or
    \c AutoText and you load an HTML file, then set \c textFormat to
    \c MarkdownText afterwards, the \c text property will contain the
    conversion from HTML to Markdown.

    \sa clear(), preeditText, textFormat
*/
void QQuickTextEdit::setText(const QString &text)
{
    Q_D(QQuickTextEdit);
    if (QQuickTextEdit::text() == text)
        return;

    d->richText = d->format == RichText || (d->format == AutoText && Qt::mightBeRichText(text));
    d->markdownText = d->format == MarkdownText;
    if (!isComponentComplete()) {
        d->text = text;
    } else if (d->richText) {
#if QT_CONFIG(texthtmlparser)
        d->control->setHtml(text);
#else
        d->control->setPlainText(text);
#endif
    } else if (d->markdownText) {
        d->control->setMarkdownText(text);
    } else {
        d->control->setPlainText(text);
    }
    setFlag(QQuickItem::ItemObservesViewport, text.size() > QQuickTextEditPrivate::largeTextSizeThreshold);
}

void QQuickTextEdit::invalidate()
{
    QMetaObject::invokeMethod(this, &QQuickTextEdit::q_invalidate);
}

void QQuickTextEdit::q_invalidate()
{
    Q_D(QQuickTextEdit);
    if (isComponentComplete()) {
        if (d->document != nullptr)
            d->document->markContentsDirty(0, d->document->characterCount());
        invalidateFontCaches();
        d->updateType = QQuickTextEditPrivate::UpdateAll;
        update();
    }
}

/*!
    \qmlproperty string QtQuick::TextEdit::preeditText
    \readonly
    \since 5.7

    This property contains partial text input from an input method.

    To turn off partial text that results from predictions, set the \c Qt.ImhNoPredictiveText
    flag in inputMethodHints.

    \sa inputMethodHints
*/
QString QQuickTextEdit::preeditText() const
{
    Q_D(const QQuickTextEdit);
    return d->control->preeditText();
}

/*!
    \qmlproperty enumeration QtQuick::TextEdit::textFormat

    The way the \l text property should be displayed.

    Supported text formats are:

    \value TextEdit.PlainText       (default) all styling tags are treated as plain text
    \value TextEdit.AutoText        detected via the Qt::mightBeRichText() heuristic
                                    or the file format of \l TextDocument::source
    \value TextEdit.RichText        \l {Supported HTML Subset} {a subset of HTML 4}
    \value TextEdit.MarkdownText    \l {https://commonmark.org/help/}{CommonMark} plus the
                                    \l {https://guides.github.com/features/mastering-markdown/}{GitHub}
                                    extensions for tables and task lists (since 5.14)

    The default is \c TextEdit.PlainText. If the text format is set to
    \c TextEdit.AutoText, the text edit will automatically determine whether
    the text should be treated as rich text. If the \l text property is set,
    this determination is made using Qt::mightBeRichText(), which can detect
    the presence of an HTML tag on the first line of text, but cannot
    distinguish Markdown from plain text. If the \l TextDocument::source
    property is set, this determination is made from the
    \l {QMimeDatabase::mimeTypeForFile()}{mime type of the file}.

    \table
    \row
    \li
    \snippet qml/text/textEditFormats.qml 0
    \li \image declarative-textformat.png
    \endtable

    With \c TextEdit.MarkdownText, checkboxes that result from using the
    \l {https://guides.github.com/features/mastering-markdown/#GitHub-flavored-markdown}{GitHub checkbox extension}
    are interactively checkable.

    If the \l TextDocument::source property is set, changing the \c textFormat
    property after loading has the effect of converting from the detected
    format to the requested format. For example, you can convert between HTML
    and Markdown. However if either of those "rich" formats is loaded and then
    you set \c textFormat to \c PlainText, the TextEdit will show the raw
    markup. Thus, suitable bindings (e.g. to a checkable Control) can enable
    the user to toggle back and forth between "raw" and WYSIWYG editing.

    \note Interactively typing markup or markdown formatting in WYSIWYG mode
    is not supported; but you can switch to \c PlainText, make changes, then
    switch back to the appropriate \c textFormat.

    \note With \c Text.MarkdownText, and with the supported subset of HTML,
    some decorative elements are not rendered as they would be in a web browser:
    \list
    \li code blocks use the \l {QFontDatabase::FixedFont}{default monospace font} but without a surrounding highlight box
    \li block quotes are indented, but there is no vertical line alongside the quote
    \endlist
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

    auto mightBeRichText = [this]() {
        return Qt::mightBeRichText(text());
    };

    auto findSourceFormat = [d, mightBeRichText](Qt::TextFormat detectedFormat) {
        if (d->format == PlainText)
            return PlainText;
        if (d->richText) return RichText;
        if (d->markdownText) return MarkdownText;
        if (detectedFormat == Qt::AutoText && mightBeRichText())
            return RichText;
        return PlainText;
    };

    auto findDestinationFormat = [format, mightBeRichText](Qt::TextFormat detectedFormat, TextFormat sourceFormat) {
        if (format == AutoText) {
            if (detectedFormat == Qt::MarkdownText || (detectedFormat == Qt::AutoText && sourceFormat == MarkdownText))
                return MarkdownText;
            if (detectedFormat == Qt::RichText || (detectedFormat == Qt::AutoText && (sourceFormat == RichText || mightBeRichText())))
                return RichText;
            return PlainText; // fallback
        }
        return format;
    };

    bool textCachedChanged = false;
    bool converted = false;

    if (isComponentComplete()) {
        Qt::TextFormat detectedFormat = Qt::AutoText; // default if we don't know
        if (d->quickDocument) {
            // If QQuickTextDocument is in use, content can be loaded from a file,
            // and then mime type detection overrides mightBeRichText().
            detectedFormat = QQuickTextDocumentPrivate::get(d->quickDocument)->detectedFormat;
        }

        const TextFormat sourceFormat = findSourceFormat(detectedFormat);
        const TextFormat destinationFormat = findDestinationFormat(detectedFormat, sourceFormat);

        d->richText = destinationFormat == RichText;
        d->markdownText = destinationFormat == MarkdownText;

        // If converting between markdown and HTML, avoid using cached text: have QTD re-generate it
        if (format != PlainText && (sourceFormat != destinationFormat)) {
            d->textCached = false;
            textCachedChanged = true;
        }

        switch (destinationFormat) {
        case PlainText:
#if QT_CONFIG(texthtmlparser)
            if (sourceFormat == RichText) {
                // If rich or unknown text was loaded and now the user wants plain text, get the raw HTML.
                // But if we didn't set textCached to false above, assume d->text already contains HTML.
                // This will allow the user to see the actual HTML they loaded (rather than Qt regenerating crufty HTML).
                d->control->setPlainText(d->textCached ? d->text : d->control->toHtml());
                converted = true;
            }
#endif
#if QT_CONFIG(textmarkdownwriter) && QT_CONFIG(textmarkdownreader)
            if (sourceFormat == MarkdownText) {
                // If markdown or unknown text was loaded and now the user wants plain text, get the raw Markdown.
                // But if we didn't set textCached to false above, assume d->text already contains markdown.
                // This will allow the user to see the actual markdown they loaded.
                d->control->setPlainText(d->textCached ? d->text : d->control->toMarkdown());
                converted = true;
            }
#endif
            break;
        case RichText:
#if QT_CONFIG(texthtmlparser)
            switch (sourceFormat) {
            case MarkdownText:
                // If markdown was loaded and now the user wants HTML, convert markdown to HTML.
                d->control->setHtml(d->control->toHtml());
                converted = true;
                break;
            case PlainText:
                // If plain text was loaded and now the user wants HTML, interpret plain text as HTML.
                // But if we didn't set textCached to false above, assume d->text already contains HTML.
                d->control->setHtml(d->textCached ? d->text : d->control->toPlainText());
                converted = true;
                break;
            case AutoText:
            case RichText: // nothing to do
                break;
            }
#endif
            break;
        case MarkdownText:
#if QT_CONFIG(textmarkdownwriter) && QT_CONFIG(textmarkdownreader)
            switch (sourceFormat) {
            case RichText:
                // If HTML was loaded and now the user wants markdown, convert HTML to markdown.
                d->control->setMarkdownText(d->control->toMarkdown());
                converted = true;
                break;
            case PlainText:
                // If plain text was loaded and now the user wants markdown, interpret plain text as markdown.
                // But if we didn't set textCached to false above, assume d->text already contains markdown.
                d->control->setMarkdownText(d->textCached ? d->text : d->control->toPlainText());
                converted = true;
                break;
            case AutoText:
            case MarkdownText: // nothing to do
                break;
            }
#endif
            break;
        case AutoText: // nothing to do
            break;
        }

        if (converted)
            updateSize();
    } else {
        d->richText = format == RichText || (format == AutoText && (d->richText || mightBeRichText()));
        d->markdownText = format == MarkdownText;
    }

    qCDebug(lcTextEdit) << d->format << "->" << format
                        << "rich?" << d->richText << "md?" << d->markdownText
                        << "converted?" << converted << "cache invalidated?" << textCachedChanged;

    d->format = format;
    d->control->setAcceptRichText(d->format != PlainText);
    emit textFormatChanged(d->format);
    if (textCachedChanged)
        emit textChanged();
}

/*!
    \qmlproperty enumeration QtQuick::TextEdit::renderType

    Override the default rendering type for this component.

    Supported render types are:

    \value TextEdit.QtRendering     Text is rendered using a scalable distance field for each glyph.
    \value TextEdit.NativeRendering Text is rendered using a platform-specific technique.
    \value TextEdit.CurveRendering  Text is rendered using a curve rasterizer running directly on
                                    the graphics hardware. (Introduced in Qt 6.7.0.)

    Select \c TextEdit.NativeRendering if you prefer text to look native on the target platform and do
    not require advanced features such as transformation of the text. Using such features in
    combination with the NativeRendering render type will lend poor and sometimes pixelated
    results.

    Both \c TextEdit.QtRendering and \c TextEdit.CurveRendering are hardware-accelerated techniques.
    \c QtRendering is the faster of the two, but uses more memory and will exhibit rendering
    artifacts at large sizes. \c CurveRendering should be considered as an alternative in cases
    where \c QtRendering does not give good visual results or where reducing graphics memory
    consumption is a priority.

    The default rendering type is determined by \l QQuickWindow::textRenderType().
*/
QQuickTextEdit::RenderType QQuickTextEdit::renderType() const
{
    Q_D(const QQuickTextEdit);
    return d->renderType;
}

void QQuickTextEdit::setRenderType(QQuickTextEdit::RenderType renderType)
{
    Q_D(QQuickTextEdit);
    if (d->renderType == renderType)
        return;

    d->renderType = renderType;
    emit renderTypeChanged();
    d->updateDefaultTextOption();

    if (isComponentComplete())
        updateSize();
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
        if (d->cursorItem) {
            d->cursorItem->setHeight(QFontMetrics(d->font).height());
            moveCursorDelegate();
        }
        updateSize();
        updateWholeDocument();
#if QT_CONFIG(im)
        updateInputMethod(Qt::ImCursorRectangle | Qt::ImAnchorRectangle | Qt::ImFont);
#endif
    }
    emit fontChanged(d->sourceFont);
}

/*!
    \qmlproperty color QtQuick::TextEdit::color

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
    updateWholeDocument();
    emit colorChanged(d->color);
}

/*!
    \qmlproperty color QtQuick::TextEdit::selectionColor

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
    updateWholeDocument();
    emit selectionColorChanged(d->selectionColor);
}

/*!
    \qmlproperty color QtQuick::TextEdit::selectedTextColor

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
    updateWholeDocument();
    emit selectedTextColorChanged(d->selectedTextColor);
}

/*!
    \qmlproperty enumeration QtQuick::TextEdit::horizontalAlignment
    \qmlproperty enumeration QtQuick::TextEdit::verticalAlignment
    \qmlproperty enumeration QtQuick::TextEdit::effectiveHorizontalAlignment

    Sets the horizontal and vertical alignment of the text within the TextEdit item's
    width and height. By default, the text alignment follows the natural alignment
    of the text, for example text that is read from left to right will be aligned to
    the left.

    Valid values for \c horizontalAlignment are:

    \value TextEdit.AlignLeft
        left alignment with ragged edges on the right (default)
    \value TextEdit.AlignRight
        align each line to the right with ragged edges on the left
    \value TextEdit.AlignHCenter
        align each line to the center
    \value TextEdit.AlignJustify
        align each line to both right and left, spreading out words as necessary

    Valid values for \c verticalAlignment are:

    \value TextEdit.AlignTop        start at the top of the item (default)
    \value TextEdit.AlignBottom     align the last line to the bottom and other lines above
    \value TextEdit.AlignVCenter    align the center vertically

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

    if (d->setHAlign(align, true) && isComponentComplete()) {
        d->updateDefaultTextOption();
        updateSize();
        updateWholeDocument();
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

bool QQuickTextEditPrivate::setHAlign(QQuickTextEdit::HAlignment align, bool forceAlign)
{
    Q_Q(QQuickTextEdit);
    if (hAlign == align && !forceAlign)
        return false;

    const bool wasImplicit = hAlignImplicit;
    const auto oldEffectiveHAlign = q->effectiveHAlign();

    hAlignImplicit = !forceAlign;
    if (hAlign != align) {
        hAlign = align;
        emit q->horizontalAlignmentChanged(align);
    }

    if (q->effectiveHAlign() != oldEffectiveHAlign) {
        emit q->effectiveHorizontalAlignmentChanged();
        return true;
    }

    if (forceAlign && wasImplicit) {
        // QTBUG-120052 - when horizontal text alignment is set explicitly,
        // we need notify any other controls that may depend on it, like QQuickPlaceholderText
        emit q->effectiveHorizontalAlignmentChanged();
    }
    return false;
}

Qt::LayoutDirection QQuickTextEditPrivate::textDirection(const QString &text) const
{
    const QChar *character = text.constData();
    while (!character->isNull()) {
        switch (character->direction()) {
        case QChar::DirL:
            return Qt::LeftToRight;
        case QChar::DirR:
        case QChar::DirAL:
        case QChar::DirAN:
            return Qt::RightToLeft;
        default:
            break;
        }
        character++;
    }
    return Qt::LayoutDirectionAuto;
}

bool QQuickTextEditPrivate::determineHorizontalAlignment()
{
    Q_Q(QQuickTextEdit);
    if (!hAlignImplicit || !q->isComponentComplete())
        return false;

    Qt::LayoutDirection direction = contentDirection;
#if QT_CONFIG(im)
    if (direction == Qt::LayoutDirectionAuto) {
        QTextBlock block = control->textCursor().block();
        if (!block.layout())
            return false;
        direction = textDirection(block.layout()->preeditAreaText());
    }
    if (direction == Qt::LayoutDirectionAuto)
        direction = qGuiApp->inputMethod()->inputDirection();
#endif

    const auto implicitHAlign = direction == Qt::RightToLeft ?
            QQuickTextEdit::AlignRight : QQuickTextEdit::AlignLeft;
    return setHAlign(implicitHAlign);
}

void QQuickTextEditPrivate::mirrorChange()
{
    Q_Q(QQuickTextEdit);
    if (q->isComponentComplete()) {
        if (!hAlignImplicit && (hAlign == QQuickTextEdit::AlignRight || hAlign == QQuickTextEdit::AlignLeft)) {
            updateDefaultTextOption();
            q->updateSize();
            q->updateWholeDocument();
            emit q->effectiveHorizontalAlignmentChanged();
        }
    }
}

bool QQuickTextEditPrivate::transformChanged(QQuickItem *transformedItem)
{
    Q_Q(QQuickTextEdit);
    qCDebug(lcVP) << q << "sees that" << transformedItem << "moved in VP" << q->clipRect();

    // If there's a lot of text, and the TextEdit has been scrolled so that the viewport
    // no longer completely covers the rendered region, we need QQuickTextEdit::updatePaintNode()
    // to re-iterate blocks and populate a different range.
    if (flags & QQuickItem::ItemObservesViewport) {
        if (QQuickItem *viewport = q->viewportItem()) {
            QRectF vp = q->mapRectFromItem(viewport, viewport->clipRect());
            if (!(vp.top() > renderedRegion.top() && vp.bottom() < renderedRegion.bottom())) {
                qCDebug(lcVP) << "viewport" << vp << "now goes beyond rendered region" << renderedRegion << "; updating";
                q->updateWholeDocument();
            }
            const bool textCursorVisible = cursorVisible && q->cursorRectangle().intersects(vp);
            if (cursorItem)
                cursorItem->setVisible(textCursorVisible);
            else
                control->setCursorVisible(textCursorVisible);
        }
    }
    return QQuickImplicitSizeItemPrivate::transformChanged(transformedItem);
}

#if QT_CONFIG(im)
Qt::InputMethodHints QQuickTextEditPrivate::effectiveInputMethodHints() const
{
    return inputMethodHints | Qt::ImhMultiLine;
}
#endif

#if QT_CONFIG(accessibility)
void QQuickTextEditPrivate::accessibilityActiveChanged(bool active)
{
    if (!active)
        return;

    Q_Q(QQuickTextEdit);
    if (QQuickAccessibleAttached *accessibleAttached = qobject_cast<QQuickAccessibleAttached *>(
                qmlAttachedPropertiesObject<QQuickAccessibleAttached>(q, true))) {
        accessibleAttached->setRole(effectiveAccessibleRole());
        accessibleAttached->set_readOnly(q->isReadOnly());
    }
}

QAccessible::Role QQuickTextEditPrivate::accessibleRole() const
{
    return QAccessible::EditableText;
}
#endif

void QQuickTextEditPrivate::setTopPadding(qreal value, bool reset)
{
    Q_Q(QQuickTextEdit);
    qreal oldPadding = q->topPadding();
    if (!reset || extra.isAllocated()) {
        extra.value().topPadding = value;
        extra.value().explicitTopPadding = !reset;
    }
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding()))) {
        q->updateSize();
        q->updateWholeDocument();
        emit q->topPaddingChanged();
    }
}

void QQuickTextEditPrivate::setLeftPadding(qreal value, bool reset)
{
    Q_Q(QQuickTextEdit);
    qreal oldPadding = q->leftPadding();
    if (!reset || extra.isAllocated()) {
        extra.value().leftPadding = value;
        extra.value().explicitLeftPadding = !reset;
    }
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding()))) {
        q->updateSize();
        q->updateWholeDocument();
        emit q->leftPaddingChanged();
    }
}

void QQuickTextEditPrivate::setRightPadding(qreal value, bool reset)
{
    Q_Q(QQuickTextEdit);
    qreal oldPadding = q->rightPadding();
    if (!reset || extra.isAllocated()) {
        extra.value().rightPadding = value;
        extra.value().explicitRightPadding = !reset;
    }
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding()))) {
        q->updateSize();
        q->updateWholeDocument();
        emit q->rightPaddingChanged();
    }
}

void QQuickTextEditPrivate::setBottomPadding(qreal value, bool reset)
{
    Q_Q(QQuickTextEdit);
    qreal oldPadding = q->bottomPadding();
    if (!reset || extra.isAllocated()) {
        extra.value().bottomPadding = value;
        extra.value().explicitBottomPadding = !reset;
    }
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding()))) {
        q->updateSize();
        q->updateWholeDocument();
        emit q->bottomPaddingChanged();
    }
}

bool QQuickTextEditPrivate::isImplicitResizeEnabled() const
{
    return !extra.isAllocated() || extra->implicitResize;
}

void QQuickTextEditPrivate::setImplicitResizeEnabled(bool enabled)
{
    if (!enabled)
        extra.value().implicitResize = false;
    else if (extra.isAllocated())
        extra->implicitResize = true;
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
    \qmlproperty enumeration QtQuick::TextEdit::wrapMode

    Set this property to wrap the text to the TextEdit item's width.
    The text will only wrap if an explicit width has been set.

    \value TextEdit.NoWrap
        (default) no wrapping will be performed. If the text contains insufficient newlines,
        \l {Item::}{implicitWidth} will exceed a set width.
    \value TextEdit.WordWrap
        wrapping is done on word boundaries only. If a word is too long,
        \l {Item::}{implicitWidth} will exceed a set width.
    \value TextEdit.WrapAnywhere
        wrapping is done at any point on a line, even if it occurs in the middle of a word.
    \value TextEdit.Wrap
        if possible, wrapping occurs at a word boundary; otherwise it will occur at the appropriate
        point on the line, even in the middle of a word.

    The default is \c TextEdit.NoWrap. If you set a width, consider using \c TextEdit.Wrap.
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
    \qmlproperty int QtQuick::TextEdit::lineCount

    Returns the total number of lines in the TextEdit item.
*/
int QQuickTextEdit::lineCount() const
{
    Q_D(const QQuickTextEdit);
    return d->lineCount;
}

/*!
    \qmlproperty int QtQuick::TextEdit::length

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
    \qmlproperty real QtQuick::TextEdit::contentWidth

    Returns the width of the text, including the width past the width
    which is covered due to insufficient wrapping if \l wrapMode is set.
*/
qreal QQuickTextEdit::contentWidth() const
{
    Q_D(const QQuickTextEdit);
    return d->contentSize.width();
}

/*!
    \qmlproperty real QtQuick::TextEdit::contentHeight

    Returns the height of the text, including the height past the height
    that is covered if the text does not fit within the set height.
*/
qreal QQuickTextEdit::contentHeight() const
{
    Q_D(const QQuickTextEdit);
    return d->contentSize.height();
}

/*!
    \qmlproperty url QtQuick::TextEdit::baseUrl

    This property specifies a base URL which is used to resolve relative URLs
    within the text.

    The default value is the url of the QML file instantiating the TextEdit item.
*/

QUrl QQuickTextEdit::baseUrl() const
{
    Q_D(const QQuickTextEdit);
    if (d->baseUrl.isEmpty()) {
        if (QQmlContext *context = qmlContext(this))
            const_cast<QQuickTextEditPrivate *>(d)->baseUrl = context->baseUrl();
    }
    return d->baseUrl;
}

void QQuickTextEdit::setBaseUrl(const QUrl &url)
{
    Q_D(QQuickTextEdit);
    if (baseUrl() != url) {
        d->baseUrl = url;

        d->document->setBaseUrl(url);
        emit baseUrlChanged();
    }
}

void QQuickTextEdit::resetBaseUrl()
{
    if (QQmlContext *context = qmlContext(this))
        setBaseUrl(context->baseUrl());
    else
        setBaseUrl(QUrl());
}

/*!
    \qmlmethod rectangle QtQuick::TextEdit::positionToRectangle(position)

    Returns the rectangle at the given \a position in the text. The x, y,
    and height properties correspond to the cursor that would describe
    that position.
*/
QRectF QQuickTextEdit::positionToRectangle(int pos) const
{
    Q_D(const QQuickTextEdit);
    QTextCursor c(d->document);
    c.setPosition(pos);
    return d->control->cursorRect(c).translated(d->xoff, d->yoff);

}

/*!
    \qmlmethod int QtQuick::TextEdit::positionAt(int x, int y)

    Returns the text position closest to pixel position (\a x, \a y).

    Position 0 is before the first character, position 1 is after the first character
    but before the second, and so on until position \l {text}.length, which is after all characters.
*/
int QQuickTextEdit::positionAt(qreal x, qreal y) const
{
    Q_D(const QQuickTextEdit);
    x -= d->xoff;
    y -= d->yoff;

    int r = d->document->documentLayout()->hitTest(QPointF(x, y), Qt::FuzzyHit);
#if QT_CONFIG(im)
    QTextCursor cursor = d->control->textCursor();
    if (r > cursor.position()) {
        // The cursor position includes positions within the preedit text, but only positions in the
        // same text block are offset so it is possible to get a position that is either part of the
        // preedit or the next text block.
        QTextLayout *layout = cursor.block().layout();
        const int preeditLength = layout
                ? layout->preeditAreaText().size()
                : 0;
        if (preeditLength > 0
                && d->document->documentLayout()->blockBoundingRect(cursor.block()).contains(x, y)) {
            r = r > cursor.position() + preeditLength
                    ? r - preeditLength
                    : cursor.position();
        }
    }
#endif
    return r;
}

/*!
    \qmlproperty QtQuick::TextSelection QtQuick::TextEdit::cursorSelection
    \since 6.7
    \preliminary

    This property is an object that provides properties of the text that is
    currently selected, if any, alongside the text cursor.

    \sa selectedText, selectionStart, selectionEnd
*/
QQuickTextSelection *QQuickTextEdit::cursorSelection() const
{
    Q_D(const QQuickTextEdit);
    if (!d->cursorSelection)
        d->cursorSelection = new QQuickTextSelection(const_cast<QQuickTextEdit *>(this));
    return d->cursorSelection;
}

/*!
    \qmlmethod QtQuick::TextEdit::moveCursorSelection(int position, SelectionMode mode)

    Moves the cursor to \a position and updates the selection according to the optional \a mode
    parameter. (To only move the cursor, set the \l cursorPosition property.)

    When this method is called it additionally sets either the
    selectionStart or the selectionEnd (whichever was at the previous cursor position)
    to the specified position. This allows you to easily extend and contract the selected
    text range.

    The selection mode specifies whether the selection is updated on a per character or a per word
    basis. If not specified the selection mode will default to \c {TextEdit.SelectCharacters}.

    \value TextEdit.SelectCharacters
        Sets either the selectionStart or selectionEnd (whichever was at the previous cursor position)
        to the specified position.
    \value TextEdit.SelectWords
        Sets the selectionStart and selectionEnd to include all words between the specified position
        and the previous cursor position. Words partially in the range are included.

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
    \qmlproperty bool QtQuick::TextEdit::cursorVisible
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
    if (on && isComponentComplete())
        QQuickTextUtil::createCursor(d);
    if (!on && !d->persistentSelection)
        d->control->setCursorIsFocusIndicator(true);
    d->control->setCursorVisible(on);
    emit cursorVisibleChanged(d->cursorVisible);
}

/*!
    \qmlproperty int QtQuick::TextEdit::cursorPosition
    The position of the cursor in the TextEdit.  The cursor is positioned between
    characters.

    \note The \e characters in this case refer to the string of \l QChar objects,
    therefore 16-bit Unicode characters, and the position is considered an index
    into this string. This does not necessarily correspond to individual graphemes
    in the writing system, as a single grapheme may be represented by multiple
    Unicode characters, such as in the case of surrogate pairs, linguistic
    ligatures or diacritics.
*/
int QQuickTextEdit::cursorPosition() const
{
    Q_D(const QQuickTextEdit);
    return d->control->textCursor().position();
}

void QQuickTextEdit::setCursorPosition(int pos)
{
    Q_D(QQuickTextEdit);
    if (pos < 0 || pos >= d->document->characterCount()) // characterCount includes the terminating null.
        return;
    QTextCursor cursor = d->control->textCursor();
    if (cursor.position() == pos && cursor.anchor() == pos)
        return;
    cursor.setPosition(pos);
    d->control->setTextCursor(cursor);
    d->control->updateCursorRectangle(true);
}

/*!
    \qmlproperty Component QtQuick::TextEdit::cursorDelegate
    The delegate for the cursor in the TextEdit.

    If you set a cursorDelegate for a TextEdit, this delegate will be used for
    drawing the cursor instead of the standard cursor. An instance of the
    delegate will be created and managed by the text edit when a cursor is
    needed, and the x and y properties of delegate instance will be set so as
    to be one pixel before the top left of the current character.

    Note that the root item of the delegate component must be a QQuickItem or
    QQuickItem derived item.
*/
QQmlComponent* QQuickTextEdit::cursorDelegate() const
{
    Q_D(const QQuickTextEdit);
    return d->cursorComponent;
}

void QQuickTextEdit::setCursorDelegate(QQmlComponent* c)
{
    Q_D(QQuickTextEdit);
    QQuickTextUtil::setCursorDelegate(d, c);
}

void QQuickTextEdit::createCursor()
{
    Q_D(QQuickTextEdit);
    d->cursorPending = true;
    QQuickTextUtil::createCursor(d);
}

/*!
    \qmlproperty int QtQuick::TextEdit::selectionStart

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
    \qmlproperty int QtQuick::TextEdit::selectionEnd

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
    \qmlproperty string QtQuick::TextEdit::selectedText

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
#if QT_CONFIG(texthtmlparser)
    return d->richText || d->markdownText
            ? d->control->textCursor().selectedText()
            : d->control->textCursor().selection().toPlainText();
#else
    return d->control->textCursor().selection().toPlainText();
#endif
}

/*!
    \qmlproperty bool QtQuick::TextEdit::activeFocusOnPress

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
    \qmlproperty bool QtQuick::TextEdit::persistentSelection

    Whether the TextEdit should keep the selection visible when it loses active focus to another
    item in the scene. By default this is set to false.
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
   \qmlproperty real QtQuick::TextEdit::textMargin

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
    \qmlproperty enumeration QtQuick::TextEdit::inputMethodHints

    Provides hints to the input method about the expected content of the text edit and how it
    should operate.

    The value is a bit-wise combination of flags or Qt.ImhNone if no hints are set.

    Flags that alter behaviour are:

    \value Qt.ImhHiddenText         Characters should be hidden, as is typically used when entering passwords.
    \value Qt.ImhSensitiveData      Typed text should not be stored by the active input method
                                    in any persistent storage like predictive user dictionary.
    \value Qt.ImhNoAutoUppercase    The input method should not try to automatically switch to
                                    upper case when a sentence ends.
    \value Qt.ImhPreferNumbers      Numbers are preferred (but not required).
    \value Qt.ImhPreferUppercase    Upper case letters are preferred (but not required).
    \value Qt.ImhPreferLowercase    Lower case letters are preferred (but not required).
    \value Qt.ImhNoPredictiveText   Do not use predictive text (i.e. dictionary lookup) while typing.
    \value Qt.ImhDate               The text editor functions as a date field.
    \value Qt.ImhTime               The text editor functions as a time field.

    Flags that restrict input (exclusive flags) are:

    \value Qt.ImhDigitsOnly         Only digits are allowed.
    \value Qt.ImhFormattedNumbersOnly Only number input is allowed. This includes decimal point and minus sign.
    \value Qt.ImhUppercaseOnly      Only upper case letter input is allowed.
    \value Qt.ImhLowercaseOnly      Only lower case letter input is allowed.
    \value Qt.ImhDialableCharactersOnly Only characters suitable for phone dialing are allowed.
    \value Qt.ImhEmailCharactersOnly Only characters suitable for email addresses are allowed.
    \value Qt.ImhUrlCharactersOnly  Only characters suitable for URLs are allowed.

    Masks:

    \value Qt.ImhExclusiveInputMask This mask yields nonzero if any of the exclusive flags are used.
*/

Qt::InputMethodHints QQuickTextEdit::inputMethodHints() const
{
#if !QT_CONFIG(im)
    return Qt::ImhNone;
#else
    Q_D(const QQuickTextEdit);
    return d->inputMethodHints;
#endif // im
}

void QQuickTextEdit::setInputMethodHints(Qt::InputMethodHints hints)
{
#if !QT_CONFIG(im)
    Q_UNUSED(hints);
#else
    Q_D(QQuickTextEdit);

    if (hints == d->inputMethodHints)
        return;

    d->inputMethodHints = hints;
    updateInputMethod(Qt::ImHints);
    emit inputMethodHintsChanged();
#endif // im
}

void QQuickTextEdit::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTextEdit);
    if (!d->inLayout && ((newGeometry.width() != oldGeometry.width())
                         || (newGeometry.height() != oldGeometry.height()))) {
        updateSize();
        updateWholeDocument();
        if (widthValid() || heightValid())
            moveCursorDelegate();
    }
    QQuickImplicitSizeItem::geometryChange(newGeometry, oldGeometry);
}

void QQuickTextEdit::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickTextEdit);
    Q_UNUSED(value);
    switch (change) {
    case ItemDevicePixelRatioHasChanged:
        if (d->containsUnscalableGlyphs) {
            // Native rendering optimizes for a given pixel grid, so its results must not be scaled.
            // Text layout code respects the current device pixel ratio automatically, we only need
            // to rerun layout after the ratio changed.
            updateSize();
            updateWholeDocument();
        }
        break;

    default:
        break;
    }
    QQuickImplicitSizeItem::itemChange(change, value);
}

/*!
    Ensures any delayed caching or data loading the class
    needs to performed is complete.
*/
void QQuickTextEdit::componentComplete()
{
    Q_D(QQuickTextEdit);
    QQuickImplicitSizeItem::componentComplete();

    const QUrl url = baseUrl();
    const QQmlContext *context = qmlContext(this);
    d->document->setBaseUrl(context ? context->resolvedUrl(url) : url);
    if (!d->text.isEmpty()) {
#if QT_CONFIG(texthtmlparser)
        if (d->richText)
            d->control->setHtml(d->text);
        else
#endif
#if QT_CONFIG(textmarkdownreader)
        if (d->markdownText)
            d->control->setMarkdownText(d->text);
        else
#endif
            d->control->setPlainText(d->text);
    }

    if (d->dirty) {
        d->determineHorizontalAlignment();
        d->updateDefaultTextOption();
        updateSize();
        d->dirty = false;
    }
    if (d->cursorComponent && isCursorVisible())
        QQuickTextUtil::createCursor(d);
    polish();

#if QT_CONFIG(accessibility)
    if (QAccessible::isActive())
        d->accessibilityActiveChanged(true);
#endif
}

int QQuickTextEdit::resourcesLoading() const
{
    Q_D(const QQuickTextEdit);
    return d->pixmapsInProgress.size();
}

/*!
    \qmlproperty bool QtQuick::TextEdit::selectByKeyboard
    \since 5.1

    Defaults to true when the editor is editable, and false
    when read-only.

    If true, the user can use the keyboard to select text
    even if the editor is read-only. If false, the user
    cannot use the keyboard to select text even if the
    editor is editable.

    \sa readOnly
*/
bool QQuickTextEdit::selectByKeyboard() const
{
    Q_D(const QQuickTextEdit);
    if (d->selectByKeyboardSet)
        return d->selectByKeyboard;
    return !isReadOnly();
}

void QQuickTextEdit::setSelectByKeyboard(bool on)
{
    Q_D(QQuickTextEdit);
    bool was = selectByKeyboard();
    if (!d->selectByKeyboardSet || was != on) {
        d->selectByKeyboardSet = true;
        d->selectByKeyboard = on;
        if (on)
            d->control->setTextInteractionFlags(d->control->textInteractionFlags() | Qt::TextSelectableByKeyboard);
        else
            d->control->setTextInteractionFlags(d->control->textInteractionFlags() & ~Qt::TextSelectableByKeyboard);
        emit selectByKeyboardChanged(on);
    }
}

/*!
    \qmlproperty bool QtQuick::TextEdit::selectByMouse

    Defaults to \c true since Qt 6.4.

    If \c true, the user can use the mouse to select text in the usual way.

    \note In versions prior to 6.4, the default was \c false; but if you
    enabled this property, you could also select text on a touchscreen by
    dragging your finger across it. This interfered with flicking when TextEdit
    was used inside a Flickable. However, Qt has supported text selection
    handles on mobile platforms, and on embedded platforms using Qt Virtual
    Keyboard, since version 5.7, via QInputMethod. Most users would be
    surprised if finger dragging selected text rather than flicking the parent
    Flickable. Therefore, selectByMouse now really means what it says: if
    \c true, you can select text by dragging \e only with a mouse, whereas
    the platform is expected to provide selection handles on touchscreens.
    If this change does not suit your application, you can set \c selectByMouse
    to \c false, or import an older API version (for example
    \c {import QtQuick 6.3}) to revert to the previous behavior. The option to
    revert behavior by changing the import version will be removed in a later
    version of Qt.
*/
bool QQuickTextEdit::selectByMouse() const
{
    Q_D(const QQuickTextEdit);
    return d->selectByMouse;
}

void QQuickTextEdit::setSelectByMouse(bool on)
{
    Q_D(QQuickTextEdit);
    if (d->selectByMouse == on)
        return;

    d->selectByMouse = on;
    setKeepMouseGrab(on);
    if (on)
        d->control->setTextInteractionFlags(d->control->textInteractionFlags() | Qt::TextSelectableByMouse);
    else
        d->control->setTextInteractionFlags(d->control->textInteractionFlags() & ~Qt::TextSelectableByMouse);

#if QT_CONFIG(cursor)
    d->updateMouseCursorShape();
#endif
    emit selectByMouseChanged(on);
}

/*!
    \qmlproperty enumeration QtQuick::TextEdit::mouseSelectionMode

    Specifies how text should be selected using a mouse.

    \value TextEdit.SelectCharacters    (default) The selection is updated with individual characters.
    \value TextEdit.SelectWords         The selection is updated with whole words.

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
    \qmlproperty bool QtQuick::TextEdit::readOnly

    Whether the user can interact with the TextEdit item. If this
    property is set to true the text cannot be edited by user interaction.

    By default this property is false.
*/
void QQuickTextEdit::setReadOnly(bool r)
{
    Q_D(QQuickTextEdit);
    if (r == isReadOnly())
        return;

#if QT_CONFIG(im)
    setFlag(QQuickItem::ItemAcceptsInputMethod, !r);
#endif
    Qt::TextInteractionFlags flags = Qt::LinksAccessibleByMouse;
    if (d->selectByMouse)
        flags = flags | Qt::TextSelectableByMouse;
    if (d->selectByKeyboardSet && d->selectByKeyboard)
        flags = flags | Qt::TextSelectableByKeyboard;
    else if (!d->selectByKeyboardSet && !r)
        flags = flags | Qt::TextSelectableByKeyboard;
    if (!r)
        flags = flags | Qt::TextEditable;
    d->control->setTextInteractionFlags(flags);
    d->control->moveCursor(QTextCursor::End);

#if QT_CONFIG(im)
    updateInputMethod(Qt::ImEnabled);
#endif
#if QT_CONFIG(cursor)
    d->updateMouseCursorShape();
#endif
    q_canPasteChanged();
    emit readOnlyChanged(r);
    if (!d->selectByKeyboardSet)
        emit selectByKeyboardChanged(!r);
    if (r) {
        setCursorVisible(false);
    } else if (hasActiveFocus()) {
        setCursorVisible(true);
    }

#if QT_CONFIG(accessibility)
    if (QAccessible::isActive()) {
        if (QQuickAccessibleAttached *accessibleAttached = QQuickAccessibleAttached::attachedProperties(this))
            accessibleAttached->set_readOnly(r);
    }
#endif
}

bool QQuickTextEdit::isReadOnly() const
{
    Q_D(const QQuickTextEdit);
    return !(d->control->textInteractionFlags() & Qt::TextEditable);
}

/*!
    \qmlproperty rectangle QtQuick::TextEdit::cursorRectangle

    The rectangle where the standard text cursor is rendered
    within the text edit. Read-only.

    The position and height of a custom cursorDelegate are updated to follow the cursorRectangle
    automatically when it changes.  The width of the delegate is unaffected by changes in the
    cursor rectangle.
*/
QRectF QQuickTextEdit::cursorRectangle() const
{
    Q_D(const QQuickTextEdit);
    return d->control->cursorRect().translated(d->xoff, d->yoff);
}

bool QQuickTextEdit::event(QEvent *event)
{
    Q_D(QQuickTextEdit);
    if (event->type() == QEvent::ShortcutOverride) {
        d->control->processEvent(event, QPointF(-d->xoff, -d->yoff));
        if (event->isAccepted())
            return true;
    }
    return QQuickImplicitSizeItem::event(event);
}

/*!
    \qmlproperty bool QtQuick::TextEdit::overwriteMode
    \since 5.8
    Whether text entered by the user will overwrite existing text.

    As with many text editors, the text editor widget can be configured
    to insert or overwrite existing text with new text entered by the user.

    If this property is \c true, existing text is overwritten, character-for-character
    by new text; otherwise, text is inserted at the cursor position, displacing
    existing text.

    By default, this property is \c false (new text does not overwrite existing text).
*/
bool QQuickTextEdit::overwriteMode() const
{
    Q_D(const QQuickTextEdit);
    return d->control->overwriteMode();
}

void QQuickTextEdit::setOverwriteMode(bool overwrite)
{
    Q_D(QQuickTextEdit);
    d->control->setOverwriteMode(overwrite);
}

/*!
\overload
Handles the given key \a event.
*/
void QQuickTextEdit::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickTextEdit);
    d->control->processEvent(event, QPointF(-d->xoff, -d->yoff));
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
    d->control->processEvent(event, QPointF(-d->xoff, -d->yoff));
    if (!event->isAccepted())
        QQuickImplicitSizeItem::keyReleaseEvent(event);
}

/*!
    \qmlmethod QtQuick::TextEdit::deselect()

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
    \qmlmethod QtQuick::TextEdit::selectAll()

    Causes all text to be selected.
*/
void QQuickTextEdit::selectAll()
{
    Q_D(QQuickTextEdit);
    d->control->selectAll();
}

/*!
    \qmlmethod QtQuick::TextEdit::selectWord()

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
    \qmlmethod QtQuick::TextEdit::select(int start, int end)

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
    if (start < 0 || end < 0 || start >= d->document->characterCount() || end >= d->document->characterCount())
        return;
    QTextCursor cursor = d->control->textCursor();
    cursor.beginEditBlock();
    cursor.setPosition(start, QTextCursor::MoveAnchor);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.endEditBlock();
    d->control->setTextCursor(cursor);

    // QTBUG-11100
    updateSelection();
#if QT_CONFIG(im)
    updateInputMethod();
#endif
}

/*!
    \qmlmethod QtQuick::TextEdit::isRightToLeft(int start, int end)

    Returns true if the natural reading direction of the editor text
    found between positions \a start and \a end is right to left.
*/
bool QQuickTextEdit::isRightToLeft(int start, int end)
{
    if (start > end) {
        qmlWarning(this) << "isRightToLeft(start, end) called with the end property being smaller than the start.";
        return false;
    } else {
        return getText(start, end).isRightToLeft();
    }
}

#if QT_CONFIG(clipboard)
/*!
    \qmlmethod QtQuick::TextEdit::cut()

    Moves the currently selected text to the system clipboard.
*/
void QQuickTextEdit::cut()
{
    Q_D(QQuickTextEdit);
    d->control->cut();
}

/*!
    \qmlmethod QtQuick::TextEdit::copy()

    Copies the currently selected text to the system clipboard.
*/
void QQuickTextEdit::copy()
{
    Q_D(QQuickTextEdit);
    d->control->copy();
}

/*!
    \qmlmethod QtQuick::TextEdit::paste()

    Replaces the currently selected text by the contents of the system clipboard.
*/
void QQuickTextEdit::paste()
{
    Q_D(QQuickTextEdit);
    d->control->paste();
}
#endif // clipboard


/*!
    \qmlmethod QtQuick::TextEdit::undo()

    Undoes the last operation if undo is \l {canUndo}{available}. Deselects any
    current selection, and updates the selection start to the current cursor
    position.
*/

void QQuickTextEdit::undo()
{
    Q_D(QQuickTextEdit);
    d->control->undo();
}

/*!
    \qmlmethod QtQuick::TextEdit::redo()

    Redoes the last operation if redo is \l {canRedo}{available}.
*/

void QQuickTextEdit::redo()
{
    Q_D(QQuickTextEdit);
    d->control->redo();
}

/*!
\overload
Handles the given mouse \a event.
*/
void QQuickTextEdit::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickTextEdit);
    const bool isMouse = QQuickDeliveryAgentPrivate::isEventFromMouseOrTouchpad(event);
    setKeepMouseGrab(d->selectByMouse && isMouse);
    d->control->processEvent(event, QPointF(-d->xoff, -d->yoff));
    if (d->focusOnPress){
        bool hadActiveFocus = hasActiveFocus();
        forceActiveFocus(Qt::MouseFocusReason);
        // re-open input panel on press if already focused
#if QT_CONFIG(im)
        if (hasActiveFocus() && hadActiveFocus && !isReadOnly())
            qGuiApp->inputMethod()->show();
#else
        Q_UNUSED(hadActiveFocus);
#endif
    }
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
    d->control->processEvent(event, QPointF(-d->xoff, -d->yoff));

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
    d->control->processEvent(event, QPointF(-d->xoff, -d->yoff));
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
    d->control->processEvent(event, QPointF(-d->xoff, -d->yoff));
    if (!event->isAccepted())
        QQuickImplicitSizeItem::mouseMoveEvent(event);
}

#if QT_CONFIG(im)
/*!
\overload
Handles the given input method \a event.
*/
void QQuickTextEdit::inputMethodEvent(QInputMethodEvent *event)
{
    Q_D(QQuickTextEdit);
    const bool wasComposing = isInputMethodComposing();
    d->control->processEvent(event, QPointF(-d->xoff, -d->yoff));
    setCursorVisible(d->control->cursorVisible());
    if (wasComposing != isInputMethodComposing())
        emit inputMethodComposingChanged();
}

/*!
\overload
Returns the value of the given \a property and \a argument.
*/
QVariant QQuickTextEdit::inputMethodQuery(Qt::InputMethodQuery property, QVariant argument) const
{
    Q_D(const QQuickTextEdit);

    QVariant v;
    switch (property) {
    case Qt::ImEnabled:
        v = (bool)(flags() & ItemAcceptsInputMethod);
        break;
    case Qt::ImHints:
        v = (int)d->effectiveInputMethodHints();
        break;
    case Qt::ImInputItemClipRectangle:
        v = QQuickItem::inputMethodQuery(property);
        break;
    case Qt::ImReadOnly:
        v = isReadOnly();
        break;
    default:
        if (property == Qt::ImCursorPosition && !argument.isNull())
            argument = QVariant(argument.toPointF() - QPointF(d->xoff, d->yoff));
        v = d->control->inputMethodQuery(property, argument);
        if (property == Qt::ImCursorRectangle || property == Qt::ImAnchorRectangle)
            v = QVariant(v.toRectF().translated(d->xoff, d->yoff));
        break;
    }
    return v;
}

/*!
\overload
Returns the value of the given \a property.
*/
QVariant QQuickTextEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    return inputMethodQuery(property, QVariant());
}
#endif // im

void QQuickTextEdit::triggerPreprocess()
{
    Q_D(QQuickTextEdit);
    if (d->updateType == QQuickTextEditPrivate::UpdateNone)
        d->updateType = QQuickTextEditPrivate::UpdateOnlyPreprocess;
    polish();
    update();
}

/*! \internal
    QTextDocument::loadResource() calls this to load inline images etc.
    But if it's a local file, don't do it: let QTextDocument::loadResource()
    load it in the default way. QQuickPixmap is for QtQuick-specific uses.
*/
QVariant QQuickTextEdit::loadResource(int type, const QUrl &source)
{
    Q_D(QQuickTextEdit);
    const QUrl url = d->document->baseUrl().resolved(source);
    if (url.isLocalFile()) {
        // qmlWarning if the file doesn't exist (because QTextDocument::loadResource() can't do that)
        QFileInfo fi(QQmlFile::urlToLocalFileOrQrc(url));
        if (!fi.exists())
            qmlWarning(this) << "Cannot open: " << url.toString();
        // let QTextDocument::loadResource() handle local file loading
        return {};
    }

    // If the image is in resources, load it here, because QTextDocument::loadResource() doesn't do that
    if (!url.scheme().compare("qrc"_L1, Qt::CaseInsensitive)) {
        // qmlWarning if the file doesn't exist
        QFile f(QQmlFile::urlToLocalFileOrQrc(url));
        if (f.open(QFile::ReadOnly)) {
            QByteArray buf = f.readAll();
            f.close();
            QImage image;
            image.loadFromData(buf);
            if (!image.isNull())
                return image;
        }
        // if we get here, loading failed
        qmlWarning(this) << "Cannot read resource: " << f.fileName();
        return {};
    }

    // see if we already started a load job
    auto existingJobIter = std::find_if(
            d->pixmapsInProgress.cbegin(), d->pixmapsInProgress.cend(),
            [&url](const auto *job) { return job->url() == url; } );
    if (existingJobIter != d->pixmapsInProgress.cend()) {
        const QQuickPixmap *job = *existingJobIter;
        if (job->isError()) {
            qmlWarning(this) << job->error();
            d->pixmapsInProgress.erase(existingJobIter);
            delete job;
            return QImage();
        } else {
            qCDebug(lcTextEdit) << "already downloading" << url;
            // existing job: return a null variant if it's not done yet
            return job->isReady() ? job->image() : QVariant();
        }
    }

    // not found: start a new load job
    qCDebug(lcTextEdit) << "loading" << source << "resolved" << url
                        << "type" << static_cast<QTextDocument::ResourceType>(type);
    QQmlContext *context = qmlContext(this);
    Q_ASSERT(context);
    // don't cache it in QQuickPixmapCache, because it's cached in QTextDocumentPrivate::cachedResources
    QQuickPixmap *p = new QQuickPixmap(context->engine(), url, QQuickPixmap::Options{});
    p->connectFinished(this, SLOT(resourceRequestFinished()));
    d->pixmapsInProgress.append(p);
    // the new job is probably not done; return a null variant if the caller should poll again
    return p->isReady() ? p->image() : QVariant();
}

/*! \internal
    Handle completion of a download that QQuickTextEdit::loadResource() started.
*/
void QQuickTextEdit::resourceRequestFinished()
{
    Q_D(QQuickTextEdit);
    for (auto it = d->pixmapsInProgress.cbegin(); it != d->pixmapsInProgress.cend(); ++it) {
        auto *job = *it;
        if (job->isError()) {
            // get QTextDocument::loadResource() to call QQuickTextEdit::loadResource() again, to return the placeholder
            qCDebug(lcTextEdit) << "failed to load (error)" << job->url();
            d->document->resource(QTextDocument::ImageResource, job->url());
            // that will call QQuickTextEdit::loadResource() which will delete the job;
            // so leave it in pixmapsInProgress for now, and stop this loop
            break;
        } else if (job->isReady()) {
            // get QTextDocument::loadResource() to call QQuickTextEdit::loadResource() again, and cache the result
            auto res = d->document->resource(QTextDocument::ImageResource, job->url());
            // If QTextDocument::resource() returned a valid variant, it's been cached too. Either way, the job is done.
            qCDebug(lcTextEdit) << (res.isValid() ? "done downloading" : "failed to load") << job->url() << job->rect();
            d->pixmapsInProgress.erase(it);
            delete job;
            break;
        }
    }
    if (d->pixmapsInProgress.isEmpty()) {
        invalidate();
        updateSize();
        q_invalidate();
    }
}

typedef QQuickTextEditPrivate::Node TextNode;
using TextNodeIterator = QQuickTextEditPrivate::TextNodeIterator;

static inline bool operator<(const TextNode &n1, const TextNode &n2)
{
    return n1.startPos() < n2.startPos();
}

static inline void updateNodeTransform(QSGInternalTextNode *node, const QPointF &topLeft)
{
    QMatrix4x4 transformMatrix;
    transformMatrix.translate(topLeft.x(), topLeft.y());
    node->setMatrix(transformMatrix);
}

/*!
 * \internal
 *
 * Invalidates font caches owned by the text objects owned by the element
 * to work around the fact that text objects cannot be used from multiple threads.
 */
void QQuickTextEdit::invalidateFontCaches()
{
    Q_D(QQuickTextEdit);
    if (d->document == nullptr)
        return;

    QTextBlock block;
    for (block = d->document->firstBlock(); block.isValid(); block = block.next()) {
        if (block.layout() != nullptr && block.layout()->engine() != nullptr)
            block.layout()->engine()->resetFontEngineCache();
    }
}

QTextDocument *QQuickTextEdit::document() const
{
    Q_D(const QQuickTextEdit);
    return d->document;
}

void QQuickTextEdit::setDocument(QTextDocument *doc)
{
    Q_D(QQuickTextEdit);
    // do not delete the owned document till after control has been updated
    std::unique_ptr<QTextDocument> cleanup(d->ownsDocument ? d->document : nullptr);
    d->document = doc;
    d->ownsDocument = false;
    d->control->setDocument(doc);
    q_textChanged();
}

inline void resetEngine(QQuickTextNodeEngine *engine, const QColor& textColor, const QColor& selectedTextColor, const QColor& selectionColor, qreal dpr)
{
    *engine = QQuickTextNodeEngine();
    engine->setTextColor(textColor);
    engine->setSelectedTextColor(selectedTextColor);
    engine->setSelectionColor(selectionColor);
    engine->setDevicePixelRatio(dpr);
}

QSGNode *QQuickTextEdit::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData);
    Q_D(QQuickTextEdit);

    if (d->updateType != QQuickTextEditPrivate::UpdatePaintNode
          && d->updateType != QQuickTextEditPrivate::UpdateAll
          && oldNode != nullptr) {
        // Update done in preprocess() in the nodes
        d->updateType = QQuickTextEditPrivate::UpdateNone;
        return oldNode;
    }

    d->containsUnscalableGlyphs = false;
    if (!oldNode || d->updateType == QQuickTextEditPrivate::UpdateAll) {
        delete oldNode;
        oldNode = nullptr;

        // If we had any QSGInternalTextNode node references, they were deleted along with the root node
        // But here we must delete the Node structures in textNodeMap
        d->textNodeMap.clear();
    }

    d->updateType = QQuickTextEditPrivate::UpdateNone;

    RootNode *rootNode = static_cast<RootNode *>(oldNode);
    TextNodeIterator nodeIterator = d->textNodeMap.begin();
    std::optional<int> firstPosAcrossAllNodes;
    if (nodeIterator != d->textNodeMap.end())
        firstPosAcrossAllNodes = nodeIterator->startPos();

    while (nodeIterator != d->textNodeMap.end() && !nodeIterator->dirty())
        ++nodeIterator;

    const auto dpr = window()->effectiveDevicePixelRatio();
    QQuickTextNodeEngine engine;
    engine.setDevicePixelRatio(dpr);
    QQuickTextNodeEngine frameDecorationsEngine;
    frameDecorationsEngine.setDevicePixelRatio(dpr);

    if (!oldNode || nodeIterator < d->textNodeMap.end() || d->textNodeMap.isEmpty()) {

        if (!oldNode)
            rootNode = new RootNode;

        int firstDirtyPos = 0;
        if (nodeIterator != d->textNodeMap.end()) {
            firstDirtyPos = nodeIterator->startPos();
            // ### this could be optimized if the first and last dirty nodes are not connected
            // as the intermediate text nodes would usually only need to be transformed differently.
            QSGInternalTextNode *firstCleanNode = nullptr;
            auto it = d->textNodeMap.constEnd();
            while (it != nodeIterator) {
                --it;
                if (it->dirty())
                    break;
                firstCleanNode = it->textNode();
            }
            do {
                rootNode->removeChildNode(nodeIterator->textNode());
                delete nodeIterator->textNode();
                nodeIterator = d->textNodeMap.erase(nodeIterator);
            } while (nodeIterator != d->textNodeMap.constEnd() && nodeIterator->textNode() != firstCleanNode);
        }

        // If there's a lot of text, insert only the range of blocks that can possibly be visible within the viewport.
        QRectF viewport;
        if (flags().testFlag(QQuickItem::ItemObservesViewport)) {
            viewport = clipRect();
            qCDebug(lcVP) << "text viewport" << viewport;
        }

        // FIXME: the text decorations could probably be handled separately (only updated for affected textFrames)
        rootNode->resetFrameDecorations(d->createTextNode());
        resetEngine(&frameDecorationsEngine, d->color, d->selectedTextColor, d->selectionColor, dpr);

        QSGInternalTextNode *node = nullptr;

        int currentNodeSize = 0;
        int nodeStart = firstDirtyPos;
        QPointF basePosition(d->xoff, d->yoff);
        QMatrix4x4 basePositionMatrix;
        basePositionMatrix.translate(basePosition.x(), basePosition.y());
        rootNode->setMatrix(basePositionMatrix);

        QPointF nodeOffset;
        const TextNode firstCleanNode = (nodeIterator != d->textNodeMap.end()) ? *nodeIterator
                                                                               : TextNode();

        QList<QTextFrame *> frames;
        frames.append(d->document->rootFrame());


        d->firstBlockInViewport = -1;
        d->firstBlockPastViewport = -1;
        int frameCount = -1;
        while (!frames.isEmpty()) {
            QTextFrame *textFrame = frames.takeFirst();
            ++frameCount;
            if (frameCount > 0)
                firstDirtyPos = 0;
            qCDebug(lcVP) << "frame" << frameCount << textFrame
                          << "from" << positionToRectangle(textFrame->firstPosition()).topLeft()
                          << "to" << positionToRectangle(textFrame->lastPosition()).bottomRight();
            frames.append(textFrame->childFrames());
            frameDecorationsEngine.addFrameDecorations(d->document, textFrame);
            resetEngine(&engine, d->color, d->selectedTextColor, d->selectionColor, dpr);

            if (textFrame->firstPosition() > textFrame->lastPosition()
                    && textFrame->frameFormat().position() != QTextFrameFormat::InFlow) {
                node = d->createTextNode();
                updateNodeTransform(node, d->document->documentLayout()->frameBoundingRect(textFrame).topLeft());
                const int pos = textFrame->firstPosition() - 1;
                auto *a = static_cast<QtPrivate::ProtectedLayoutAccessor *>(d->document->documentLayout());
                QTextCharFormat format = a->formatAccessor(pos);
                QTextBlock block = textFrame->firstCursorPosition().block();
                nodeOffset = d->document->documentLayout()->blockBoundingRect(block).topLeft();
                bool inView = true;
                if (!viewport.isNull() && block.layout()) {
                    QRectF coveredRegion = block.layout()->boundingRect().adjusted(nodeOffset.x(), nodeOffset.y(), nodeOffset.x(), nodeOffset.y());
                    inView = coveredRegion.bottom() >= viewport.top() && coveredRegion.top() <= viewport.bottom();
                    qCDebug(lcVP) << "non-flow frame" << coveredRegion << "in viewport?" << inView;
                }
                if (inView) {
                    engine.setCurrentLine(block.layout()->lineForTextPosition(pos - block.position()));
                    engine.addTextObject(block, QPointF(0, 0), format, QQuickTextNodeEngine::Unselected, d->document,
                                                  pos, textFrame->frameFormat().position());
                }
                nodeStart = pos;
            } else {
                // Having nodes spanning across frame boundaries will break the current bookkeeping mechanism. We need to prevent that.
                QList<int> frameBoundaries;
                frameBoundaries.reserve(frames.size());
                for (QTextFrame *frame : std::as_const(frames))
                    frameBoundaries.append(frame->firstPosition());
                std::sort(frameBoundaries.begin(), frameBoundaries.end());

                QTextFrame::iterator it = textFrame->begin();
                while (!it.atEnd()) {
                    QTextBlock block = it.currentBlock();
                    if (block.position() < firstDirtyPos) {
                        ++it;
                        continue;
                    }

                    if (!engine.hasContents())
                        nodeOffset = d->document->documentLayout()->blockBoundingRect(block).topLeft();

                    bool inView = true;
                    if (!viewport.isNull()) {
                        QRectF coveredRegion;
                        if (block.layout()) {
                            coveredRegion = block.layout()->boundingRect().adjusted(nodeOffset.x(), nodeOffset.y(), nodeOffset.x(), nodeOffset.y());
                            inView = coveredRegion.bottom() > viewport.top();
                        }
                        const bool potentiallyScrollingBackwards = firstPosAcrossAllNodes && *firstPosAcrossAllNodes == firstDirtyPos;
                        if (d->firstBlockInViewport < 0 && inView && potentiallyScrollingBackwards) {
                            // During backward scrolling, we need to iterate backwards from textNodeMap.begin() to fill the top of the viewport.
                            if (coveredRegion.top() > viewport.top() + 1) {
                                qCDebug(lcVP) << "checking backwards from block" << block.blockNumber() << "@" << nodeOffset.y() << coveredRegion;
                                while (it != textFrame->begin() && it.currentBlock().layout() &&
                                       it.currentBlock().layout()->boundingRect().top() + nodeOffset.y() > viewport.top()) {
                                    nodeOffset = d->document->documentLayout()->blockBoundingRect(it.currentBlock()).topLeft();
                                    --it;
                                }
                                if (!it.currentBlock().layout())
                                    ++it;
                                if (Q_LIKELY(it.currentBlock().layout())) {
                                    block = it.currentBlock();
                                    coveredRegion = block.layout()->boundingRect().adjusted(nodeOffset.x(), nodeOffset.y(), nodeOffset.x(), nodeOffset.y());
                                    firstDirtyPos = it.currentBlock().position();
                                } else {
                                    qCWarning(lcVP) << "failed to find a text block with layout during back-scrolling";
                                }
                            }
                            qCDebug(lcVP) << "first block in viewport" << block.blockNumber() << "@" << nodeOffset.y() << coveredRegion;
                            if (block.layout())
                                d->renderedRegion = coveredRegion;
                        } else {
                            if (nodeOffset.y() > viewport.bottom()) {
                                inView = false;
                                if (d->firstBlockInViewport >= 0 && d->firstBlockPastViewport < 0) {
                                    qCDebug(lcVP) << "first block past viewport" << viewport << block.blockNumber()
                                                  << "@" << nodeOffset.y() << "total region rendered" << d->renderedRegion;
                                    d->firstBlockPastViewport = block.blockNumber();
                                }
                                break; // skip rest of blocks in this frame
                            }
                            if (inView && !block.text().isEmpty() && coveredRegion.isValid()) {
                                d->renderedRegion = d->renderedRegion.united(coveredRegion);
                                // In case we're going to visit more (nested) frames after this, ensure that we
                                // don't omit any blocks that fit within the region that we claim as fully rendered.
                                if (!frames.isEmpty())
                                    viewport = viewport.united(d->renderedRegion);
                            }
                        }
                        if (inView && d->firstBlockInViewport < 0)
                            d->firstBlockInViewport = block.blockNumber();
                    }

                    bool createdNodeInView = false;
                    if (inView) {
                        if (!engine.hasContents()) {
                            if (node) {
                                d->containsUnscalableGlyphs = d->containsUnscalableGlyphs
                                                              || node->containsUnscalableGlyphs();
                                if (!node->parent())
                                    d->addCurrentTextNodeToRoot(&engine, rootNode, node, nodeIterator, nodeStart);
                            }
                            node = d->createTextNode();
                            createdNodeInView = true;
                            updateNodeTransform(node, nodeOffset);
                            nodeStart = block.position();
                        }
                        engine.addTextBlock(d->document, block, -nodeOffset, d->color, QColor(), selectionStart(), selectionEnd() - 1);
                        currentNodeSize += block.length();
                    }

                    if ((it.atEnd()) || block.next().position() >= firstCleanNode.startPos())
                        break; // last node that needed replacing or last block of the frame
                    QList<int>::const_iterator lowerBound = std::lower_bound(frameBoundaries.constBegin(), frameBoundaries.constEnd(), block.next().position());
                    if (node && (currentNodeSize > nodeBreakingSize || lowerBound == frameBoundaries.constEnd() || *lowerBound > nodeStart)) {
                        currentNodeSize = 0;
                        d->containsUnscalableGlyphs = d->containsUnscalableGlyphs
                                                      || node->containsUnscalableGlyphs();
                        if (!node->parent())
                            d->addCurrentTextNodeToRoot(&engine, rootNode, node, nodeIterator, nodeStart);
                        if (!createdNodeInView)
                            node = d->createTextNode();
                        resetEngine(&engine, d->color, d->selectedTextColor, d->selectionColor, dpr);
                        nodeStart = block.next().position();
                    }
                    ++it;
                } // loop over blocks in frame
            }
            if (Q_LIKELY(node)) {
                d->containsUnscalableGlyphs = d->containsUnscalableGlyphs
                                              || node->containsUnscalableGlyphs();
                if (Q_LIKELY(!node->parent()))
                    d->addCurrentTextNodeToRoot(&engine, rootNode, node, nodeIterator, nodeStart);
            }
        }
        frameDecorationsEngine.addToSceneGraph(rootNode->frameDecorationsNode, QQuickText::Normal, QColor());
        // Now prepend the frame decorations since we want them rendered first, with the text nodes and cursor in front.
        rootNode->prependChildNode(rootNode->frameDecorationsNode);

        Q_ASSERT(nodeIterator == d->textNodeMap.end()
                 || (nodeIterator->textNode() == firstCleanNode.textNode()
                     && nodeIterator->startPos() == firstCleanNode.startPos()));
        // Update the position of the subsequent text blocks.
        if (firstCleanNode.textNode() != nullptr) {
            QPointF oldOffset = firstCleanNode.textNode()->matrix().map(QPointF(0,0));
            QPointF currentOffset = d->document->documentLayout()->blockBoundingRect(
                        d->document->findBlock(firstCleanNode.startPos())).topLeft();
            QPointF delta = currentOffset - oldOffset;
            while (nodeIterator != d->textNodeMap.end()) {
                QMatrix4x4 transformMatrix = nodeIterator->textNode()->matrix();
                transformMatrix.translate(delta.x(), delta.y());
                nodeIterator->textNode()->setMatrix(transformMatrix);
                ++nodeIterator;
            }

        }

        // Since we iterate over blocks from different text frames that are potentially not sorted
        // we need to ensure that our list of nodes is sorted again:
        std::sort(d->textNodeMap.begin(), d->textNodeMap.end());
    }

    if (d->cursorComponent == nullptr) {
        QSGInternalRectangleNode* cursor = nullptr;
        if (!isReadOnly() && d->cursorVisible && d->control->cursorOn() && d->control->cursorVisible())
            cursor = d->sceneGraphContext()->createInternalRectangleNode(d->control->cursorRect(), d->color);
        rootNode->resetCursorNode(cursor);
    }

    invalidateFontCaches();

    return rootNode;
}

void QQuickTextEdit::updatePolish()
{
    invalidateFontCaches();
}

/*!
    \qmlproperty bool QtQuick::TextEdit::canPaste

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
    \qmlproperty bool QtQuick::TextEdit::canUndo

    Returns true if the TextEdit is writable and there are previous operations
    that can be undone.
*/

bool QQuickTextEdit::canUndo() const
{
    Q_D(const QQuickTextEdit);
    return d->document->isUndoAvailable();
}

/*!
    \qmlproperty bool QtQuick::TextEdit::canRedo

    Returns true if the TextEdit is writable and there are \l {undo}{undone}
    operations that can be redone.
*/

bool QQuickTextEdit::canRedo() const
{
    Q_D(const QQuickTextEdit);
    return d->document->isRedoAvailable();
}

/*!
    \qmlproperty bool QtQuick::TextEdit::inputMethodComposing


    This property holds whether the TextEdit has partial text input from an
    input method.

    While it is composing an input method may rely on mouse or key events from
    the TextEdit to edit or commit the partial text.  This property can be used
    to determine when to disable events handlers that may interfere with the
    correct operation of an input method.
*/
bool QQuickTextEdit::isInputMethodComposing() const
{
#if !QT_CONFIG(im)
    return false;
#else
    Q_D(const QQuickTextEdit);
    return d->control->hasImState();
#endif // im
}

QQuickTextEditPrivate::ExtraData::ExtraData()
    : explicitTopPadding(false)
    , explicitLeftPadding(false)
    , explicitRightPadding(false)
    , explicitBottomPadding(false)
    , implicitResize(true)
{
}

void QQuickTextEditPrivate::init()
{
    Q_Q(QQuickTextEdit);

#if QT_CONFIG(clipboard)
    if (QGuiApplication::clipboard()->supportsSelection())
        q->setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton);
    else
#endif
        q->setAcceptedMouseButtons(Qt::LeftButton);

#if QT_CONFIG(im)
    q->setFlag(QQuickItem::ItemAcceptsInputMethod);
#endif
    q->setFlag(QQuickItem::ItemHasContents);

    q->setAcceptHoverEvents(true);

    document = new QTextDocument(q);
    ownsDocument = true;
    auto *imageHandler = new QQuickTextImageHandler(document);
    document->documentLayout()->registerHandler(QTextFormat::ImageObject, imageHandler);

    control = new QQuickTextControl(document, q);
    control->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::TextEditable);
    control->setAcceptRichText(false);
    control->setCursorIsFocusIndicator(true);
    q->setKeepMouseGrab(true);

    qmlobject_connect(control, QQuickTextControl, SIGNAL(updateCursorRequest()), q, QQuickTextEdit, SLOT(updateCursor()));
    qmlobject_connect(control, QQuickTextControl, SIGNAL(selectionChanged()), q, QQuickTextEdit, SIGNAL(selectedTextChanged()));
    qmlobject_connect(control, QQuickTextControl, SIGNAL(selectionChanged()), q, QQuickTextEdit, SLOT(updateSelection()));
    qmlobject_connect(control, QQuickTextControl, SIGNAL(cursorPositionChanged()), q, QQuickTextEdit, SLOT(updateSelection()));
    qmlobject_connect(control, QQuickTextControl, SIGNAL(cursorPositionChanged()), q, QQuickTextEdit, SIGNAL(cursorPositionChanged()));
    qmlobject_connect(control, QQuickTextControl, SIGNAL(cursorRectangleChanged()), q, QQuickTextEdit, SLOT(moveCursorDelegate()));
    qmlobject_connect(control, QQuickTextControl, SIGNAL(linkActivated(QString)), q, QQuickTextEdit, SIGNAL(linkActivated(QString)));
    qmlobject_connect(control, QQuickTextControl, SIGNAL(overwriteModeChanged(bool)), q, QQuickTextEdit, SIGNAL(overwriteModeChanged(bool)));
    qmlobject_connect(control, QQuickTextControl, SIGNAL(textChanged()), q, QQuickTextEdit, SLOT(q_textChanged()));
    qmlobject_connect(control, QQuickTextControl, SIGNAL(preeditTextChanged()), q, QQuickTextEdit, SIGNAL(preeditTextChanged()));
#if QT_CONFIG(clipboard)
    qmlobject_connect(QGuiApplication::clipboard(), QClipboard, SIGNAL(dataChanged()), q, QQuickTextEdit, SLOT(q_canPasteChanged()));
#endif
    qmlobject_connect(document, QTextDocument, SIGNAL(undoAvailable(bool)), q, QQuickTextEdit, SIGNAL(canUndoChanged()));
    qmlobject_connect(document, QTextDocument, SIGNAL(redoAvailable(bool)), q, QQuickTextEdit, SIGNAL(canRedoChanged()));
    QObject::connect(document, &QTextDocument::contentsChange, q, &QQuickTextEdit::q_contentsChange);
    QObject::connect(document->documentLayout(), &QAbstractTextDocumentLayout::updateBlock, q, &QQuickTextEdit::invalidateBlock);
    QObject::connect(control, &QQuickTextControl::linkHovered, q, &QQuickTextEdit::q_linkHovered);
    QObject::connect(control, &QQuickTextControl::markerHovered, q, &QQuickTextEdit::q_markerHovered);

    document->setPageSize(QSizeF(0, 0));
    document->setDefaultFont(font);
    document->setDocumentMargin(textMargin);
    document->setUndoRedoEnabled(false); // flush undo buffer.
    document->setUndoRedoEnabled(true);
    updateDefaultTextOption();
    document->setModified(false); // we merely changed some defaults: no edits worth saving yet
    q->updateSize();
#if QT_CONFIG(cursor)
    updateMouseCursorShape();
#endif
    setSizePolicy(QLayoutPolicy::Expanding, QLayoutPolicy::Expanding);
}

void QQuickTextEditPrivate::resetInputMethod()
{
    Q_Q(QQuickTextEdit);
    if (!q->isReadOnly() && q->hasActiveFocus() && qGuiApp)
        QGuiApplication::inputMethod()->reset();
}

void QQuickTextEdit::q_textChanged()
{
    Q_D(QQuickTextEdit);
    d->textCached = false;
    for (QTextBlock it = d->document->begin(); it != d->document->end(); it = it.next()) {
        d->contentDirection = d->textDirection(it.text());
        if (d->contentDirection != Qt::LayoutDirectionAuto)
            break;
    }
    d->determineHorizontalAlignment();
    d->updateDefaultTextOption();
    updateSize();

    markDirtyNodesForRange(0, d->document->characterCount(), 0);
    if (isComponentComplete()) {
        polish();
        d->updateType = QQuickTextEditPrivate::UpdatePaintNode;
        update();
    }

    emit textChanged();
    if (d->control->isBeingEdited())
        emit textEdited();
}

void QQuickTextEdit::markDirtyNodesForRange(int start, int end, int charDelta)
{
    Q_D(QQuickTextEdit);
    if (start == end)
        return;

    TextNode dummyNode(start);

    const TextNodeIterator textNodeMapBegin = d->textNodeMap.begin();
    const TextNodeIterator textNodeMapEnd = d->textNodeMap.end();

    TextNodeIterator it = std::lower_bound(textNodeMapBegin, textNodeMapEnd, dummyNode);
    // qLowerBound gives us the first node past the start of the affected portion, rewind to the first node
    // that starts at the last position before the edit position. (there might be several because of images)
    if (it != textNodeMapBegin) {
        --it;
        TextNode otherDummy(it->startPos());
        it = std::lower_bound(textNodeMapBegin, textNodeMapEnd, otherDummy);
    }

    // mark the affected nodes as dirty
    while (it != textNodeMapEnd) {
        if (it->startPos() <= end)
            it->setDirty();
        else if (charDelta)
            it->moveStartPos(charDelta);
        else
            return;
        ++it;
    }
}

void QQuickTextEdit::q_contentsChange(int pos, int charsRemoved, int charsAdded)
{
    Q_D(QQuickTextEdit);

    const int editRange = pos + qMax(charsAdded, charsRemoved);
    const int delta = charsAdded - charsRemoved;

    markDirtyNodesForRange(pos, editRange, delta);

    if (isComponentComplete()) {
        polish();
        d->updateType = QQuickTextEditPrivate::UpdatePaintNode;
        update();
    }
}

void QQuickTextEdit::moveCursorDelegate()
{
    Q_D(QQuickTextEdit);
#if QT_CONFIG(im)
    updateInputMethod();
#endif
    emit cursorRectangleChanged();
    if (!d->cursorItem)
        return;
    QRectF cursorRect = cursorRectangle();
    d->cursorItem->setX(cursorRect.x());
    d->cursorItem->setY(cursorRect.y());
    d->cursorItem->setHeight(cursorRect.height());
}

void QQuickTextEdit::updateSelection()
{
    Q_D(QQuickTextEdit);

    // No need for node updates when we go from an empty selection to another empty selection
    if (d->control->textCursor().hasSelection() || d->hadSelection) {
        markDirtyNodesForRange(qMin(d->lastSelectionStart, d->control->textCursor().selectionStart()), qMax(d->control->textCursor().selectionEnd(), d->lastSelectionEnd), 0);
        if (isComponentComplete()) {
            polish();
            d->updateType = QQuickTextEditPrivate::UpdatePaintNode;
            update();
        }
    }

    d->hadSelection = d->control->textCursor().hasSelection();

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
    QRectF r(
            QQuickTextUtil::alignedX(d->contentSize.width(), width(), effectiveHAlign()),
            d->yoff,
            d->contentSize.width(),
            d->contentSize.height());

    int cursorWidth = 1;
    if (d->cursorItem)
        cursorWidth = 0;
    else if (!d->document->isEmpty())
        cursorWidth += 3;// ### Need a better way of accounting for space between char and cursor

    // Could include font max left/right bearings to either side of rectangle.
    r.setRight(r.right() + cursorWidth);

    return r;
}

QRectF QQuickTextEdit::clipRect() const
{
    Q_D(const QQuickTextEdit);
    QRectF r = QQuickImplicitSizeItem::clipRect();
    int cursorWidth = 1;
    if (d->cursorItem)
        cursorWidth = d->cursorItem->width();
    if (!d->document->isEmpty())
        cursorWidth += 3;// ### Need a better way of accounting for space between char and cursor

    // Could include font max left/right bearings to either side of rectangle.

    r.setRight(r.right() + cursorWidth);
    return r;
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
    if (!isComponentComplete()) {
        d->dirty = true;
        return;
    }

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
            const qreal naturalWidth = d->document->idealWidth();
            const bool wasInLayout = d->inLayout;
            d->inLayout = true;
            if (d->isImplicitResizeEnabled())
                setImplicitWidth(naturalWidth + leftPadding() + rightPadding());
            d->inLayout = wasInLayout;
            if (d->inLayout)    // probably the result of a binding loop, but by letting it
                return;         // get this far we'll get a warning to that effect.
        }
        const qreal newTextWidth = width() - leftPadding() - rightPadding();
        if (d->document->textWidth() != newTextWidth)
            d->document->setTextWidth(newTextWidth);
    } else if (d->wrapMode == NoWrap) {
        // normally, if explicit width is not set, we should call setTextWidth(-1) here,
        // as we don't need to fit the text to any fixed width. But because of some bug
        // in QTextDocument it also breaks RTL text alignment, so we use "idealWidth" instead.
        const qreal newTextWidth = d->document->idealWidth();
        if (d->document->textWidth() != newTextWidth)
            d->document->setTextWidth(newTextWidth);
    } else {
        d->document->setTextWidth(-1);
    }

    QFontMetricsF fm(d->font);
    const qreal newHeight = d->document->isEmpty() ? qCeil(fm.height()) : d->document->size().height();
    const qreal newWidth = d->document->idealWidth();

    if (d->isImplicitResizeEnabled()) {
        // ### Setting the implicitWidth triggers another updateSize(), and unless there are bindings nothing has changed.
        if (!widthValid())
            setImplicitSize(newWidth + leftPadding() + rightPadding(), newHeight + topPadding() + bottomPadding());
        else
            setImplicitHeight(newHeight + topPadding() + bottomPadding());
    }

    d->xoff = leftPadding() + qMax(qreal(0), QQuickTextUtil::alignedX(d->document->size().width(), width() - leftPadding() - rightPadding(), effectiveHAlign()));
    d->yoff = topPadding() + QQuickTextUtil::alignedY(d->document->size().height(), height() - topPadding() - bottomPadding(), d->vAlign);

    qreal baseline = fm.ascent();
    QTextBlock firstBlock = d->document->firstBlock();
    if (firstBlock.isValid() && firstBlock.layout() != nullptr && firstBlock.lineCount() > 0) {
        QTextLine firstLine = firstBlock.layout()->lineAt(0);
        if (firstLine.isValid())
            baseline = firstLine.ascent();
    }

    setBaselineOffset(baseline + d->yoff + d->textMargin);

    QSizeF size(newWidth, newHeight);
    if (d->contentSize != size) {
        d->contentSize = size;
        // Note: inResize is a bitfield so QScopedValueRollback can't be used here
        const bool wasInResize = d->inResize;
        d->inResize = true;
        if (!wasInResize)
            emit contentSizeChanged();
        d->inResize = wasInResize;
        updateTotalLines();
    }
}

void QQuickTextEdit::updateWholeDocument()
{
    Q_D(QQuickTextEdit);
    if (!d->textNodeMap.isEmpty()) {
        for (TextNode &node : d->textNodeMap)
            node.setDirty();
    }

    if (isComponentComplete()) {
        polish();
        d->updateType = QQuickTextEditPrivate::UpdatePaintNode;
        update();
    }
}

void QQuickTextEdit::invalidateBlock(const QTextBlock &block)
{
    Q_D(QQuickTextEdit);
    markDirtyNodesForRange(block.position(), block.position() + block.length(), 0);

    if (isComponentComplete()) {
        polish();
        d->updateType = QQuickTextEditPrivate::UpdatePaintNode;
        update();
    }
}

void QQuickTextEdit::updateCursor()
{
    Q_D(QQuickTextEdit);
    if (isComponentComplete() && isVisible()) {
        polish();
        d->updateType = QQuickTextEditPrivate::UpdatePaintNode;
        update();
    }
}

void QQuickTextEdit::q_linkHovered(const QString &link)
{
    Q_D(QQuickTextEdit);
    emit linkHovered(link);
#if QT_CONFIG(cursor)
    if (link.isEmpty()) {
        d->updateMouseCursorShape();
    } else if (cursor().shape() != Qt::PointingHandCursor) {
        setCursor(Qt::PointingHandCursor);
    }
#endif
}

void QQuickTextEdit::q_markerHovered(bool hovered)
{
    Q_D(QQuickTextEdit);
#if QT_CONFIG(cursor)
    if (!hovered) {
        d->updateMouseCursorShape();
    } else if (cursor().shape() != Qt::PointingHandCursor) {
        setCursor(Qt::PointingHandCursor);
    }
#endif
}

void QQuickTextEdit::q_updateAlignment()
{
    Q_D(QQuickTextEdit);
    if (d->determineHorizontalAlignment()) {
        d->updateDefaultTextOption();
        d->xoff = qMax(qreal(0), QQuickTextUtil::alignedX(d->document->size().width(), width(), effectiveHAlign()));
        moveCursorDelegate();
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
    const Qt::Alignment oldAlignment = opt.alignment();
    Qt::LayoutDirection oldTextDirection = opt.textDirection();

    QQuickTextEdit::HAlignment horizontalAlignment = q->effectiveHAlign();
    if (contentDirection == Qt::RightToLeft) {
        if (horizontalAlignment == QQuickTextEdit::AlignLeft)
            horizontalAlignment = QQuickTextEdit::AlignRight;
        else if (horizontalAlignment == QQuickTextEdit::AlignRight)
            horizontalAlignment = QQuickTextEdit::AlignLeft;
    }
    if (!hAlignImplicit)
        opt.setAlignment((Qt::Alignment)(int)(horizontalAlignment | vAlign));
    else
        opt.setAlignment(Qt::Alignment(vAlign));

#if QT_CONFIG(im)
    if (contentDirection == Qt::LayoutDirectionAuto) {
        opt.setTextDirection(qGuiApp->inputMethod()->inputDirection());
    } else
#endif
    {
        opt.setTextDirection(contentDirection);
    }

    QTextOption::WrapMode oldWrapMode = opt.wrapMode();
    opt.setWrapMode(QTextOption::WrapMode(wrapMode));

    bool oldUseDesignMetrics = opt.useDesignMetrics();
    opt.setUseDesignMetrics(renderType != QQuickTextEdit::NativeRendering);

    if (oldWrapMode != opt.wrapMode() || oldAlignment != opt.alignment()
        || oldTextDirection != opt.textDirection()
        || oldUseDesignMetrics != opt.useDesignMetrics()) {
        document->setDefaultTextOption(opt);
    }
}

void QQuickTextEditPrivate::onDocumentStatusChanged()
{
    Q_ASSERT(quickDocument);
    switch (quickDocument->status()) {
    case QQuickTextDocument::Status::Loaded:
    case QQuickTextDocument::Status::Saved:
        switch (QQuickTextDocumentPrivate::get(quickDocument)->detectedFormat) {
        case Qt::RichText:
            richText = (format == QQuickTextEdit::RichText || format == QQuickTextEdit::AutoText);
            markdownText = false;
            break;
        case Qt::MarkdownText:
            richText = false;
            markdownText = (format == QQuickTextEdit::MarkdownText || format == QQuickTextEdit::AutoText);
            break;
        case Qt::PlainText:
            richText = false;
            markdownText = false;
            break;
        case Qt::AutoText: // format not detected
            break;
        }
        break;
    default:
        break;
    }
}

void QQuickTextEdit::focusInEvent(QFocusEvent *event)
{
    Q_D(QQuickTextEdit);
    d->handleFocusEvent(event);
    QQuickImplicitSizeItem::focusInEvent(event);
}

void QQuickTextEdit::focusOutEvent(QFocusEvent *event)
{
    Q_D(QQuickTextEdit);
    d->handleFocusEvent(event);
    QQuickImplicitSizeItem::focusOutEvent(event);
}

void QQuickTextEditPrivate::handleFocusEvent(QFocusEvent *event)
{
    Q_Q(QQuickTextEdit);
    bool focus = event->type() == QEvent::FocusIn;
    if (!q->isReadOnly())
        q->setCursorVisible(focus);
    control->processEvent(event, QPointF(-xoff, -yoff));
    if (focus) {
        q->q_updateAlignment();
#if QT_CONFIG(im)
        if (focusOnPress && !q->isReadOnly())
            qGuiApp->inputMethod()->show();
        q->connect(QGuiApplication::inputMethod(), SIGNAL(inputDirectionChanged(Qt::LayoutDirection)),
                q, SLOT(q_updateAlignment()));
#endif
    } else {
#if QT_CONFIG(im)
        q->disconnect(QGuiApplication::inputMethod(), SIGNAL(inputDirectionChanged(Qt::LayoutDirection)),
                   q, SLOT(q_updateAlignment()));
#endif
        if (event->reason() != Qt::ActiveWindowFocusReason
                && event->reason() != Qt::PopupFocusReason
                && control->textCursor().hasSelection()
                && !persistentSelection)
            q->deselect();

        emit q->editingFinished();
    }
}

void QQuickTextEditPrivate::addCurrentTextNodeToRoot(QQuickTextNodeEngine *engine, QSGTransformNode *root, QSGInternalTextNode *node, TextNodeIterator &it, int startPos)
{
    engine->addToSceneGraph(node, QQuickText::Normal, QColor());
    it = textNodeMap.insert(it, TextNode(startPos, node));
    ++it;
    root->appendChildNode(node);
    ++renderedBlockCount;
}

QSGInternalTextNode *QQuickTextEditPrivate::createTextNode()
{
    Q_Q(QQuickTextEdit);
    QSGInternalTextNode* node = sceneGraphContext()->createInternalTextNode(sceneGraphRenderContext());
    node->setRenderType(QSGTextNode::RenderType(renderType));
    node->setFiltering(q->smooth() ? QSGTexture::Linear : QSGTexture::Nearest);
    return node;
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
    \qmlmethod string QtQuick::TextEdit::getText(int start, int end)

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
#if QT_CONFIG(texthtmlparser)
    return d->richText || d->markdownText
            ? cursor.selectedText()
            : cursor.selection().toPlainText();
#else
    return cursor.selection().toPlainText();
#endif
}

/*!
    \qmlmethod string QtQuick::TextEdit::getFormattedText(int start, int end)

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
#if QT_CONFIG(texthtmlparser)
        return cursor.selection().toHtml();
#else
        return cursor.selection().toPlainText();
#endif
    } else if (d->markdownText) {
#if QT_CONFIG(textmarkdownwriter)
        return cursor.selection().toMarkdown();
#else
        return cursor.selection().toPlainText();
#endif
    } else {
        return cursor.selection().toPlainText();
    }
}

/*!
    \qmlmethod QtQuick::TextEdit::insert(int position, string text)

    Inserts \a text into the TextEdit at \a position.
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
#if QT_CONFIG(texthtmlparser)
        cursor.insertHtml(text);
#else
        cursor.insertText(text);
#endif
    } else if (d->markdownText) {
#if QT_CONFIG(textmarkdownreader)
        cursor.insertMarkdown(text);
#else
        cursor.insertText(text);
#endif
    } else {
        cursor.insertText(text);
    }
    d->control->updateCursorRectangle(false);
}

/*!
    \qmlmethod string QtQuick::TextEdit::remove(int start, int end)

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
    d->control->updateCursorRectangle(false);
}

/*!
    \qmlproperty TextDocument QtQuick::TextEdit::textDocument
    \since 5.1

    Returns the QQuickTextDocument of this TextEdit.
    Since Qt 6.7, it has features for loading and saving files.
    It can also be used in C++ as a means of accessing the underlying QTextDocument
    instance, for example to install a \l QSyntaxHighlighter.

    \sa QQuickTextDocument
*/

QQuickTextDocument *QQuickTextEdit::textDocument()
{
    Q_D(QQuickTextEdit);
    if (!d->quickDocument) {
        d->quickDocument = new QQuickTextDocument(this);
        connect(d->quickDocument, &QQuickTextDocument::statusChanged, d->quickDocument,
                [d]() { d->onDocumentStatusChanged(); } );
    }
    return d->quickDocument;
}

bool QQuickTextEditPrivate::isLinkHoveredConnected()
{
    Q_Q(QQuickTextEdit);
    IS_SIGNAL_CONNECTED(q, QQuickTextEdit, linkHovered, (const QString &));
}

#if QT_CONFIG(cursor)
void QQuickTextEditPrivate::updateMouseCursorShape()
{
    Q_Q(QQuickTextEdit);
    q->setCursor(q->isReadOnly() && !q->selectByMouse() ? Qt::ArrowCursor : Qt::IBeamCursor);
}
#endif

/*!
    \qmlsignal QtQuick::TextEdit::linkHovered(string link)
    \since 5.2

    This signal is emitted when the user hovers a link embedded in the text.
    The link must be in rich text or HTML format and the
    \a link string provides access to the particular link.

    \sa hoveredLink, linkAt()
*/

/*!
    \qmlsignal QtQuick::TextEdit::editingFinished()
    \since 5.6

    This signal is emitted when the text edit loses focus.
*/

/*!
    \qmlproperty string QtQuick::TextEdit::hoveredLink
    \since 5.2

    This property contains the link string when the user hovers a link
    embedded in the text. The link must be in rich text or HTML format
    and the link string provides access to the particular link.

    \sa linkHovered, linkAt()
*/

/*!
    \qmlsignal QtQuick::TextEdit::textEdited()
    \since 6.9

    This signal is emitted whenever the text is edited. Unlike \l{TextEdit::text}{textChanged()},
    this signal is not emitted when the text is changed programmatically, for example,
    by changing the value of the \l text property or by calling \l clear().
*/

QString QQuickTextEdit::hoveredLink() const
{
    Q_D(const QQuickTextEdit);
    if (const_cast<QQuickTextEditPrivate *>(d)->isLinkHoveredConnected()) {
        return d->control->hoveredLink();
    } else {
#if QT_CONFIG(cursor)
        if (QQuickWindow *wnd = window()) {
            QPointF pos = QCursor::pos(wnd->screen()) - wnd->position() - mapToScene(QPointF(0, 0));
            return d->control->anchorAt(pos);
        }
#endif // cursor
    }
    return QString();
}

void QQuickTextEdit::hoverEnterEvent(QHoverEvent *event)
{
    Q_D(QQuickTextEdit);
    if (d->isLinkHoveredConnected())
        d->control->processEvent(event, QPointF(-d->xoff, -d->yoff));
    event->ignore();
}

void QQuickTextEdit::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(QQuickTextEdit);
    if (d->isLinkHoveredConnected())
        d->control->processEvent(event, QPointF(-d->xoff, -d->yoff));
    event->ignore();
}

void QQuickTextEdit::hoverLeaveEvent(QHoverEvent *event)
{
    Q_D(QQuickTextEdit);
    if (d->isLinkHoveredConnected())
        d->control->processEvent(event, QPointF(-d->xoff, -d->yoff));
    event->ignore();
}

/*!
    \qmlmethod void QtQuick::TextEdit::append(string text)
    \since 5.2

    Appends a new paragraph with \a text to the end of the TextEdit.

    In order to append without inserting a new paragraph,
    call \c myTextEdit.insert(myTextEdit.length, text) instead.
*/
void QQuickTextEdit::append(const QString &text)
{
    Q_D(QQuickTextEdit);
    QTextCursor cursor(d->document);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::End);

    if (!d->document->isEmpty())
        cursor.insertBlock();

    if (d->format == RichText || (d->format == AutoText && Qt::mightBeRichText(text))) {
#if QT_CONFIG(texthtmlparser)
        cursor.insertHtml(text);
#else
        cursor.insertText(text);
#endif
    } else if (d->format == MarkdownText) {
#if QT_CONFIG(textmarkdownreader)
        cursor.insertMarkdown(text);
#else
        cursor.insertText(text);
#endif
    } else {
        cursor.insertText(text);
    }

    cursor.endEditBlock();
    d->control->updateCursorRectangle(false);
}

/*!
    \qmlmethod QtQuick::TextEdit::linkAt(real x, real y)
    \since 5.3

    Returns the link string at point \a x, \a y in content coordinates,
    or an empty string if no link exists at that point.

    \sa hoveredLink
*/
QString QQuickTextEdit::linkAt(qreal x, qreal y) const
{
    Q_D(const QQuickTextEdit);
    return d->control->anchorAt(QPointF(x + topPadding(), y + leftPadding()));
}

/*!
    \since 5.6
    \qmlproperty real QtQuick::TextEdit::padding
    \qmlproperty real QtQuick::TextEdit::topPadding
    \qmlproperty real QtQuick::TextEdit::leftPadding
    \qmlproperty real QtQuick::TextEdit::bottomPadding
    \qmlproperty real QtQuick::TextEdit::rightPadding

    These properties hold the padding around the content. This space is reserved
    in addition to the contentWidth and contentHeight.
*/
qreal QQuickTextEdit::padding() const
{
    Q_D(const QQuickTextEdit);
    return d->padding();
}

void QQuickTextEdit::setPadding(qreal padding)
{
    Q_D(QQuickTextEdit);
    if (qFuzzyCompare(d->padding(), padding))
        return;

    d->extra.value().padding = padding;
    updateSize();
    if (isComponentComplete()) {
        d->updateType = QQuickTextEditPrivate::UpdatePaintNode;
        update();
    }
    emit paddingChanged();
    if (!d->extra.isAllocated() || !d->extra->explicitTopPadding)
        emit topPaddingChanged();
    if (!d->extra.isAllocated() || !d->extra->explicitLeftPadding)
        emit leftPaddingChanged();
    if (!d->extra.isAllocated() || !d->extra->explicitRightPadding)
        emit rightPaddingChanged();
    if (!d->extra.isAllocated() || !d->extra->explicitBottomPadding)
        emit bottomPaddingChanged();
}

void QQuickTextEdit::resetPadding()
{
    setPadding(0);
}

qreal QQuickTextEdit::topPadding() const
{
    Q_D(const QQuickTextEdit);
    if (d->extra.isAllocated() && d->extra->explicitTopPadding)
        return d->extra->topPadding;
    return d->padding();
}

void QQuickTextEdit::setTopPadding(qreal padding)
{
    Q_D(QQuickTextEdit);
    d->setTopPadding(padding);
}

void QQuickTextEdit::resetTopPadding()
{
    Q_D(QQuickTextEdit);
    d->setTopPadding(0, true);
}

qreal QQuickTextEdit::leftPadding() const
{
    Q_D(const QQuickTextEdit);
    if (d->extra.isAllocated() && d->extra->explicitLeftPadding)
        return d->extra->leftPadding;
    return d->padding();
}

void QQuickTextEdit::setLeftPadding(qreal padding)
{
    Q_D(QQuickTextEdit);
    d->setLeftPadding(padding);
}

void QQuickTextEdit::resetLeftPadding()
{
    Q_D(QQuickTextEdit);
    d->setLeftPadding(0, true);
}

qreal QQuickTextEdit::rightPadding() const
{
    Q_D(const QQuickTextEdit);
    if (d->extra.isAllocated() && d->extra->explicitRightPadding)
        return d->extra->rightPadding;
    return d->padding();
}

void QQuickTextEdit::setRightPadding(qreal padding)
{
    Q_D(QQuickTextEdit);
    d->setRightPadding(padding);
}

void QQuickTextEdit::resetRightPadding()
{
    Q_D(QQuickTextEdit);
    d->setRightPadding(0, true);
}

qreal QQuickTextEdit::bottomPadding() const
{
    Q_D(const QQuickTextEdit);
    if (d->extra.isAllocated() && d->extra->explicitBottomPadding)
        return d->extra->bottomPadding;
    return d->padding();
}

void QQuickTextEdit::setBottomPadding(qreal padding)
{
    Q_D(QQuickTextEdit);
    d->setBottomPadding(padding);
}

void QQuickTextEdit::resetBottomPadding()
{
    Q_D(QQuickTextEdit);
    d->setBottomPadding(0, true);
}

/*!
    \qmlproperty real QtQuick::TextEdit::tabStopDistance
    \since 5.10

    The default distance, in device units, between tab stops.

    \sa QTextOption::setTabStopDistance()
*/
int QQuickTextEdit::tabStopDistance() const
{
    Q_D(const QQuickTextEdit);
    return d->document->defaultTextOption().tabStopDistance();
}

void QQuickTextEdit::setTabStopDistance(qreal distance)
{
    Q_D(QQuickTextEdit);
    QTextOption textOptions = d->document->defaultTextOption();
    if (textOptions.tabStopDistance() == distance)
        return;

    textOptions.setTabStopDistance(distance);
    d->document->setDefaultTextOption(textOptions);
    emit tabStopDistanceChanged(distance);
}

/*!
    \qmlmethod QtQuick::TextEdit::clear()
    \since 5.7

    Clears the contents of the text edit
    and resets partial text input from an input method.

    Use this method instead of setting the \l text property to an empty string.

    \sa QInputMethod::reset()
*/
void QQuickTextEdit::clear()
{
    Q_D(QQuickTextEdit);
    d->resetInputMethod();
    d->control->clear();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QQuickTextEditPrivate::Node &n)
{
    QDebugStateSaver saver(debug);
    debug.space();
    debug << "Node(startPos:" << n.m_startPos << "dirty:" << n.m_dirty << n.m_node << ')';
    return debug;
}
#endif

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
void QQuickTextEdit::setOldSelectionDefault()
{
    Q_D(QQuickTextEdit);
    d->selectByMouse = false;
    setKeepMouseGrab(false);
    d->control->setTextInteractionFlags(d->control->textInteractionFlags() & ~Qt::TextSelectableByMouse);
    d->control->setTouchDragSelectionEnabled(true);
    qCDebug(lcTextEdit, "pre-6.4 behavior chosen: selectByMouse defaults false; if enabled, touchscreen acts like a mouse");
}

// TODO in 6.7.0: remove the note about versions prior to 6.4 in selectByMouse() documentation
QQuickPre64TextEdit::QQuickPre64TextEdit(QQuickItem *parent)
    : QQuickTextEdit(parent)
{
    setOldSelectionDefault();
}
#endif

QT_END_NAMESPACE

#include "moc_qquicktextedit_p.cpp"
