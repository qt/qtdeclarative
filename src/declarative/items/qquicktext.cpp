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

#include "qquicktext_p.h"
#include "qquicktext_p_p.h"

#include <private/qsgdistancefieldglyphcache_p.h>
#include <private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>
#include "qquicktextnode_p.h"
#include "qquickimage_p_p.h"
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

class QQuickTextDocumentWithImageResources : public QTextDocument {
    Q_OBJECT

public:
    QQuickTextDocumentWithImageResources(QQuickText *parent);
    virtual ~QQuickTextDocumentWithImageResources();

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

QString QQuickTextPrivate::elideChar = QString(0x2026);

QQuickTextPrivate::QQuickTextPrivate()
: color((QRgb)0), style(QQuickText::Normal), hAlign(QQuickText::AlignLeft),
  vAlign(QQuickText::AlignTop), elideMode(QQuickText::ElideNone),
  format(QQuickText::AutoText), wrapMode(QQuickText::NoWrap), lineHeight(1),
  lineHeightMode(QQuickText::ProportionalHeight), lineCount(1), maximumLineCount(INT_MAX),
  maximumLineCountValid(false),
  texture(0),
  imageCacheDirty(false), updateOnComponentComplete(true),
  richText(false), styledText(false), singleline(false), cacheAllTextAsImage(true), internalWidthUpdate(false),
  requireImplicitWidth(false), truncated(false), hAlignImplicit(true), rightToLeftText(false),
  layoutTextElided(false), richTextAsImage(false), textureImageCacheDirty(false), textHasChanged(true),
  naturalWidth(0), doc(0), textLine(0), nodeType(NodeIsNull)

#if defined(Q_OS_MAC)
, layoutThread(0), paintingThread(0)
#endif

{
    cacheAllTextAsImage = enableImageCache();
}

void QQuickTextPrivate::init()
{
    Q_Q(QQuickText);
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFlag(QQuickItem::ItemHasContents);
}

QQuickTextDocumentWithImageResources::QQuickTextDocumentWithImageResources(QQuickText *parent)
: QTextDocument(parent), outstanding(0)
{
    setUndoRedoEnabled(false);
}

QQuickTextDocumentWithImageResources::~QQuickTextDocumentWithImageResources()
{
    if (!m_resources.isEmpty())
        qDeleteAll(m_resources);
}

QVariant QQuickTextDocumentWithImageResources::loadResource(int type, const QUrl &name)
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

void QQuickTextDocumentWithImageResources::requestFinished()
{
    outstanding--;
    if (outstanding == 0) {
        QQuickText *textItem = static_cast<QQuickText*>(parent());
        QString text = textItem->text();
#ifndef QT_NO_TEXTHTMLPARSER
        setHtml(text);
#else
        setPlainText(text);
#endif
        QQuickTextPrivate *d = QQuickTextPrivate::get(textItem);
        d->updateLayout();
    }
}

void QQuickTextDocumentWithImageResources::setText(const QString &text)
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

QSet<QUrl> QQuickTextDocumentWithImageResources::errors;

QQuickTextPrivate::~QQuickTextPrivate()
{
    delete textLine; textLine = 0;
}

qreal QQuickTextPrivate::getImplicitWidth() const
{
    if (!requireImplicitWidth) {
        // We don't calculate implicitWidth unless it is required.
        // We need to force a size update now to ensure implicitWidth is calculated
        QQuickTextPrivate *me = const_cast<QQuickTextPrivate*>(this);
        me->requireImplicitWidth = true;
        me->updateSize();
    }
    return implicitWidth;
}

void QQuickTextPrivate::updateLayout()
{
    Q_Q(QQuickText);
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
            if (singleline && !maximumLineCountValid && elideMode != QQuickText::ElideNone && q->widthValid()) {
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
            if (textHasChanged) {
                QDeclarativeStyledText::parse(text, layout);
                textHasChanged = false;
            }
        }
    } else {
        ensureDoc();
        QTextBlockFormat::LineHeightTypes type;
        type = lineHeightMode == QQuickText::FixedHeight ? QTextBlockFormat::FixedHeight : QTextBlockFormat::ProportionalHeight;
        QTextBlockFormat blockFormat;
        blockFormat.setLineHeight((lineHeightMode == QQuickText::FixedHeight ? lineHeight : lineHeight * 100), type);
        for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next()) {
            QTextCursor cursor(it);
            cursor.mergeBlockFormat(blockFormat);
        }
    }

    updateSize();
}

void QQuickTextPrivate::updateSize()
{
    Q_Q(QQuickText);

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
        QQuickText::HAlignment horizontalAlignment = q->effectiveHAlign();
        if (rightToLeftText) {
            if (horizontalAlignment == QQuickText::AlignLeft)
                horizontalAlignment = QQuickText::AlignRight;
            else if (horizontalAlignment == QQuickText::AlignRight)
                horizontalAlignment = QQuickText::AlignLeft;
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
        if (wrapMode != QQuickText::NoWrap && q->widthValid())
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
        if (vAlign == QQuickText::AlignBottom)
            yoff = dy;
        else if (vAlign == QQuickText::AlignVCenter)
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

QQuickTextLine::QQuickTextLine()
    : QObject(), m_line(0), m_height(0)
{
}

void QQuickTextLine::setLine(QTextLine *line)
{
    m_line = line;
}

int QQuickTextLine::number() const
{
    if (m_line)
        return m_line->lineNumber();
    return 0;
}

qreal QQuickTextLine::width() const
{
    if (m_line)
        return m_line->width();
    return 0;
}

void QQuickTextLine::setWidth(qreal width)
{
    if (m_line)
        m_line->setLineWidth(width);
}

qreal QQuickTextLine::height() const
{
    if (m_height)
        return m_height;
    if (m_line)
        return m_line->height();
    return 0;
}

void QQuickTextLine::setHeight(qreal height)
{
    if (m_line)
        m_line->setPosition(QPointF(m_line->x(), m_line->y() - m_line->height() + height));
    m_height = height;
}

qreal QQuickTextLine::x() const
{
    if (m_line)
        return m_line->x();
    return 0;
}

void QQuickTextLine::setX(qreal x)
{
    if (m_line)
        m_line->setPosition(QPointF(x, m_line->y()));
}

qreal QQuickTextLine::y() const
{
    if (m_line)
        return m_line->y();
    return 0;
}

void QQuickTextLine::setY(qreal y)
{
    if (m_line)
        m_line->setPosition(QPointF(m_line->x(), y));
}

void QQuickText::doLayout()
{
    Q_D(QQuickText);
    d->updateSize();
}

bool QQuickTextPrivate::isLineLaidOutConnected()
{
    static int idx = this->signalIndex("lineLaidOut(QQuickTextLine*)");
    return this->isSignalConnected(idx);
}

void QQuickTextPrivate::setupCustomLineGeometry(QTextLine &line, qreal &height, qreal elideWidth = 0)
{
    Q_Q(QQuickText);

#if defined(Q_OS_MAC)
    if (QThread::currentThread() != paintingThread) {
#endif
        if (!line.lineNumber())
            linesRects.clear();

        if (!textLine)
            textLine = new QQuickTextLine;
        textLine->setLine(&line);
        textLine->setY(height);
        textLine->setHeight(0);

        // use the text item's width by default if it has one and wrap is on
        if (q->widthValid() && q->wrapMode() != QQuickText::NoWrap)
            textLine->setWidth(q->width() - elideWidth);
        else
            textLine->setWidth(INT_MAX);
        if (lineHeight != 1.0)
            textLine->setHeight((lineHeightMode == QQuickText::FixedHeight) ? lineHeight : line.height() * lineHeight);

        emit q->lineLaidOut(textLine);

        linesRects << QRectF(textLine->x(), textLine->y(), textLine->width(), textLine->height());
        height += textLine->height();

#if defined(Q_OS_MAC)
    } else {
        if (line.lineNumber() < linesRects.count()) {
            QRectF r = linesRects.at(line.lineNumber());
            line.setLineWidth(r.width());
            line.setPosition(r.topLeft());
        }
    }
#endif
}

/*!
    Lays out the QQuickTextPrivate::layout QTextLayout in the constraints of the QQuickText.

    Returns the size of the final text.  This can be used to position the text vertically (the text is
    already absolutely positioned horizontally).
*/
QRect QQuickTextPrivate::setupTextLayout()
{
    // ### text layout handling should be profiled and optimized as needed
    // what about QStackTextEngine engine(tmp, d->font.font()); QTextLayout textLayout(&engine);
    Q_Q(QQuickText);
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

    qreal height = 0;
    bool customLayout = isLineLaidOutConnected();

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

            if (customLayout)
                setupCustomLineGeometry(line, height);
            else if (lineWidth)
                line.setLineWidth(lineWidth);
            visibleTextLength += line.textLength();

            if (--linesLeft == 0) {
                if (visibleTextLength < text.length()) {
                    truncate = true;
                    if (elideMode == QQuickText::ElideRight && q->widthValid()) {
                        qreal elideWidth = fm.width(elideChar);
                        // Need to correct for alignment
                        if (customLayout)
                            setupCustomLineGeometry(line, height, elideWidth);
                        else
                            line.setLineWidth(lineWidth - elideWidth);
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
            if (customLayout)
                setupCustomLineGeometry(line, height);
            else {
                if (lineWidth)
                    line.setLineWidth(lineWidth);
            }
        }
        layout.endLayout();
    }

    height = 0;
    QRectF br;
    for (int i = 0; i < layout.lineCount(); ++i) {
        QTextLine line = layout.lineAt(i);
        // set line spacing
        if (!customLayout)
            line.setPosition(QPointF(line.position().x(), height));
        if (elideText && i == layout.lineCount()-1) {
            elidePos.setY(height + fm.ascent());
            br = br.united(QRectF(elidePos, QSizeF(fm.width(elideChar), fm.ascent())));
        }
        br = br.united(line.naturalTextRect());
        height += (lineHeightMode == QQuickText::FixedHeight) ? lineHeight : line.height() * lineHeight;
    }
    if (!customLayout)
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
    Returns a painted version of the QQuickTextPrivate::layout QTextLayout.
    If \a drawStyle is true, the style color overrides all colors in the document.
*/
QPixmap QQuickTextPrivate::textLayoutImage(bool drawStyle)
{
    QSize size = layedOutTextRect.size();

    //paint text
    QPixmap img(size);
    if (!size.isEmpty()) {
        img.fill(Qt::transparent);
/*#ifdef Q_OS_MAC // Fails on CocoaX64
        bool oldSmooth = qt_applefontsmoothing_enabled;
        qt_applefontsmoothing_enabled = false;
#endif*/
        QPainter p(&img);
/*#ifdef Q_OS_MAC // Fails on CocoaX64
        qt_applefontsmoothing_enabled = oldSmooth;
#endif*/
        drawTextLayout(&p, QPointF(-layedOutTextRect.x(),0), drawStyle);
    }
    return img;
}

/*!
    Paints the QQuickTextPrivate::layout QTextLayout into \a painter at \a pos.  If
    \a drawStyle is true, the style color overrides all colors in the document.
*/
void QQuickTextPrivate::drawTextLayout(QPainter *painter, const QPointF &pos, bool drawStyle)
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
    Returns a painted version of the QQuickTextPrivate::doc QTextDocument.
    If \a drawStyle is true, the style color overrides all colors in the document.
*/
QPixmap QQuickTextPrivate::textDocumentImage(bool drawStyle)
{
    QSize size = doc->size().toSize();

    //paint text
    QPixmap img(size);
    img.fill(Qt::transparent);
/*#ifdef Q_OS_MAC // Fails on CocoaX64
    bool oldSmooth = qt_applefontsmoothing_enabled;
    qt_applefontsmoothing_enabled = false;
#endif*/
    QPainter p(&img);
/*#ifdef Q_OS_MAC // Fails on CocoaX64
    qt_applefontsmoothing_enabled = oldSmooth;
#endif*/

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
void QQuickTextPrivate::invalidateImageCache()
{
    Q_Q(QQuickText);

    if (richTextAsImage || cacheAllTextAsImage || (qmlDisableDistanceField() && style != QQuickText::Normal)) { // If actually using the image cache
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
void QQuickTextPrivate::checkImageCache()
{
    Q_Q(QQuickText);

    if (!imageCacheDirty)
        return;

    if (text.isEmpty()) {

        imageCache = QPixmap();

    } else {

        QPixmap textImage;
        QPixmap styledImage;

        if (richText) {
            textImage = textDocumentImage(false);
            if (style != QQuickText::Normal)
                styledImage = textDocumentImage(true); //### should use styleColor
        } else {
            textImage = textLayoutImage(false);
            if (style != QQuickText::Normal)
                styledImage = textLayoutImage(true); //### should use styleColor
        }

        switch (style) {
        case QQuickText::Outline:
            imageCache = drawOutline(textImage, styledImage);
            break;
        case QQuickText::Sunken:
            imageCache = drawOutline(textImage, styledImage, -1);
            break;
        case QQuickText::Raised:
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
    Ensures the QQuickTextPrivate::doc variable is set to a valid text document
*/
void QQuickTextPrivate::ensureDoc()
{
    if (!doc) {
        Q_Q(QQuickText);
        doc = new QQuickTextDocumentWithImageResources(q);
        doc->setDocumentMargin(0);
    }
}

/*!
    Draw \a styleSource as an outline around \a source and return the new image.
*/
QPixmap QQuickTextPrivate::drawOutline(const QPixmap &source, const QPixmap &styleSource)
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
QPixmap QQuickTextPrivate::drawOutline(const QPixmap &source, const QPixmap &styleSource, int yOffset)
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
    \qmlclass Text QQuickText
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
QQuickText::QQuickText(QQuickItem *parent)
: QQuickImplicitSizeItem(*(new QQuickTextPrivate), parent)
{
    Q_D(QQuickText);
    d->init();
}

QQuickText::~QQuickText()
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
    \qmlsignal QtQuick2::Text::onLineLaidOut(line)

    This handler is called for every line during the layout process.
    This gives the opportunity to position and resize a line as it is being laid out.
    It can for example be used to create columns or lay out text around objects.

    The properties of a line are:
    \list
    \o number (read-only)
    \o x
    \o y
    \o width
    \o height
    \endlist

    For example, this will move the first 5 lines of a text element by 100 pixels to the right:
    \code
    onLineLaidOut: {
        if (line.number < 5) {
            line.x = line.x + 100
            line.width = line.width - 100
        }
    }
    \endcode
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
    \o Font.AllLowercase - This alters the text to be rendered in all lowercase type.
    \o Font.SmallCaps - This alters the text to be rendered in small-caps type.
    \o Font.Capitalize - This alters the text to be rendered with the first character of each word as an uppercase character.
    \endlist

    \qml
    Text { text: "Hello"; font.capitalization: Font.AllLowercase }
    \endqml
*/
QFont QQuickText::font() const
{
    Q_D(const QQuickText);
    return d->sourceFont;
}

void QQuickText::setFont(const QFont &font)
{
    Q_D(QQuickText);
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
QString QQuickText::text() const
{
    Q_D(const QQuickText);
    return d->text;
}

void QQuickText::setText(const QString &n)
{
    Q_D(QQuickText);
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
    d->textHasChanged = true;
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
QColor QQuickText::color() const
{
    Q_D(const QQuickText);
    return d->color;
}

void QQuickText::setColor(const QColor &color)
{
    Q_D(QQuickText);
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
QQuickText::TextStyle QQuickText::style() const
{
    Q_D(const QQuickText);
    return d->style;
}

void QQuickText::setStyle(QQuickText::TextStyle style)
{
    Q_D(QQuickText);
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
QColor QQuickText::styleColor() const
{
    Q_D(const QQuickText);
    return d->styleColor;
}

void QQuickText::setStyleColor(const QColor &color)
{
    Q_D(QQuickText);
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
QQuickText::HAlignment QQuickText::hAlign() const
{
    Q_D(const QQuickText);
    return d->hAlign;
}

void QQuickText::setHAlign(HAlignment align)
{
    Q_D(QQuickText);
    bool forceAlign = d->hAlignImplicit && d->effectiveLayoutMirror;
    d->hAlignImplicit = false;
    if (d->setHAlign(align, forceAlign) && isComponentComplete())
        d->updateLayout();
}

void QQuickText::resetHAlign()
{
    Q_D(QQuickText);
    d->hAlignImplicit = true;
    if (d->determineHorizontalAlignment() && isComponentComplete())
        d->updateLayout();
}

QQuickText::HAlignment QQuickText::effectiveHAlign() const
{
    Q_D(const QQuickText);
    QQuickText::HAlignment effectiveAlignment = d->hAlign;
    if (!d->hAlignImplicit && d->effectiveLayoutMirror) {
        switch (d->hAlign) {
        case QQuickText::AlignLeft:
            effectiveAlignment = QQuickText::AlignRight;
            break;
        case QQuickText::AlignRight:
            effectiveAlignment = QQuickText::AlignLeft;
            break;
        default:
            break;
        }
    }
    return effectiveAlignment;
}

bool QQuickTextPrivate::setHAlign(QQuickText::HAlignment alignment, bool forceAlign)
{
    Q_Q(QQuickText);
    if (hAlign != alignment || forceAlign) {
        QQuickText::HAlignment oldEffectiveHAlign = q->effectiveHAlign();
        hAlign = alignment;

        emit q->horizontalAlignmentChanged(hAlign);
        if (oldEffectiveHAlign != q->effectiveHAlign())
            emit q->effectiveHorizontalAlignmentChanged();
        return true;
    }
    return false;
}

bool QQuickTextPrivate::determineHorizontalAlignment()
{
    Q_Q(QQuickText);
    if (hAlignImplicit && q->isComponentComplete()) {
        bool alignToRight = text.isEmpty() ? QGuiApplication::keyboardInputDirection() == Qt::RightToLeft : rightToLeftText;
        return setHAlign(alignToRight ? QQuickText::AlignRight : QQuickText::AlignLeft);
    }
    return false;
}

void QQuickTextPrivate::mirrorChange()
{
    Q_Q(QQuickText);
    if (q->isComponentComplete()) {
        if (!hAlignImplicit && (hAlign == QQuickText::AlignRight || hAlign == QQuickText::AlignLeft)) {
            updateLayout();
            emit q->effectiveHorizontalAlignmentChanged();
        }
    }
}

QTextDocument *QQuickTextPrivate::textDocument()
{
    return doc;
}

QQuickText::VAlignment QQuickText::vAlign() const
{
    Q_D(const QQuickText);
    return d->vAlign;
}

void QQuickText::setVAlign(VAlignment align)
{
    Q_D(QQuickText);
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
QQuickText::WrapMode QQuickText::wrapMode() const
{
    Q_D(const QQuickText);
    return d->wrapMode;
}

void QQuickText::setWrapMode(WrapMode mode)
{
    Q_D(QQuickText);
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
int QQuickText::lineCount() const
{
    Q_D(const QQuickText);
    return d->lineCount;
}

/*!
    \qmlproperty bool QtQuick2::Text::truncated

    Returns true if the text has been truncated due to \l maximumLineCount
    or \l elide.

    This property is not supported for rich text.

    \sa maximumLineCount, elide
*/
bool QQuickText::truncated() const
{
    Q_D(const QQuickText);
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
int QQuickText::maximumLineCount() const
{
    Q_D(const QQuickText);
    return d->maximumLineCount;
}

void QQuickText::setMaximumLineCount(int lines)
{
    Q_D(QQuickText);

    d->maximumLineCountValid = lines==INT_MAX ? false : true;
    if (d->maximumLineCount != lines) {
        d->maximumLineCount = lines;
        d->updateLayout();
        emit maximumLineCountChanged();
    }
}

void QQuickText::resetMaximumLineCount()
{
    Q_D(QQuickText);
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
QQuickText::TextFormat QQuickText::textFormat() const
{
    Q_D(const QQuickText);
    return d->format;
}

void QQuickText::setTextFormat(TextFormat format)
{
    Q_D(QQuickText);
    if (format == d->format)
        return;
    d->format = format;
    bool wasRich = d->richText;
    d->richText = format == RichText;
    d->styledText = format == StyledText || (format == AutoText && Qt::mightBeRichText(d->text));

    if (!wasRich && d->richText && isComponentComplete()) {
        d->ensureDoc();
        d->doc->setText(d->text);
        d->rightToLeftText = d->doc->toPlainText().isRightToLeft();
        d->richTextAsImage = enableImageCache();
    } else {
        d->rightToLeftText = d->text.isRightToLeft();
    }
    d->determineHorizontalAlignment();
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
QQuickText::TextElideMode QQuickText::elideMode() const
{
    Q_D(const QQuickText);
    return d->elideMode;
}

void QQuickText::setElideMode(QQuickText::TextElideMode mode)
{
    Q_D(QQuickText);
    if (mode == d->elideMode)
        return;

    d->elideMode = mode;
    d->updateLayout();

    emit elideModeChanged(d->elideMode);
}

/*! \internal */
QRectF QQuickText::boundingRect() const
{
    Q_D(const QQuickText);

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
void QQuickText::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickText);
    if ((!d->internalWidthUpdate && newGeometry.width() != oldGeometry.width())
            && (d->wrapMode != QQuickText::NoWrap
                || d->elideMode != QQuickText::ElideNone
                || d->hAlign != QQuickText::AlignLeft)) {
        if ((d->singleline || d->maximumLineCountValid) && d->elideMode != QQuickText::ElideNone && widthValid()) {
            // We need to re-elide
            d->updateLayout();
        } else {
            // We just need to re-layout
            d->updateSize();
        }
    }

    QQuickItem::geometryChanged(newGeometry, oldGeometry);
}

QSGNode *QQuickText::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    Q_D(QQuickText);

    if (d->text.isEmpty()) {
        delete oldNode;
        return 0;
    }

    QRectF bounds = boundingRect();

    // We need to make sure the layout is done in the current thread
#if defined(Q_OS_MAC)
    d->paintingThread = QThread::currentThread();
    if (d->layoutThread != d->paintingThread)
        d->updateLayout();
#endif

    // XXX todo - some styled text can be done by the QQuickTextNode
    if (d->richTextAsImage || d->cacheAllTextAsImage || (qmlDisableDistanceField() && d->style != Normal)) {
        bool wasDirty = d->textureImageCacheDirty;
        d->textureImageCacheDirty = false;

        if (d->imageCache.isNull()) {
            delete oldNode;
            return 0;
        }

        QSGImageNode *node = 0;
        if (!oldNode || d->nodeType != QQuickTextPrivate::NodeIsTexture) {
            delete oldNode;
            node = QQuickItemPrivate::get(this)->sceneGraphContext()->createImageNode();
            d->texture = new QSGPlainTexture();
            wasDirty = true;
            d->nodeType = QQuickTextPrivate::NodeIsTexture;
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
        QQuickTextNode *node = 0;
        if (!oldNode || d->nodeType != QQuickTextPrivate::NodeIsText) {
            delete oldNode;
            node = new QQuickTextNode(QQuickItemPrivate::get(this)->sceneGraphContext());
            d->nodeType = QQuickTextPrivate::NodeIsText;
        } else {
            node = static_cast<QQuickTextNode *>(oldNode);
        }

        node->deleteContent();
        node->setMatrix(QMatrix4x4());

        if (d->richText) {
            d->ensureDoc();
            node->addTextDocument(bounds.topLeft(), d->doc, d->color, d->style, d->styleColor);

        } else {
            node->addTextLayout(QPoint(0, bounds.y()), &d->layout, d->color, d->style, d->styleColor);
        }

        return node;
    }
}

bool QQuickText::event(QEvent *e)
{
    Q_D(QQuickText);
    if (e->type() == QEvent::User) {
        d->checkImageCache();
        return true;
    } else {
        return QQuickImplicitSizeItem::event(e);
    }
}

/*!
    \qmlproperty real QtQuick2::Text::paintedWidth

    Returns the width of the text, including width past the width
    which is covered due to insufficient wrapping if WrapMode is set.
*/
qreal QQuickText::paintedWidth() const
{
    Q_D(const QQuickText);
    return d->paintedSize.width();
}

/*!
    \qmlproperty real QtQuick2::Text::paintedHeight

    Returns the height of the text, including height past the height
    which is covered due to there being more text than fits in the set height.
*/
qreal QQuickText::paintedHeight() const
{
    Q_D(const QQuickText);
    return d->paintedSize.height();
}

/*!
    \qmlproperty real QtQuick2::Text::lineHeight

    Sets the line height for the text.
    The value can be in pixels or a multiplier depending on lineHeightMode.

    The default value is a multiplier of 1.0.
    The line height must be a positive value.
*/
qreal QQuickText::lineHeight() const
{
    Q_D(const QQuickText);
    return d->lineHeight;
}

void QQuickText::setLineHeight(qreal lineHeight)
{
    Q_D(QQuickText);

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
QQuickText::LineHeightMode QQuickText::lineHeightMode() const
{
    Q_D(const QQuickText);
    return d->lineHeightMode;
}

void QQuickText::setLineHeightMode(LineHeightMode mode)
{
    Q_D(QQuickText);
    if (mode == d->lineHeightMode)
        return;

    d->lineHeightMode = mode;
    d->updateLayout();

    emit lineHeightModeChanged(mode);
}

/*!
    Returns the number of resources (images) that are being loaded asynchronously.
*/
int QQuickText::resourcesLoading() const
{
    Q_D(const QQuickText);
    return d->doc ? d->doc->resourcesLoading() : 0;
}

/*! \internal */
void QQuickText::componentComplete()
{
    Q_D(QQuickText);
    QQuickItem::componentComplete();
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


QString QQuickTextPrivate::anchorAt(const QPointF &mousePos)
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

bool QQuickTextPrivate::isLinkActivatedConnected()
{
    static int idx = this->signalIndex("linkActivated(QString)");
    return this->isSignalConnected(idx);
}

/*!  \internal */
void QQuickText::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickText);

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
        QQuickItem::mousePressEvent(event);

}

/*! \internal */
void QQuickText::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickText);

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
        QQuickItem::mouseReleaseEvent(event);
}

QT_END_NAMESPACE

#include "qquicktext.moc"
