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

#include "qsgtext_p.h"
#include "qsgtext_p_p.h"

#include <private/qsgdistancefieldglyphcache_p.h>
#include <private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>
#include "qsgtextnode_p.h"
#include "qsgimage_p_p.h"
#include <private/qsgtexture_p.h>

#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtGui/qevent.h>
#include <QtGui/qabstracttextdocumentlayout.h>
#include <QtGui/qpainter.h>
#include <QtGui/qtextdocument.h>
#include <QtGui/qtextobject.h>
#include <QtGui/qtextcursor.h>
#include <QtGui/qguiapplication.h>

#include <private/qdeclarativestyledtext_p.h>
#include <private/qdeclarativepixmapcache_p.h>

#include <qmath.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

extern Q_GUI_EXPORT bool qt_applefontsmoothing_enabled;

class QSGTextDocumentWithImageResources : public QTextDocument {
    Q_OBJECT

public:
    QSGTextDocumentWithImageResources(QSGText *parent);
    virtual ~QSGTextDocumentWithImageResources();

    void setText(const QString &);
    int resourcesLoading() const { return outstanding; }

protected:
    QVariant loadResource(int type, const QUrl &name);

private slots:
    void requestFinished();

private:
    QHash<QUrl, QDeclarativePixmap *> m_resources;

    int outstanding;
    static QSet<QUrl> errors;
};

DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)
DEFINE_BOOL_CONFIG_OPTION(enableImageCache, QML_ENABLE_TEXT_IMAGE_CACHE);

QString QSGTextPrivate::elideChar = QString(0x2026);

QSGTextPrivate::QSGTextPrivate()
: color((QRgb)0), style(QSGText::Normal), hAlign(QSGText::AlignLeft),
  vAlign(QSGText::AlignTop), elideMode(QSGText::ElideNone),
  format(QSGText::AutoText), wrapMode(QSGText::NoWrap), lineHeight(1),
  lineHeightMode(QSGText::ProportionalHeight), lineCount(1), maximumLineCount(INT_MAX),
  maximumLineCountValid(false),
  texture(0),
  imageCacheDirty(false), updateOnComponentComplete(true),
  richText(false), styledText(false), singleline(false), cacheAllTextAsImage(true), internalWidthUpdate(false),
  requireImplicitWidth(false), truncated(false), hAlignImplicit(true), rightToLeftText(false),
  layoutTextElided(false), richTextAsImage(false), textureImageCacheDirty(false), naturalWidth(0),
  doc(0), nodeType(NodeIsNull)

#if defined(Q_OS_MAC)
  , layoutThread(0)
#endif

{
    cacheAllTextAsImage = enableImageCache();
}

void QSGTextPrivate::init()
{
    Q_Q(QSGText);
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFlag(QSGItem::ItemHasContents);
}

QSGTextDocumentWithImageResources::QSGTextDocumentWithImageResources(QSGText *parent)
: QTextDocument(parent), outstanding(0)
{
    setUndoRedoEnabled(false);
}

QSGTextDocumentWithImageResources::~QSGTextDocumentWithImageResources()
{
    if (!m_resources.isEmpty())
        qDeleteAll(m_resources);
}

QVariant QSGTextDocumentWithImageResources::loadResource(int type, const QUrl &name)
{
    QDeclarativeContext *context = qmlContext(parent());
    QUrl url = context->resolvedUrl(name);

    if (type == QTextDocument::ImageResource) {
        QHash<QUrl, QDeclarativePixmap *>::Iterator iter = m_resources.find(url);

        if (iter == m_resources.end()) {
            QDeclarativePixmap *p = new QDeclarativePixmap(context->engine(), url);
            iter = m_resources.insert(name, p);

            if (p->isLoading()) {
                p->connectFinished(this, SLOT(requestFinished()));
                outstanding++;
            }
        }

        QDeclarativePixmap *p = *iter;
        if (p->isReady()) {
            return p->pixmap();
        } else if (p->isError()) {
            if (!errors.contains(url)) {
                errors.insert(url);
                qmlInfo(parent()) << p->error();
            }
        }
    }

    return QTextDocument::loadResource(type,url); // The *resolved* URL
}

void QSGTextDocumentWithImageResources::requestFinished()
{
    outstanding--;
    if (outstanding == 0) {
        QSGText *textItem = static_cast<QSGText*>(parent());
        QString text = textItem->text();
#ifndef QT_NO_TEXTHTMLPARSER
        setHtml(text);
#else
        setPlainText(text);
#endif
        QSGTextPrivate *d = QSGTextPrivate::get(textItem);
        d->updateLayout();
    }
}

void QSGTextDocumentWithImageResources::setText(const QString &text)
{
    if (!m_resources.isEmpty()) {
        qDeleteAll(m_resources);
        m_resources.clear();
        outstanding = 0;
    }

#ifndef QT_NO_TEXTHTMLPARSER
    setHtml(text);
#else
    setPlainText(text);
#endif
}

QSet<QUrl> QSGTextDocumentWithImageResources::errors;

QSGTextPrivate::~QSGTextPrivate()
{
}

qreal QSGTextPrivate::getImplicitWidth() const
{
    if (!requireImplicitWidth) {
        // We don't calculate implicitWidth unless it is required.
        // We need to force a size update now to ensure implicitWidth is calculated
        QSGTextPrivate *me = const_cast<QSGTextPrivate*>(this);
        me->requireImplicitWidth = true;
        me->updateSize();
    }
    return implicitWidth;
}

void QSGTextPrivate::updateLayout()
{
    Q_Q(QSGText);
    if (!q->isComponentComplete()) {
        updateOnComponentComplete = true;
        return;
    }

    layoutTextElided = false;
    // Setup instance of QTextLayout for all cases other than richtext
    if (!richText) {
        layout.clearLayout();
        layout.setFont(font);
        if (!styledText) {
            QString tmp = text;
            tmp.replace(QLatin1Char('\n'), QChar::LineSeparator);
            singleline = !tmp.contains(QChar::LineSeparator);
            if (singleline && !maximumLineCountValid && elideMode != QSGText::ElideNone && q->widthValid()) {
                QFontMetrics fm(font);
                tmp = fm.elidedText(tmp,(Qt::TextElideMode)elideMode,q->width());
                if (tmp != text) {
                    layoutTextElided = true;
                    if (!truncated) {
                        truncated = true;
                        emit q->truncatedChanged();
                    }
                }
            }
            layout.setText(tmp);
        } else {
            singleline = false;
            QDeclarativeStyledText::parse(text, layout);
        }
    } else {
        ensureDoc();
        QTextBlockFormat::LineHeightTypes type;
        type = lineHeightMode == QSGText::FixedHeight ? QTextBlockFormat::FixedHeight : QTextBlockFormat::ProportionalHeight;
        QTextBlockFormat blockFormat;
        blockFormat.setLineHeight((lineHeightMode == QSGText::FixedHeight ? lineHeight : lineHeight * 100), type);
        for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next()) {
            QTextCursor cursor(it);
            cursor.setBlockFormat(blockFormat);
        }
    }

    updateSize();
}

void QSGTextPrivate::updateSize()
{
    Q_Q(QSGText);

    if (!q->isComponentComplete()) {
        updateOnComponentComplete = true;
        return;
    }

    if (!requireImplicitWidth) {
        emit q->implicitWidthChanged();
        // if the implicitWidth is used, then updateSize() has already been called (recursively)
        if (requireImplicitWidth)
            return;
    }

    invalidateImageCache();

    QFontMetrics fm(font);
    if (text.isEmpty()) {
        q->setImplicitWidth(0);
        q->setImplicitHeight(fm.height());
        paintedSize = QSize(0, fm.height());
        emit q->paintedSizeChanged();
        q->update();
        return;
    }

    int dy = q->height();
    QSize size(0, 0);

#if defined(Q_OS_MAC)
    layoutThread = QThread::currentThread();
#endif

    //setup instance of QTextLayout for all cases other than richtext
    if (!richText) {
        QRect textRect = setupTextLayout();
        layedOutTextRect = textRect;
        size = textRect.size();
        dy -= size.height();
    } else {
        singleline = false; // richtext can't elide or be optimized for single-line case
        ensureDoc();
        doc->setDefaultFont(font);
        QSGText::HAlignment horizontalAlignment = q->effectiveHAlign();
        if (rightToLeftText) {
            if (horizontalAlignment == QSGText::AlignLeft)
                horizontalAlignment = QSGText::AlignRight;
            else if (horizontalAlignment == QSGText::AlignRight)
                horizontalAlignment = QSGText::AlignLeft;
        }
        QTextOption option;
        option.setAlignment((Qt::Alignment)int(horizontalAlignment | vAlign));
        option.setWrapMode(QTextOption::WrapMode(wrapMode));
        if (!cacheAllTextAsImage && !richTextAsImage && !qmlDisableDistanceField())
            option.setUseDesignMetrics(true);
        doc->setDefaultTextOption(option);
        if (requireImplicitWidth && q->widthValid()) {
            doc->setTextWidth(-1);
            naturalWidth = doc->idealWidth();
        }
        if (wrapMode != QSGText::NoWrap && q->widthValid())
            doc->setTextWidth(q->width());
        else
            doc->setTextWidth(doc->idealWidth()); // ### Text does not align if width is not set (QTextDoc bug)
        dy -= (int)doc->size().height();
        QSize dsize = doc->size().toSize();
        layedOutTextRect = QRect(QPoint(0,0), dsize);
        size = QSize(int(doc->idealWidth()),dsize.height());
    }
    int yoff = 0;

    if (q->heightValid()) {
        if (vAlign == QSGText::AlignBottom)
            yoff = dy;
        else if (vAlign == QSGText::AlignVCenter)
            yoff = dy/2;
    }
    q->setBaselineOffset(fm.ascent() + yoff);

    //### need to comfirm cost of always setting these for richText
    internalWidthUpdate = true;
    if (!q->widthValid())
        q->setImplicitWidth(size.width());
    else if (requireImplicitWidth)
        q->setImplicitWidth(naturalWidth);
    internalWidthUpdate = false;

    q->setImplicitHeight(size.height());
    if (paintedSize != size) {
        paintedSize = size;
        emit q->paintedSizeChanged();
    }
    q->update();
}

/*!
    Lays out the QSGTextPrivate::layout QTextLayout in the constraints of the QSGText.

    Returns the size of the final text.  This can be used to position the text vertically (the text is
    already absolutely positioned horizontally).
*/
QRect QSGTextPrivate::setupTextLayout()
{
    // ### text layout handling should be profiled and optimized as needed
    // what about QStackTextEngine engine(tmp, d->font.font()); QTextLayout textLayout(&engine);
    Q_Q(QSGText);
    layout.setCacheEnabled(true);

    qreal lineWidth = 0;
    int visibleCount = 0;

    //set manual width
    if (q->widthValid())
        lineWidth = q->width();

    QTextOption textOption = layout.textOption();
    textOption.setAlignment(Qt::Alignment(q->effectiveHAlign()));
    textOption.setWrapMode(QTextOption::WrapMode(wrapMode));
    if (!cacheAllTextAsImage && !richTextAsImage && !qmlDisableDistanceField())
        textOption.setUseDesignMetrics(true);
    layout.setTextOption(textOption);

    bool elideText = false;
    bool truncate = false;

    QFontMetrics fm(layout.font());
    elidePos = QPointF();

    if (requireImplicitWidth && q->widthValid()) {
        // requires an extra layout
        QString elidedText;
        if (layoutTextElided) {
            // We have provided elided text to the layout, but we must calculate unelided width.
            elidedText = layout.text();
            layout.setText(text);
        }
        layout.beginLayout();
        forever {
            QTextLine line = layout.createLine();
            if (!line.isValid())
                break;
        }
        layout.endLayout();
        QRectF br;
        for (int i = 0; i < layout.lineCount(); ++i) {
            QTextLine line = layout.lineAt(i);
            br = br.united(line.naturalTextRect());
        }
        naturalWidth = br.width();
        if (layoutTextElided)
            layout.setText(elidedText);
    }

    if (maximumLineCountValid) {
        layout.beginLayout();
        if (!lineWidth)
            lineWidth = INT_MAX;
        int linesLeft = maximumLineCount;
        int visibleTextLength = 0;
        while (linesLeft > 0) {
            QTextLine line = layout.createLine();
            if (!line.isValid())
                break;

            visibleCount++;
            if (lineWidth)
                line.setLineWidth(lineWidth);
            visibleTextLength += line.textLength();

            if (--linesLeft == 0) {
                if (visibleTextLength < text.length()) {
                    truncate = true;
                    if (elideMode==QSGText::ElideRight && q->widthValid()) {
                        qreal elideWidth = fm.width(elideChar);
                        // Need to correct for alignment
                        line.setLineWidth(lineWidth-elideWidth);
                        if (layout.text().mid(line.textStart(), line.textLength()).isRightToLeft()) {
                            line.setPosition(QPointF(line.position().x() + elideWidth, line.position().y()));
                            elidePos.setX(line.naturalTextRect().left() - elideWidth);
                        } else {
                            elidePos.setX(line.naturalTextRect().right());
                        }
                        elideText = true;
                    }
                }
            }
        }
        layout.endLayout();

        //Update truncated
        if (truncated != truncate) {
            truncated = truncate;
            emit q->truncatedChanged();
        }
    } else {
        layout.beginLayout();
        forever {
            QTextLine line = layout.createLine();
            if (!line.isValid())
                break;
            visibleCount++;
            if (lineWidth)
                line.setLineWidth(lineWidth);
        }
        layout.endLayout();
    }

    qreal height = 0;
    QRectF br;
    for (int i = 0; i < layout.lineCount(); ++i) {
        QTextLine line = layout.lineAt(i);
        // set line spacing
        line.setPosition(QPointF(line.position().x(), height));
        if (elideText && i == layout.lineCount()-1) {
            elidePos.setY(height + fm.ascent());
            br = br.united(QRectF(elidePos, QSizeF(fm.width(elideChar), fm.ascent())));
        }
        br = br.united(line.naturalTextRect());
        height += (lineHeightMode == QSGText::FixedHeight) ? lineHeight : line.height() * lineHeight;
    }
    br.setHeight(height);

    if (!q->widthValid())
        naturalWidth = br.width();

    //Update the number of visible lines
    if (lineCount != visibleCount) {
        lineCount = visibleCount;
        emit q->lineCountChanged();
    }

    return QRect(qRound(br.x()), qRound(br.y()), qCeil(br.width()), qCeil(br.height()));
}

/*!
    Returns a painted version of the QSGTextPrivate::layout QTextLayout.
    If \a drawStyle is true, the style color overrides all colors in the document.
*/
QPixmap QSGTextPrivate::textLayoutImage(bool drawStyle)
{
    QSize size = layedOutTextRect.size();

    //paint text
    QPixmap img(size);
    if (!size.isEmpty()) {
        img.fill(Qt::transparent);
#ifdef Q_WS_MAC
        bool oldSmooth = qt_applefontsmoothing_enabled;
        qt_applefontsmoothing_enabled = false;
#endif
        QPainter p(&img);
#ifdef Q_WS_MAC
        qt_applefontsmoothing_enabled = oldSmooth;
#endif
        drawTextLayout(&p, QPointF(-layedOutTextRect.x(),0), drawStyle);
    }
    return img;
}

/*!
    Paints the QSGTextPrivate::layout QTextLayout into \a painter at \a pos.  If
    \a drawStyle is true, the style color overrides all colors in the document.
*/
void QSGTextPrivate::drawTextLayout(QPainter *painter, const QPointF &pos, bool drawStyle)
{
    if (drawStyle)
        painter->setPen(styleColor);
    else
        painter->setPen(color);
    painter->setFont(font);
    layout.draw(painter, pos);
    if (!elidePos.isNull())
        painter->drawText(pos + elidePos, elideChar);
}

/*!
    Returns a painted version of the QSGTextPrivate::doc QTextDocument.
    If \a drawStyle is true, the style color overrides all colors in the document.
*/
QPixmap QSGTextPrivate::textDocumentImage(bool drawStyle)
{
    QSize size = doc->size().toSize();

    //paint text
    QPixmap img(size);
    img.fill(Qt::transparent);
#ifdef Q_WS_MAC
    bool oldSmooth = qt_applefontsmoothing_enabled;
    qt_applefontsmoothing_enabled = false;
#endif
    QPainter p(&img);
#ifdef Q_WS_MAC
    qt_applefontsmoothing_enabled = oldSmooth;
#endif

    QAbstractTextDocumentLayout::PaintContext context;

    QTextOption oldOption(doc->defaultTextOption());
    if (drawStyle) {
        context.palette.setColor(QPalette::Text, styleColor);
        QTextOption colorOption(doc->defaultTextOption());
        colorOption.setFlags(QTextOption::SuppressColors);
        doc->setDefaultTextOption(colorOption);
    } else {
        context.palette.setColor(QPalette::Text, color);
    }
    doc->documentLayout()->draw(&p, context);
    if (drawStyle)
        doc->setDefaultTextOption(oldOption);
    return img;
}

/*!
    Mark the image cache as dirty.
*/
void QSGTextPrivate::invalidateImageCache()
{
    Q_Q(QSGText);

    if(richTextAsImage || cacheAllTextAsImage || (qmlDisableDistanceField() && style != QSGText::Normal)){//If actually using the image cache
        if (imageCacheDirty)
            return;

        imageCacheDirty = true;

        if (q->isComponentComplete())
            QCoreApplication::postEvent(q, new QEvent(QEvent::User));
    } else if (q->isComponentComplete())
        q->update();
}

/*!
    Tests if the image cache is dirty, and repaints it if it is.
*/
void QSGTextPrivate::checkImageCache()
{
    Q_Q(QSGText);

    if (!imageCacheDirty)
        return;

    if (text.isEmpty()) {

        imageCache = QPixmap();

    } else {

        QPixmap textImage;
        QPixmap styledImage;

        if (richText) {
            textImage = textDocumentImage(false);
            if (style != QSGText::Normal)
                styledImage = textDocumentImage(true); //### should use styleColor
        } else {
            textImage = textLayoutImage(false);
            if (style != QSGText::Normal)
                styledImage = textLayoutImage(true); //### should use styleColor
        }

        switch (style) {
        case QSGText::Outline:
            imageCache = drawOutline(textImage, styledImage);
            break;
        case QSGText::Sunken:
            imageCache = drawOutline(textImage, styledImage, -1);
            break;
        case QSGText::Raised:
            imageCache = drawOutline(textImage, styledImage, 1);
            break;
        default:
            imageCache = textImage;
            break;
        }

    }

    imageCacheDirty = false;
    textureImageCacheDirty = true;
    q->update();
}

/*!
    Ensures the QSGTextPrivate::doc variable is set to a valid text document
*/
void QSGTextPrivate::ensureDoc()
{
    if (!doc) {
        Q_Q(QSGText);
        doc = new QSGTextDocumentWithImageResources(q);
        doc->setDocumentMargin(0);
    }
}

/*!
    Draw \a styleSource as an outline around \a source and return the new image.
*/
QPixmap QSGTextPrivate::drawOutline(const QPixmap &source, const QPixmap &styleSource)
{
    QPixmap img = QPixmap(styleSource.width() + 2, styleSource.height() + 2);
    img.fill(Qt::transparent);

    QPainter ppm(&img);

    QPoint pos(0, 0);
    pos += QPoint(-1, 0);
    ppm.drawPixmap(pos, styleSource);
    pos += QPoint(2, 0);
    ppm.drawPixmap(pos, styleSource);
    pos += QPoint(-1, -1);
    ppm.drawPixmap(pos, styleSource);
    pos += QPoint(0, 2);
    ppm.drawPixmap(pos, styleSource);

    pos += QPoint(0, -1);
    ppm.drawPixmap(pos, source);
    ppm.end();

    return img;
}

/*!
    Draw \a styleSource below \a source at \a yOffset and return the new image.
*/
QPixmap QSGTextPrivate::drawOutline(const QPixmap &source, const QPixmap &styleSource, int yOffset)
{
    QPixmap img = QPixmap(styleSource.width() + 2, styleSource.height() + 2);
    img.fill(Qt::transparent);

    QPainter ppm(&img);

    ppm.drawPixmap(QPoint(0, yOffset), styleSource);
    ppm.drawPixmap(0, 0, source);

    ppm.end();

    return img;
}

/*!
    \qmlclass Text QSGText
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-visual-elements
    \brief The Text item allows you to add formatted text to a scene.
    \inherits Item

    Text items can display both plain and rich text. For example, red text with
    a specific font and size can be defined like this:

    \qml
    Text {
        text: "Hello World!"
        font.family: "Helvetica"
        font.pointSize: 24
        color: "red"
    }
    \endqml

    Rich text is defined using HTML-style markup:

    \qml
    Text {
        text: "<b>Hello</b> <i>World!</i>"
    }
    \endqml

    \image declarative-text.png

    If height and width are not explicitly set, Text will attempt to determine how
    much room is needed and set it accordingly. Unless \l wrapMode is set, it will always
    prefer width to height (all text will be placed on a single line).

    The \l elide property can alternatively be used to fit a single line of
    plain text to a set width.

    Note that the \l{Supported HTML Subset} is limited. Also, if the text contains
    HTML img tags that load remote images, the text is reloaded.

    Text provides read-only text. For editable text, see \l TextEdit.

    \sa {declarative/text/fonts}{Fonts example}
*/
QSGText::QSGText(QSGItem *parent)
: QSGImplicitSizeItem(*(new QSGTextPrivate), parent)
{
    Q_D(QSGText);
    d->init();
}

QSGText::~QSGText()
{
}

/*!
  \qmlproperty bool QtQuick2::Text::clip
  This property holds whether the text is clipped.

  Note that if the text does not fit in the bounding rectangle it will be abruptly chopped.

  If you want to display potentially long text in a limited space, you probably want to use \c elide instead.
*/

/*!
    \qmlproperty bool QtQuick2::Text::smooth

    This property holds whether the text is smoothly scaled or transformed.

    Smooth filtering gives better visual quality, but is slower.  If
    the item is displayed at its natural size, this property has no visual or
    performance effect.

    \note Generally scaling artifacts are only visible if the item is stationary on
    the screen.  A common pattern when animating an item is to disable smooth
    filtering at the beginning of the animation and reenable it at the conclusion.
*/

/*!
    \qmlsignal QtQuick2::Text::onLinkActivated(string link)

    This handler is called when the user clicks on a link embedded in the text.
    The link must be in rich text or HTML format and the
    \a link string provides access to the particular link.

    \snippet doc/src/snippets/declarative/text/onLinkActivated.qml 0

    The example code will display the text
    "The main website is at \l{http://qt.nokia.com}{Nokia Qt DF}."

    Clicking on the highlighted link will output
    \tt{http://qt.nokia.com link activated} to the console.
*/

/*!
    \qmlproperty string QtQuick2::Text::font.family

    Sets the family name of the font.

    The family name is case insensitive and may optionally include a foundry name, e.g. "Helvetica [Cronyx]".
    If the family is available from more than one foundry and the foundry isn't specified, an arbitrary foundry is chosen.
    If the family isn't available a family will be set using the font matching algorithm.
*/

/*!
    \qmlproperty bool QtQuick2::Text::font.bold

    Sets whether the font weight is bold.
*/

/*!
    \qmlproperty enumeration QtQuick2::Text::font.weight

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
    Text { text: "Hello"; font.weight: Font.DemiBold }
    \endqml
*/

/*!
    \qmlproperty bool QtQuick2::Text::font.italic

    Sets whether the font has an italic style.
*/

/*!
    \qmlproperty bool QtQuick2::Text::font.underline

    Sets whether the text is underlined.
*/

/*!
    \qmlproperty bool QtQuick2::Text::font.strikeout

    Sets whether the font has a strikeout style.
*/

/*!
    \qmlproperty real QtQuick2::Text::font.pointSize

    Sets the font size in points. The point size must be greater than zero.
*/

/*!
    \qmlproperty int QtQuick2::Text::font.pixelSize

    Sets the font size in pixels.

    Using this function makes the font device dependent.
    Use \c pointSize to set the size of the font in a device independent manner.
*/

/*!
    \qmlproperty real QtQuick2::Text::font.letterSpacing

    Sets the letter spacing for the font.

    Letter spacing changes the default spacing between individual letters in the font.
    A positive value increases the letter spacing by the corresponding pixels; a negative value decreases the spacing.
*/

/*!
    \qmlproperty real QtQuick2::Text::font.wordSpacing

    Sets the word spacing for the font.

    Word spacing changes the default spacing between individual words.
    A positive value increases the word spacing by a corresponding amount of pixels,
    while a negative value decreases the inter-word spacing accordingly.
*/

/*!
    \qmlproperty enumeration QtQuick2::Text::font.capitalization

    Sets the capitalization for the text.

    \list
    \o Font.MixedCase - This is the normal text rendering option where no capitalization change is applied.
    \o Font.AllUppercase - This alters the text to be rendered in all uppercase type.
    \o Font.AllLowercase	 - This alters the text to be rendered in all lowercase type.
    \o Font.SmallCaps -	This alters the text to be rendered in small-caps type.
    \o Font.Capitalize - This alters the text to be rendered with the first character of each word as an uppercase character.
    \endlist

    \qml
    Text { text: "Hello"; font.capitalization: Font.AllLowercase }
    \endqml
*/
QFont QSGText::font() const
{
    Q_D(const QSGText);
    return d->sourceFont;
}

void QSGText::setFont(const QFont &font)
{
    Q_D(QSGText);
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

    if (oldFont != d->font)
        d->updateLayout();

    emit fontChanged(d->sourceFont);
}

/*!
    \qmlproperty string QtQuick2::Text::text

    The text to display. Text supports both plain and rich text strings.

    The item will try to automatically determine whether the text should
    be treated as styled text. This determination is made using Qt::mightBeRichText().
*/
QString QSGText::text() const
{
    Q_D(const QSGText);
    return d->text;
}

void QSGText::setText(const QString &n)
{
    Q_D(QSGText);
    if (d->text == n)
        return;

    d->richText = d->format == RichText;
    d->styledText = d->format == StyledText || (d->format == AutoText && Qt::mightBeRichText(n));
    d->text = n;
    if (isComponentComplete()) {
        if (d->richText) {
            d->ensureDoc();
            d->doc->setText(n);
            d->rightToLeftText = d->doc->toPlainText().isRightToLeft();
            d->richTextAsImage = enableImageCache();
        } else {
            d->rightToLeftText = d->text.isRightToLeft();
        }
        d->determineHorizontalAlignment();
    }
    d->updateLayout();
    emit textChanged(d->text);
}

/*!
    \qmlproperty color QtQuick2::Text::color

    The text color.

    An example of green text defined using hexadecimal notation:
    \qml
    Text {
        color: "#00FF00"
        text: "green text"
    }
    \endqml

    An example of steel blue text defined using an SVG color name:
    \qml
    Text {
        color: "steelblue"
        text: "blue text"
    }
    \endqml
*/
QColor QSGText::color() const
{
    Q_D(const QSGText);
    return d->color;
}

void QSGText::setColor(const QColor &color)
{
    Q_D(QSGText);
    if (d->color == color)
        return;

    d->color = color;
    d->invalidateImageCache();
    emit colorChanged(d->color);
}
/*!
    \qmlproperty enumeration QtQuick2::Text::style

    Set an additional text style.

    Supported text styles are:
    \list
    \o Text.Normal - the default
    \o Text.Outline
    \o Text.Raised
    \o Text.Sunken
    \endlist

    \qml
    Row {
        Text { font.pointSize: 24; text: "Normal" }
        Text { font.pointSize: 24; text: "Raised"; style: Text.Raised; styleColor: "#AAAAAA" }
        Text { font.pointSize: 24; text: "Outline";style: Text.Outline; styleColor: "red" }
        Text { font.pointSize: 24; text: "Sunken"; style: Text.Sunken; styleColor: "#AAAAAA" }
    }
    \endqml

    \image declarative-textstyle.png
*/
QSGText::TextStyle QSGText::style() const
{
    Q_D(const QSGText);
    return d->style;
}

void QSGText::setStyle(QSGText::TextStyle style)
{
    Q_D(QSGText);
    if (d->style == style)
        return;

    // changing to/from Normal requires the boundingRect() to change
    if (isComponentComplete() && (d->style == Normal || style == Normal))
        update();
    d->style = style;
    d->invalidateImageCache();
    emit styleChanged(d->style);
}

/*!
    \qmlproperty color QtQuick2::Text::styleColor

    Defines the secondary color used by text styles.

    \c styleColor is used as the outline color for outlined text, and as the
    shadow color for raised or sunken text. If no style has been set, it is not
    used at all.

    \qml
    Text { font.pointSize: 18; text: "hello"; style: Text.Raised; styleColor: "gray" }
    \endqml

    \sa style
 */
QColor QSGText::styleColor() const
{
    Q_D(const QSGText);
    return d->styleColor;
}

void QSGText::setStyleColor(const QColor &color)
{
    Q_D(QSGText);
    if (d->styleColor == color)
        return;

    d->styleColor = color;
    d->invalidateImageCache();
    emit styleColorChanged(d->styleColor);
}

/*!
    \qmlproperty enumeration QtQuick2::Text::horizontalAlignment
    \qmlproperty enumeration QtQuick2::Text::verticalAlignment
    \qmlproperty enumeration QtQuick2::Text::effectiveHorizontalAlignment

    Sets the horizontal and vertical alignment of the text within the Text items
    width and height. By default, the text is vertically aligned to the top. Horizontal
    alignment follows the natural alignment of the text, for example text that is read
    from left to right will be aligned to the left.

    The valid values for \c horizontalAlignment are \c Text.AlignLeft, \c Text.AlignRight, \c Text.AlignHCenter and
    \c Text.AlignJustify.  The valid values for \c verticalAlignment are \c Text.AlignTop, \c Text.AlignBottom
    and \c Text.AlignVCenter.

    Note that for a single line of text, the size of the text is the area of the text. In this common case,
    all alignments are equivalent. If you want the text to be, say, centered in its parent, then you will
    need to either modify the Item::anchors, or set horizontalAlignment to Text.AlignHCenter and bind the width to
    that of the parent.

    When using the attached property LayoutMirroring::enabled to mirror application
    layouts, the horizontal alignment of text will also be mirrored. However, the property
    \c horizontalAlignment will remain unchanged. To query the effective horizontal alignment
    of Text, use the read-only property \c effectiveHorizontalAlignment.
*/
QSGText::HAlignment QSGText::hAlign() const
{
    Q_D(const QSGText);
    return d->hAlign;
}

void QSGText::setHAlign(HAlignment align)
{
    Q_D(QSGText);
    bool forceAlign = d->hAlignImplicit && d->effectiveLayoutMirror;
    d->hAlignImplicit = false;
    if (d->setHAlign(align, forceAlign) && isComponentComplete())
        d->updateLayout();
}

void QSGText::resetHAlign()
{
    Q_D(QSGText);
    d->hAlignImplicit = true;
    if (d->determineHorizontalAlignment() && isComponentComplete())
        d->updateLayout();
}

QSGText::HAlignment QSGText::effectiveHAlign() const
{
    Q_D(const QSGText);
    QSGText::HAlignment effectiveAlignment = d->hAlign;
    if (!d->hAlignImplicit && d->effectiveLayoutMirror) {
        switch (d->hAlign) {
        case QSGText::AlignLeft:
            effectiveAlignment = QSGText::AlignRight;
            break;
        case QSGText::AlignRight:
            effectiveAlignment = QSGText::AlignLeft;
            break;
        default:
            break;
        }
    }
    return effectiveAlignment;
}

bool QSGTextPrivate::setHAlign(QSGText::HAlignment alignment, bool forceAlign)
{
    Q_Q(QSGText);
    if (hAlign != alignment || forceAlign) {
        QSGText::HAlignment oldEffectiveHAlign = q->effectiveHAlign();
        hAlign = alignment;

        emit q->horizontalAlignmentChanged(hAlign);
        if (oldEffectiveHAlign != q->effectiveHAlign())
            emit q->effectiveHorizontalAlignmentChanged();
        return true;
    }
    return false;
}

bool QSGTextPrivate::determineHorizontalAlignment()
{
    Q_Q(QSGText);
    if (hAlignImplicit && q->isComponentComplete()) {
        bool alignToRight = text.isEmpty() ? QGuiApplication::keyboardInputDirection() == Qt::RightToLeft : rightToLeftText;
        return setHAlign(alignToRight ? QSGText::AlignRight : QSGText::AlignLeft);
    }
    return false;
}

void QSGTextPrivate::mirrorChange()
{
    Q_Q(QSGText);
    if (q->isComponentComplete()) {
        if (!hAlignImplicit && (hAlign == QSGText::AlignRight || hAlign == QSGText::AlignLeft)) {
            updateLayout();
            emit q->effectiveHorizontalAlignmentChanged();
        }
    }
}

QTextDocument *QSGTextPrivate::textDocument()
{
    return doc;
}

QSGText::VAlignment QSGText::vAlign() const
{
    Q_D(const QSGText);
    return d->vAlign;
}

void QSGText::setVAlign(VAlignment align)
{
    Q_D(QSGText);
    if (d->vAlign == align)
        return;

    d->vAlign = align;
    emit verticalAlignmentChanged(align);
}

/*!
    \qmlproperty enumeration QtQuick2::Text::wrapMode

    Set this property to wrap the text to the Text item's width.  The text will only
    wrap if an explicit width has been set.  wrapMode can be one of:

    \list
    \o Text.NoWrap (default) - no wrapping will be performed. If the text contains insufficient newlines, then \l paintedWidth will exceed a set width.
    \o Text.WordWrap - wrapping is done on word boundaries only. If a word is too long, \l paintedWidth will exceed a set width.
    \o Text.WrapAnywhere - wrapping is done at any point on a line, even if it occurs in the middle of a word.
    \o Text.Wrap - if possible, wrapping occurs at a word boundary; otherwise it will occur at the appropriate point on the line, even in the middle of a word.
    \endlist
*/
QSGText::WrapMode QSGText::wrapMode() const
{
    Q_D(const QSGText);
    return d->wrapMode;
}

void QSGText::setWrapMode(WrapMode mode)
{
    Q_D(QSGText);
    if (mode == d->wrapMode)
        return;

    d->wrapMode = mode;
    d->updateLayout();

    emit wrapModeChanged();
}

/*!
    \qmlproperty int QtQuick2::Text::lineCount

    Returns the number of lines visible in the text item.

    This property is not supported for rich text.

    \sa maximumLineCount
*/
int QSGText::lineCount() const
{
    Q_D(const QSGText);
    return d->lineCount;
}

/*!
    \qmlproperty bool QtQuick2::Text::truncated

    Returns true if the text has been truncated due to \l maximumLineCount
    or \l elide.

    This property is not supported for rich text.

    \sa maximumLineCount, elide
*/
bool QSGText::truncated() const
{
    Q_D(const QSGText);
    return d->truncated;
}

/*!
    \qmlproperty int QtQuick2::Text::maximumLineCount

    Set this property to limit the number of lines that the text item will show.
    If elide is set to Text.ElideRight, the text will be elided appropriately.
    By default, this is the value of the largest possible integer.

    This property is not supported for rich text.

    \sa lineCount, elide
*/
int QSGText::maximumLineCount() const
{
    Q_D(const QSGText);
    return d->maximumLineCount;
}

void QSGText::setMaximumLineCount(int lines)
{
    Q_D(QSGText);

    d->maximumLineCountValid = lines==INT_MAX ? false : true;
    if (d->maximumLineCount != lines) {
        d->maximumLineCount = lines;
        d->updateLayout();
        emit maximumLineCountChanged();
    }
}

void QSGText::resetMaximumLineCount()
{
    Q_D(QSGText);
    setMaximumLineCount(INT_MAX);
    d->elidePos = QPointF();
    if (d->truncated != false) {
        d->truncated = false;
        emit truncatedChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick2::Text::textFormat

    The way the text property should be displayed.

    Supported text formats are:

    \list
    \o Text.AutoText (default)
    \o Text.PlainText
    \o Text.StyledText
    \o Text.RichText
    \endlist

    If the text format is \c Text.AutoText the text element
    will automatically determine whether the text should be treated as
    styled text.  This determination is made using Qt::mightBeRichText().

    Text.StyledText is an optimized format supporting some basic text
    styling markup, in the style of html 3.2:

    \code
    <b></b> - bold
    <i></i> - italic
    <br> - new line
    <p> - paragraph
    <u> - underlined text
    <font color="color_name" size="1-7"></font>
    <h1> to <h6> - headers
    <a href=""> - anchor
    <ol type="">, <ul type=""> and <li> - ordered and unordered lists
    &gt; &lt; &amp;
    \endcode

    \c Text.StyledText parser is strict, requiring tags to be correctly nested.

    \table
    \row
    \o
    \qml
Column {
    Text {
        font.pointSize: 24
        text: "<b>Hello</b> <i>World!</i>"
    }
    Text {
        font.pointSize: 24
        textFormat: Text.RichText
        text: "<b>Hello</b> <i>World!</i>"
    }
    Text {
        font.pointSize: 24
        textFormat: Text.PlainText
        text: "<b>Hello</b> <i>World!</i>"
    }
}
    \endqml
    \o \image declarative-textformat.png
    \endtable
*/
QSGText::TextFormat QSGText::textFormat() const
{
    Q_D(const QSGText);
    return d->format;
}

void QSGText::setTextFormat(TextFormat format)
{
    Q_D(QSGText);
    if (format == d->format)
        return;
    d->format = format;
    bool wasRich = d->richText;
    d->richText = format == RichText;
    d->styledText = format == StyledText || (format == AutoText && Qt::mightBeRichText(d->text));

    if (!wasRich && d->richText && isComponentComplete()) {
        d->ensureDoc();
        d->doc->setText(d->text);
        d->richTextAsImage = enableImageCache();
    }

    d->updateLayout();

    emit textFormatChanged(d->format);
}

/*!
    \qmlproperty enumeration QtQuick2::Text::elide

    Set this property to elide parts of the text fit to the Text item's width.
    The text will only elide if an explicit width has been set.

    This property cannot be used with rich text.

    Eliding can be:
    \list
    \o Text.ElideNone  - the default
    \o Text.ElideLeft
    \o Text.ElideMiddle
    \o Text.ElideRight
    \endlist

    If this property is set to Text.ElideRight, it can be used with multiline
    text. The text will only elide if maximumLineCount has been set.

    If the text is a multi-length string, and the mode is not \c Text.ElideNone,
    the first string that fits will be used, otherwise the last will be elided.

    Multi-length strings are ordered from longest to shortest, separated by the
    Unicode "String Terminator" character \c U009C (write this in QML with \c{"\u009C"} or \c{"\x9C"}).
*/
QSGText::TextElideMode QSGText::elideMode() const
{
    Q_D(const QSGText);
    return d->elideMode;
}

void QSGText::setElideMode(QSGText::TextElideMode mode)
{
    Q_D(QSGText);
    if (mode == d->elideMode)
        return;

    d->elideMode = mode;
    d->updateLayout();

    emit elideModeChanged(d->elideMode);
}

/*! \internal */
QRectF QSGText::boundingRect() const
{
    Q_D(const QSGText);

    QRect rect = d->layedOutTextRect;
    if (d->style != Normal)
        rect.adjust(-1, 0, 1, 2);

    // Could include font max left/right bearings to either side of rectangle.

    int h = height();
    switch (d->vAlign) {
    case AlignTop:
        break;
    case AlignBottom:
        rect.moveTop(h - rect.height());
        break;
    case AlignVCenter:
        rect.moveTop((h - rect.height()) / 2);
        break;
    }

    return QRectF(rect);
}

/*! \internal */
void QSGText::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QSGText);
    if ((!d->internalWidthUpdate && newGeometry.width() != oldGeometry.width())
            && (d->wrapMode != QSGText::NoWrap
                || d->elideMode != QSGText::ElideNone
                || d->hAlign != QSGText::AlignLeft)) {
        if ((d->singleline || d->maximumLineCountValid) && d->elideMode != QSGText::ElideNone && widthValid()) {
            // We need to re-elide
            d->updateLayout();
        } else {
            // We just need to re-layout
            d->updateSize();
        }
    }

    QSGItem::geometryChanged(newGeometry, oldGeometry);
}

QSGNode *QSGText::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    Q_D(QSGText);

    if (d->text.isEmpty()) {
        delete oldNode;
        return 0;
    }

    QRectF bounds = boundingRect();

    // We need to make sure the layout is done in the current thread
#if defined(Q_OS_MAC)
    if (d->layoutThread != QThread::currentThread())
        d->updateLayout();
#endif

    // XXX todo - some styled text can be done by the QSGTextNode
    if (d->richTextAsImage || d->cacheAllTextAsImage || (qmlDisableDistanceField() && d->style != Normal)) {
        bool wasDirty = d->textureImageCacheDirty;
        d->textureImageCacheDirty = false;

        if (d->imageCache.isNull()) {
            delete oldNode;
            return 0;
        }

        QSGImageNode *node = 0;
        if (!oldNode || d->nodeType != QSGTextPrivate::NodeIsTexture) {
            delete oldNode;
            node = QSGItemPrivate::get(this)->sceneGraphContext()->createImageNode();
            d->texture = new QSGPlainTexture();
            wasDirty = true;
            d->nodeType = QSGTextPrivate::NodeIsTexture;
        } else {
            node = static_cast<QSGImageNode *>(oldNode);
            Q_ASSERT(d->texture);
        }

        if (wasDirty) {
            qobject_cast<QSGPlainTexture *>(d->texture)->setImage(d->imageCache.toImage());
            node->setTexture(0);
            node->setTexture(d->texture);
        }

        node->setTargetRect(QRectF(bounds.x(), bounds.y(), d->imageCache.width(), d->imageCache.height()));
        node->setSourceRect(QRectF(0, 0, 1, 1));
        node->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        node->setVerticalWrapMode(QSGTexture::ClampToEdge);
        node->setFiltering(QSGTexture::Linear); // Nonsmooth text just ugly, so don't do that..
        node->update();

        return node;

    } else {
        QSGTextNode *node = 0;
        if (!oldNode || d->nodeType != QSGTextPrivate::NodeIsText) {
            delete oldNode;
            node = new QSGTextNode(QSGItemPrivate::get(this)->sceneGraphContext());
            d->nodeType = QSGTextPrivate::NodeIsText;
        } else {
            node = static_cast<QSGTextNode *>(oldNode);
        }

        node->deleteContent();
        node->setMatrix(QMatrix4x4());

        if (d->richText) {

            d->ensureDoc();
            node->addTextDocument(bounds.topLeft(), d->doc, QColor(), d->style, d->styleColor);

        } else {
            node->addTextLayout(QPoint(0, bounds.y()), &d->layout, d->color, d->style, d->styleColor);
        }

        return node;
    }
}

bool QSGText::event(QEvent *e)
{
    Q_D(QSGText);
    if (e->type() == QEvent::User) {
        d->checkImageCache();
        return true;
    } else {
        return QSGImplicitSizeItem::event(e);
    }
}

/*!
    \qmlproperty real QtQuick2::Text::paintedWidth

    Returns the width of the text, including width past the width
    which is covered due to insufficient wrapping if WrapMode is set.
*/
qreal QSGText::paintedWidth() const
{
    Q_D(const QSGText);
    return d->paintedSize.width();
}

/*!
    \qmlproperty real QtQuick2::Text::paintedHeight

    Returns the height of the text, including height past the height
    which is covered due to there being more text than fits in the set height.
*/
qreal QSGText::paintedHeight() const
{
    Q_D(const QSGText);
    return d->paintedSize.height();
}

/*!
    \qmlproperty real QtQuick2::Text::lineHeight

    Sets the line height for the text.
    The value can be in pixels or a multiplier depending on lineHeightMode.

    The default value is a multiplier of 1.0.
    The line height must be a positive value.
*/
qreal QSGText::lineHeight() const
{
    Q_D(const QSGText);
    return d->lineHeight;
}

void QSGText::setLineHeight(qreal lineHeight)
{
    Q_D(QSGText);

    if ((d->lineHeight == lineHeight) || (lineHeight < 0.0))
        return;

    d->lineHeight = lineHeight;
    d->updateLayout();
    emit lineHeightChanged(lineHeight);
}

/*!
    \qmlproperty enumeration QtQuick2::Text::lineHeightMode

    This property determines how the line height is specified.
    The possible values are:

    \list
    \o Text.ProportionalHeight (default) - this sets the spacing proportional to the
       line (as a multiplier). For example, set to 2 for double spacing.
    \o Text.FixedHeight - this sets the line height to a fixed line height (in pixels).
    \endlist
*/
QSGText::LineHeightMode QSGText::lineHeightMode() const
{
    Q_D(const QSGText);
    return d->lineHeightMode;
}

void QSGText::setLineHeightMode(LineHeightMode mode)
{
    Q_D(QSGText);
    if (mode == d->lineHeightMode)
        return;

    d->lineHeightMode = mode;
    d->updateLayout();

    emit lineHeightModeChanged(mode);
}

/*!
    Returns the number of resources (images) that are being loaded asynchronously.
*/
int QSGText::resourcesLoading() const
{
    Q_D(const QSGText);
    return d->doc ? d->doc->resourcesLoading() : 0;
}

/*! \internal */
void QSGText::componentComplete()
{
    Q_D(QSGText);
    QSGItem::componentComplete();
    if (d->updateOnComponentComplete) {
        d->updateOnComponentComplete = false;
        if (d->richText) {
            d->ensureDoc();
            d->doc->setText(d->text);
            d->rightToLeftText = d->doc->toPlainText().isRightToLeft();
            d->richTextAsImage = enableImageCache();
        } else {
            d->rightToLeftText = d->text.isRightToLeft();
        }
        d->determineHorizontalAlignment();
        d->updateLayout();
    }
}


QString QSGTextPrivate::anchorAt(const QPointF &mousePos)
{
    if (styledText) {
        for (int i = 0; i < layout.lineCount(); ++i) {
            QTextLine line = layout.lineAt(i);
            if (line.naturalTextRect().contains(mousePos)) {
                int charPos = line.xToCursor(mousePos.x());
                foreach (const QTextLayout::FormatRange &formatRange, layout.additionalFormats()) {
                    if (formatRange.format.isAnchor()
                            && charPos >= formatRange.start
                            && charPos <= formatRange.start + formatRange.length) {
                        return formatRange.format.anchorHref();
                    }
                }
                break;
            }
        }
    }
    return QString();
}

bool QSGTextPrivate::isLinkActivatedConnected()
{
    static int idx = this->signalIndex("linkActivated(QString)");
    return this->isSignalConnected(idx);
}

/*!  \internal */
void QSGText::mousePressEvent(QMouseEvent *event)
{
    Q_D(QSGText);

    if (d->isLinkActivatedConnected()) {
        if (d->styledText)
            d->activeLink = d->anchorAt(event->localPos());
        else if (d->richText && d->doc)
            d->activeLink = d->doc->documentLayout()->anchorAt(event->localPos());
    }

    if (d->activeLink.isEmpty())
        event->setAccepted(false);

    // ### may malfunction if two of the same links are clicked & dragged onto each other)

    if (!event->isAccepted())
        QSGItem::mousePressEvent(event);

}

/*! \internal */
void QSGText::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QSGText);

    // ### confirm the link, and send a signal out

    QString link;
    if (d->isLinkActivatedConnected()) {
        if (d->styledText)
            link = d->anchorAt(event->localPos());
        else if (d->richText && d->doc)
            link = d->doc->documentLayout()->anchorAt(event->localPos());
    }

    if (!link.isEmpty() && d->activeLink == link)
        emit linkActivated(d->activeLink);
    else
        event->setAccepted(false);

    if (!event->isAccepted())
        QSGItem::mouseReleaseEvent(event);
}

QT_END_NAMESPACE

#include "qsgtext.moc"
