/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qquicktext_p.h"
#include "qquicktext_p_p.h"

#include <QtQuick/private/qsgcontext_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qsgadaptationlayer_p.h>
#include "qquicktextnode_p.h"
#include "qquickimage_p_p.h"
#include <QtQuick/private/qsgtexture_p.h>

#include <QtQml/qqmlinfo.h>
#include <QtGui/qevent.h>
#include <QtGui/qabstracttextdocumentlayout.h>
#include <QtGui/qpainter.h>
#include <QtGui/qtextdocument.h>
#include <QtGui/qtextobject.h>
#include <QtGui/qtextcursor.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qinputmethod.h>

#include <private/qtextengine_p.h>
#include <private/qquickstyledtext_p.h>
#include <QtQuick/private/qquickpixmapcache_p.h>

#include <qmath.h>
#include <limits.h>

QT_BEGIN_NAMESPACE


const QChar QQuickTextPrivate::elideChar = QChar(0x2026);

QQuickTextPrivate::QQuickTextPrivate()
    : elideLayout(0), textLine(0)
#if defined(Q_OS_MAC)
    , layoutThread(0), paintingThread(0)
#endif
    , color(0xFF000000), linkColor(0xFF0000FF), styleColor(0xFF000000)
    , lineCount(1), multilengthEos(-1)
    , elideMode(QQuickText::ElideNone), hAlign(QQuickText::AlignLeft), vAlign(QQuickText::AlignTop)
    , format(QQuickText::AutoText), wrapMode(QQuickText::NoWrap)
    , style(QQuickText::Normal)
    , updateType(UpdatePaintNode)
    , maximumLineCountValid(false), updateOnComponentComplete(true), richText(false)
    , styledText(false), singleline(false), internalWidthUpdate(false), requireImplicitWidth(false)
    , truncated(false), hAlignImplicit(true), rightToLeftText(false)
    , layoutTextElided(false), textHasChanged(true), needToUpdateLayout(false), formatModifiesFontSize(false)
{
}

QQuickTextPrivate::ExtraData::ExtraData()
    : lineHeight(1.0)
    , doc(0)
    , minimumPixelSize(12)
    , minimumPointSize(12)
    , nbActiveDownloads(0)
    , maximumLineCount(INT_MAX)
    , lineHeightMode(QQuickText::ProportionalHeight)
    , fontSizeMode(QQuickText::FixedSize)
{
}

void QQuickTextPrivate::init()
{
    Q_Q(QQuickText);
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFlag(QQuickItem::ItemHasContents);
}

QQuickTextDocumentWithImageResources::QQuickTextDocumentWithImageResources(QQuickItem *parent)
: QTextDocument(parent), outstanding(0)
{
    setUndoRedoEnabled(false);
    documentLayout()->registerHandler(QTextFormat::ImageObject, this);
}

QQuickTextDocumentWithImageResources::~QQuickTextDocumentWithImageResources()
{
    if (!m_resources.isEmpty())
        qDeleteAll(m_resources);
}

QVariant QQuickTextDocumentWithImageResources::loadResource(int type, const QUrl &name)
{
    QQmlContext *context = qmlContext(parent());
    QUrl url = m_baseUrl.resolved(name);

    if (type == QTextDocument::ImageResource) {
        QQuickPixmap *p = loadPixmap(context, url);
        return p->image();
    }

    return QTextDocument::loadResource(type,url); // The *resolved* URL
}

void QQuickTextDocumentWithImageResources::requestFinished()
{
    outstanding--;
    if (outstanding == 0) {
        markContentsDirty(0, characterCount());
        emit imagesLoaded();
    }
}

void QQuickTextDocumentWithImageResources::clear()
{
    clearResources();

    QTextDocument::clear();
}


QSizeF QQuickTextDocumentWithImageResources::intrinsicSize(
        QTextDocument *, int, const QTextFormat &format)
{
    if (format.isImageFormat()) {
        QTextImageFormat imageFormat = format.toImageFormat();

        const bool hasWidth = imageFormat.hasProperty(QTextFormat::ImageWidth);
        const int width = qRound(imageFormat.width());
        const bool hasHeight = imageFormat.hasProperty(QTextFormat::ImageHeight);
        const int height = qRound(imageFormat.height());

        QSizeF size(width, height);
        if (!hasWidth || !hasHeight) {
            QQmlContext *context = qmlContext(parent());
            QUrl url = m_baseUrl.resolved(QUrl(imageFormat.name()));

            QQuickPixmap *p = loadPixmap(context, url);
            if (!p->isReady()) {
                if (!hasWidth)
                    size.setWidth(16);
                if (!hasHeight)
                    size.setHeight(16);
                return size;
            }
            QSize implicitSize = p->implicitSize();

            if (!hasWidth) {
                if (!hasHeight)
                    size.setWidth(implicitSize.width());
                else
                    size.setWidth(qRound(height * (implicitSize.width() / (qreal) implicitSize.height())));
            }
            if (!hasHeight) {
                if (!hasWidth)
                    size.setHeight(implicitSize.height());
                else
                    size.setHeight(qRound(width * (implicitSize.height() / (qreal) implicitSize.width())));
            }
        }
        return size;
    }
    return QSizeF();
}

void QQuickTextDocumentWithImageResources::drawObject(
        QPainter *, const QRectF &, QTextDocument *, int, const QTextFormat &)
{
}

QImage QQuickTextDocumentWithImageResources::image(const QTextImageFormat &format)
{
    QQmlContext *context = qmlContext(parent());
    QUrl url = m_baseUrl.resolved(QUrl(format.name()));

    QQuickPixmap *p = loadPixmap(context, url);
    return p->image();
}

void QQuickTextDocumentWithImageResources::setBaseUrl(const QUrl &url, bool clear)
{
    m_baseUrl = url;
    if (clear) {
        clearResources();
        markContentsDirty(0, characterCount());
    }
}

QQuickPixmap *QQuickTextDocumentWithImageResources::loadPixmap(
        QQmlContext *context, const QUrl &url)
{

    QHash<QUrl, QQuickPixmap *>::Iterator iter = m_resources.find(url);

    if (iter == m_resources.end()) {
        QQuickPixmap *p = new QQuickPixmap(context->engine(), url);
        iter = m_resources.insert(url, p);

        if (p->isLoading()) {
            p->connectFinished(this, SLOT(requestFinished()));
            outstanding++;
        }
    }

    QQuickPixmap *p = *iter;
    if (p->isError()) {
        if (!errors.contains(url)) {
            errors.insert(url);
            qmlInfo(parent()) << p->error();
        }
    }
    return p;
}

void QQuickTextDocumentWithImageResources::clearResources()
{
    foreach (QQuickPixmap *pixmap, m_resources)
        pixmap->clear(this);
    qDeleteAll(m_resources);
    m_resources.clear();
    outstanding = 0;
}

void QQuickTextDocumentWithImageResources::setText(const QString &text)
{
    clearResources();

#ifndef QT_NO_TEXTHTMLPARSER
    setHtml(text);
#else
    setPlainText(text);
#endif
}

QSet<QUrl> QQuickTextDocumentWithImageResources::errors;

QQuickTextPrivate::~QQuickTextPrivate()
{
    delete elideLayout;
    delete textLine; textLine = 0;
    qDeleteAll(imgTags);
    imgTags.clear();
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

void QQuickText::q_imagesLoaded()
{
    Q_D(QQuickText);
    d->updateLayout();
}

void QQuickTextPrivate::updateLayout()
{
    Q_Q(QQuickText);
    if (!q->isComponentComplete()) {
        updateOnComponentComplete = true;
        return;
    }
    updateOnComponentComplete = false;
    layoutTextElided = false;

    if (!visibleImgTags.isEmpty())
        visibleImgTags.clear();
    needToUpdateLayout = false;

    // Setup instance of QTextLayout for all cases other than richtext
    if (!richText) {
        if (textHasChanged) {
            if (styledText && !text.isEmpty()) {
                layout.setFont(font);
                // needs temporary bool because formatModifiesFontSize is in a bit-field
                bool fontSizeModified = false;
                QQuickStyledText::parse(text, layout, imgTags, q->baseUrl(), qmlContext(q), !maximumLineCountValid, &fontSizeModified);
                formatModifiesFontSize = fontSizeModified;
            } else {
                layout.clearAdditionalFormats();
                QString tmp = text;
                multilengthEos = tmp.indexOf(QLatin1Char('\x9c'));
                if (multilengthEos != -1) {
                    tmp = tmp.mid(0, multilengthEos);
                    tmp.replace(QLatin1Char('\n'), QChar::LineSeparator);
                } else if (tmp.contains(QLatin1Char('\n'))) {
                    // Replace always does a detach.  Checking for the new line character first
                    // means iterating over those items again if found but prevents a realloc
                    // otherwise.
                    tmp.replace(QLatin1Char('\n'), QChar::LineSeparator);
                }
                layout.setText(tmp);
            }
            textHasChanged = false;
        }
    } else {
        ensureDoc();
        QTextBlockFormat::LineHeightTypes type;
        type = lineHeightMode() == QQuickText::FixedHeight ? QTextBlockFormat::FixedHeight : QTextBlockFormat::ProportionalHeight;
        QTextBlockFormat blockFormat;
        blockFormat.setLineHeight((lineHeightMode() == QQuickText::FixedHeight ? lineHeight() : lineHeight() * 100), type);
        for (QTextBlock it = extra->doc->begin(); it != extra->doc->end(); it = it.next()) {
            QTextCursor cursor(it);
            cursor.mergeBlockFormat(blockFormat);
        }
    }

    updateSize();

    if (needToUpdateLayout) {
        needToUpdateLayout = false;
        textHasChanged = true;
        updateLayout();
    }
}

void QQuickText::imageDownloadFinished()
{
    Q_D(QQuickText);

    (d->extra->nbActiveDownloads)--;

    // when all the remote images have been downloaded,
    // if one of the sizes was not specified at parsing time
    // we use the implicit size from pixmapcache and re-layout.

    if (d->extra.isAllocated() && d->extra->nbActiveDownloads == 0) {
        bool needToUpdateLayout = false;
        foreach (QQuickStyledTextImgTag *img, d->visibleImgTags) {
            if (!img->size.isValid()) {
                img->size = img->pix->implicitSize();
                needToUpdateLayout = true;
            }
        }

        if (needToUpdateLayout) {
            d->textHasChanged = true;
            d->updateLayout();
        } else {
            d->updateType = QQuickTextPrivate::UpdatePaintNode;
            update();
        }
    }
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

    QFontMetricsF fm(font);
    if (text.isEmpty()) {
        qreal fontHeight = fm.height();
        q->setImplicitSize(0, fontHeight);
        layedOutTextRect = QRect(0, 0, 0, fontHeight);
        emit q->contentSizeChanged();
        updateType = UpdatePaintNode;
        q->update();
        return;
    }

    qreal naturalWidth = 0;

    qreal dy = q->height();
    QSizeF size(0, 0);
    QSizeF previousSize = layedOutTextRect.size();
#if defined(Q_OS_MAC)
    layoutThread = QThread::currentThread();
#endif

    //setup instance of QTextLayout for all cases other than richtext
    if (!richText) {
        QRectF textRect = setupTextLayout(&naturalWidth);
        layedOutTextRect = textRect;
        size = textRect.size();
        dy -= size.height();
    } else {
        singleline = false; // richtext can't elide or be optimized for single-line case
        ensureDoc();
        extra->doc->setDefaultFont(font);
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
        option.setUseDesignMetrics(true);
        extra->doc->setDefaultTextOption(option);
        if (requireImplicitWidth && q->widthValid()) {
            extra->doc->setTextWidth(-1);
            naturalWidth = extra->doc->idealWidth();
        }
        if (wrapMode != QQuickText::NoWrap && q->widthValid())
            extra->doc->setTextWidth(q->width());
        else
            extra->doc->setTextWidth(extra->doc->idealWidth()); // ### Text does not align if width is not set (QTextDoc bug)
        dy -= extra->doc->size().height();
        QSizeF dsize = extra->doc->size();
        layedOutTextRect = QRectF(QPointF(0,0), dsize);
        size = QSizeF(extra->doc->idealWidth(),dsize.height());
    }
    qreal yoff = 0;

    if (q->heightValid()) {
        if (vAlign == QQuickText::AlignBottom)
            yoff = dy;
        else if (vAlign == QQuickText::AlignVCenter)
            yoff = dy/2;
    }
    q->setBaselineOffset(fm.ascent() + yoff);

    //### need to comfirm cost of always setting these for richText
    internalWidthUpdate = true;
    qreal iWidth = -1;
    if (!q->widthValid())
        iWidth = size.width();
    else if (requireImplicitWidth)
        iWidth = naturalWidth;
    if (iWidth > -1)
        q->setImplicitSize(iWidth, size.height());
    internalWidthUpdate = false;

    if (iWidth == -1)
        q->setImplicitHeight(size.height());
    if (layedOutTextRect.size() != previousSize)
        emit q->contentSizeChanged();
    updateType = UpdatePaintNode;
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

void QQuickTextLine::setLineOffset(int offset)
{
    m_lineOffset = offset;
}

int QQuickTextLine::number() const
{
    if (m_line)
        return m_line->lineNumber() + m_lineOffset;
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

void QQuickTextPrivate::setupCustomLineGeometry(QTextLine &line, qreal &height, int lineOffset)
{
    Q_Q(QQuickText);

#if defined(Q_OS_MAC)
    if (QThread::currentThread() != paintingThread) {
        if (!line.lineNumber())
            linesRects.clear();
#endif

        if (!textLine)
            textLine = new QQuickTextLine;
        textLine->setLine(&line);
        textLine->setY(height);
        textLine->setHeight(0);
        textLine->setLineOffset(lineOffset);

        // use the text item's width by default if it has one and wrap is on
        if (q->widthValid() && q->wrapMode() != QQuickText::NoWrap)
            textLine->setWidth(q->width());
        else
            textLine->setWidth(INT_MAX);
        if (lineHeight() != 1.0)
            textLine->setHeight((lineHeightMode() == QQuickText::FixedHeight) ? lineHeight() : line.height() * lineHeight());

        emit q->lineLaidOut(textLine);

        height += textLine->height();

#if defined(Q_OS_MAC)
        linesRects << QRectF(textLine->x(), textLine->y(), textLine->width(), textLine->height());

    } else {
        if (line.lineNumber() < linesRects.count()) {
            QRectF r = linesRects.at(line.lineNumber());
            line.setLineWidth(r.width());
            line.setPosition(r.topLeft());
        }
    }
#endif
}

void QQuickTextPrivate::elideFormats(
        const int start, const int length, int offset, QList<QTextLayout::FormatRange> *elidedFormats)
{
    const int end = start + length;
    QList<QTextLayout::FormatRange> formats = layout.additionalFormats();
    for (int i = 0; i < formats.count(); ++i) {
        QTextLayout::FormatRange format = formats.at(i);
        const int formatLength = qMin(format.start + format.length, end) - qMax(format.start, start);
        if (formatLength > 0) {
            format.start = qMax(offset, format.start - start + offset);
            format.length = formatLength;
            elidedFormats->append(format);
        }
    }
}

QString QQuickTextPrivate::elidedText(qreal lineWidth, const QTextLine &line, QTextLine *nextLine) const
{
    if (nextLine) {
        nextLine->setLineWidth(INT_MAX);
        return layout.engine()->elidedText(
                Qt::TextElideMode(elideMode),
                QFixed::fromReal(lineWidth),
                0,
                line.textStart(),
                line.textLength() + nextLine->textLength());
    } else {
        QString elideText = layout.text().mid(line.textStart(), line.textLength());
        if (!styledText) {
            // QFontMetrics won't help eliding styled text.
            elideText[elideText.length() - 1] = elideChar;
            // Appending the elide character may push the line over the maximum width
            // in which case the elided text will need to be elided.
            QFontMetricsF metrics(layout.font());
            if (metrics.width(elideChar) + line.naturalTextWidth() >= lineWidth)
                elideText = metrics.elidedText(elideText, Qt::TextElideMode(elideMode), lineWidth);
        }
        return elideText;
    }
}

/*!
    Lays out the QQuickTextPrivate::layout QTextLayout in the constraints of the QQuickText.

    Returns the size of the final text.  This can be used to position the text vertically (the text is
    already absolutely positioned horizontally).
*/

QRectF QQuickTextPrivate::setupTextLayout(qreal *const naturalWidth)
{
    Q_Q(QQuickText);
    layout.setCacheEnabled(true);

    QTextOption textOption = layout.textOption();
    textOption.setAlignment(Qt::Alignment(q->effectiveHAlign()));
    textOption.setWrapMode(QTextOption::WrapMode(wrapMode));
    textOption.setUseDesignMetrics(true);
    layout.setTextOption(textOption);
    layout.setFont(font);

    if ((q->widthValid() && q->width() <= 0. && elideMode != QQuickText::ElideNone)
            || (q->heightValid() && q->height() <= 0. && wrapMode != QQuickText::NoWrap && elideMode == QQuickText::ElideRight)) {
        // we are elided and we have a zero width or height
        if (!truncated) {
            truncated = true;
            emit q->truncatedChanged();
        }
        if (lineCount) {
            lineCount = 0;
            emit q->lineCountChanged();
        }

        if (requireImplicitWidth) {
            // Layout to determine the implicit width.
            layout.beginLayout();

            for (int i = 0; i < maximumLineCount(); ++i) {
                QTextLine line = layout.createLine();
                if (!line.isValid())
                    break;
            }
            layout.endLayout();
            *naturalWidth = layout.maximumWidth();
            layout.clearLayout();
        }

        QFontMetrics fm(font);
        qreal height = (lineHeightMode() == QQuickText::FixedHeight) ? lineHeight() : fm.height() * lineHeight();
        return QRect(0, 0, 0, height);
    }

    const qreal lineWidth = q->widthValid() ? q->width() : FLT_MAX;
    const qreal maxHeight = q->heightValid() ? q->height() : FLT_MAX;

    const bool customLayout = isLineLaidOutConnected();
    const bool wasTruncated = truncated;

    const bool singlelineElide = elideMode != QQuickText::ElideNone && q->widthValid();
    const bool multilineElide = elideMode == QQuickText::ElideRight
            && q->widthValid()
            && (q->heightValid() || maximumLineCountValid);
    const bool canWrap = wrapMode != QQuickText::NoWrap && q->widthValid();

    const bool horizontalFit = fontSizeMode() & QQuickText::HorizontalFit && q->widthValid();
    const bool verticalFit = fontSizeMode() & QQuickText::VerticalFit
            && (q->heightValid() || (maximumLineCountValid && canWrap));
    const bool pixelSize = font.pixelSize() != -1;
    QString layoutText = layout.text();

    int largeFont = pixelSize ? font.pixelSize() : font.pointSize();
    int smallFont = fontSizeMode() != QQuickText::FixedSize
            ? qMin(pixelSize ? minimumPixelSize() : minimumPointSize(), largeFont)
            : largeFont;
    int scaledFontSize = largeFont;

    QRectF br;

    QFont scaledFont = font;

    QTextLine line;
    int visibleCount = 0;
    bool elide;
    qreal height = 0;
    QString elideText;
    bool once = true;
    int elideStart = 0;
    int elideEnd = 0;

    *naturalWidth = 0;

    int eos = multilengthEos;

    // Repeated layouts with reduced font sizes or abbreviated strings may be required if the text
    // doesn't fit within the element dimensions.
    for (;;) {
        if (!once) {
            if (pixelSize)
                scaledFont.setPixelSize(scaledFontSize);
            else
                scaledFont.setPointSize(scaledFontSize);
            layout.setFont(scaledFont);
        }
        layout.beginLayout();


        bool wrapped = false;
        bool truncateHeight = false;
        truncated = false;
        elide = false;
        int characterCount = 0;
        int unwrappedLineCount = 1;
        int maxLineCount = maximumLineCount();
        height = 0;
        br = QRectF();
        line = layout.createLine();
        for (visibleCount = 1; ; ++visibleCount) {
            qreal preLayoutHeight = height;

            if (customLayout) {
                setupCustomLineGeometry(line, height);
            } else {
                setLineGeometry(line, lineWidth, height);
            }

            // Elide the previous line if the accumulated height of the text exceeds the height
            // of the element.
            if (multilineElide && height > maxHeight && visibleCount > 1) {
                elide = true;
                if (eos != -1)  // There's an abbreviated string available, skip the rest as it's
                    break;      // all going to be discarded.

                truncated = true;
                truncateHeight = true;
                height = preLayoutHeight;

                characterCount = line.textStart() + line.textLength();

                QTextLine previousLine = layout.lineAt(visibleCount - 2);
                elideText = layoutText.at(line.textStart() - 1) != QChar::LineSeparator
                        ? elidedText(lineWidth, previousLine, &line)
                        : elidedText(lineWidth, previousLine);
                elideStart = previousLine.textStart();
                // elideEnd isn't required for right eliding.

                // The previous line is the last one visible so move the current one off somewhere
                // out of the way and back everything up one line.
                line.setLineWidth(0);
                line.setPosition(QPointF(FLT_MAX, FLT_MAX));
                line = previousLine;
                --visibleCount;
                height -= (lineHeightMode() == QQuickText::FixedHeight) ? lineHeight() : previousLine.height() * lineHeight();
                break;
            }

            QTextLine nextLine = layout.createLine();
            if (!nextLine.isValid()) {
                characterCount = line.textStart() + line.textLength();
                if (singlelineElide && visibleCount == 1 && line.naturalTextWidth() > lineWidth) {
                    // Elide a single line of  text if its width exceeds the element width.
                    elide = true;
                    if (eos != -1) // There's an abbreviated string available.
                        break;

                    truncated = true;
                    height = preLayoutHeight;
                    elideText = layout.engine()->elidedText(
                            Qt::TextElideMode(elideMode),
                            QFixed::fromReal(lineWidth),
                            0,
                            line.textStart(),
                            line.textLength());
                    elideStart = line.textStart();
                    elideEnd = elideStart + line.textLength();
                } else {
                    br = br.united(line.naturalTextRect());
                }
                break;
            } else {
                const bool wrappedLine = layoutText.at(nextLine.textStart() - 1) != QChar::LineSeparator;
                wrapped |= wrappedLine;

                if (!wrappedLine)
                    ++unwrappedLineCount;

                // Stop if the maximum number of lines has been reached and elide the last line
                // if enabled.
                if (visibleCount == maxLineCount) {
                    truncated = true;
                    characterCount = nextLine.textStart() + nextLine.textLength();

                    if (multilineElide) {
                        elide = true;
                        if (eos != -1)  // There's an abbreviated string available
                            break;
                        height = preLayoutHeight;
                        elideText = wrappedLine
                                ? elidedText(lineWidth, line, &nextLine)
                                : elidedText(lineWidth, line);
                        elideStart = line.textStart();
                        // elideEnd isn't required for right eliding.
                    } else {
                        br = br.united(line.naturalTextRect());
                    }
                    nextLine.setLineWidth(0);
                    nextLine.setPosition(QPointF(FLT_MAX, FLT_MAX));
                    break;
                }
            }
            br = br.united(line.naturalTextRect());
            line = nextLine;
        }

        layout.endLayout();
        br.moveTop(0);

        // Save the implicitWidth of the text on the first layout only.
        if (once) {
            *naturalWidth = layout.maximumWidth();
            once = false;

            if (requireImplicitWidth
                    && characterCount < layoutText.length()
                    && unwrappedLineCount < maxLineCount) {
                // Use a new layout to get the maximum width for the remaining text.  Using a
                // different layout excludes the truncated text from rendering.
                QTextLayout widthLayout(layoutText.mid(characterCount), scaledFont);
                widthLayout.setTextOption(layout.textOption());

                for (; unwrappedLineCount <= maxLineCount; ++unwrappedLineCount) {
                    QTextLine line = widthLayout.createLine();
                    if (!line.isValid())
                        break;
                }
                *naturalWidth = qMax(*naturalWidth, widthLayout.maximumWidth());
            }
        }

        // If the next needs to be elided and there's an abbreviated string available
        // go back and do another layout with the abbreviated string.
        if (eos != -1 && elide) {
            int start = eos + 1;
            eos = text.indexOf(QLatin1Char('\x9c'),  start);
            layoutText = text.mid(start, eos != -1 ? eos - start : -1);
            layoutText.replace(QLatin1Char('\n'), QChar::LineSeparator);
            layout.setText(layoutText);
            textHasChanged = true;
            continue;
        }

        if (!horizontalFit && !verticalFit)
            break;

        // Try and find a font size that better fits the dimensions of the element.
        QRectF unelidedRect = br.united(line.naturalTextRect());

        if (horizontalFit) {
            if (unelidedRect.width() > lineWidth || (!verticalFit && wrapped)) {
                largeFont = scaledFontSize - 1;
                scaledFontSize = (smallFont + largeFont) / 2;
                if (smallFont > largeFont)
                    break;
                continue;
            } else if (!verticalFit) {
                smallFont = scaledFontSize;
                scaledFontSize = (smallFont + largeFont + 1) / 2;
                if (smallFont == largeFont)
                    break;
            }
        }

        if (verticalFit) {
            if (truncateHeight || unelidedRect.height() > maxHeight) {
                largeFont = scaledFontSize - 1;
                scaledFontSize = (smallFont + largeFont + 1) / 2;
                if (smallFont > largeFont)
                    break;
            } else {
                smallFont = scaledFontSize;
                scaledFontSize = (smallFont + largeFont + 1) / 2;
                if (smallFont == largeFont)
                    break;
            }
        }

    }

    if (eos != multilengthEos)
        truncated = true;

    if (elide) {
        if (!elideLayout)
            elideLayout = new QTextLayout;
        if (styledText) {
            QList<QTextLayout::FormatRange> formats;
            switch (elideMode) {
            case QQuickText::ElideRight:
                elideFormats(elideStart, elideText.length() - 1, 0, &formats);
                break;
            case QQuickText::ElideLeft:
                elideFormats(elideEnd - elideText.length() + 1, elideText.length() - 1, 1, &formats);
                break;
            case QQuickText::ElideMiddle: {
                const int index = elideText.indexOf(elideChar);
                if (index != -1) {
                    elideFormats(elideStart, index, 0, &formats);
                    elideFormats(
                            elideEnd - elideText.length() + index + 1,
                            elideText.length() - index - 1,
                            index + 1,
                            &formats);
                }
                break;
            }
            default:
                break;
            }
            elideLayout->setAdditionalFormats(formats);
        }

        elideLayout->setFont(layout.font());
        elideLayout->setTextOption(layout.textOption());
        elideLayout->setText(elideText);
        elideLayout->beginLayout();

        QTextLine elidedLine = elideLayout->createLine();
        elidedLine.setPosition(QPointF(0, height));
        if (customLayout) {
            setupCustomLineGeometry(elidedLine, height, line.lineNumber());
        } else {
            setLineGeometry(elidedLine, lineWidth, height);
        }
        elideLayout->endLayout();

        br = br.united(elidedLine.naturalTextRect());

        if (visibleCount > 1)
            line.setPosition(QPointF(FLT_MAX, FLT_MAX));
        else
            layout.clearLayout();
    } else {
        delete elideLayout;
        elideLayout = 0;
    }

    if (!customLayout)
        br.setHeight(height);

    //Update the number of visible lines
    if (lineCount != visibleCount) {
        lineCount = visibleCount;
        emit q->lineCountChanged();
    }

    if (truncated != wasTruncated)
        emit q->truncatedChanged();

    return br;
}

void QQuickTextPrivate::setLineGeometry(QTextLine &line, qreal lineWidth, qreal &height)
{
    Q_Q(QQuickText);
    line.setLineWidth(lineWidth);

    if (imgTags.isEmpty()) {
        line.setPosition(QPointF(line.position().x(), height));
        height += (lineHeightMode() == QQuickText::FixedHeight) ? lineHeight() : line.height() * lineHeight();
        return;
    }

    qreal textTop = 0;
    qreal textHeight = line.height();
    qreal totalLineHeight = textHeight;

    QList<QQuickStyledTextImgTag *> imagesInLine;

    foreach (QQuickStyledTextImgTag *image, imgTags) {
        if (image->position >= line.textStart() &&
            image->position < line.textStart() + line.textLength()) {

            if (!image->pix) {
                QUrl url = q->baseUrl().resolved(image->url);
                image->pix = new QQuickPixmap(qmlEngine(q), url, image->size);
                if (image->pix->isLoading()) {
                    image->pix->connectFinished(q, SLOT(imageDownloadFinished()));
                    if (!extra.isAllocated() || !extra->nbActiveDownloads)
                        extra.value().nbActiveDownloads = 0;
                    extra->nbActiveDownloads++;
                } else if (image->pix->isReady()) {
                    if (!image->size.isValid()) {
                        image->size = image->pix->implicitSize();
                        // if the size of the image was not explicitly set, we need to
                        // call updateLayout() once again.
                        needToUpdateLayout = true;
                    }
                } else if (image->pix->isError()) {
                    qmlInfo(q) << image->pix->error();
                }
            }

            qreal ih = qreal(image->size.height());
            if (image->align == QQuickStyledTextImgTag::Top)
                image->pos.setY(0);
            else if (image->align == QQuickStyledTextImgTag::Middle)
                image->pos.setY((textHeight / 2.0) - (ih / 2.0));
            else
                image->pos.setY(textHeight - ih);
            imagesInLine << image;
            textTop = qMax(textTop, qAbs(image->pos.y()));
        }
    }

    foreach (QQuickStyledTextImgTag *image, imagesInLine) {
        totalLineHeight = qMax(totalLineHeight, textTop + image->pos.y() + image->size.height());
        image->pos.setX(line.cursorToX(image->position));
        image->pos.setY(image->pos.y() + height + textTop);
        visibleImgTags << image;
    }

    line.setPosition(QPointF(line.position().x(), height + textTop));
    height += (lineHeightMode() == QQuickText::FixedHeight) ? lineHeight() : totalLineHeight * lineHeight();
}

/*!
    Ensures the QQuickTextPrivate::doc variable is set to a valid text document
*/
void QQuickTextPrivate::ensureDoc()
{
    if (!extra.isAllocated() || !extra->doc) {
        Q_Q(QQuickText);
        extra.value().doc = new QQuickTextDocumentWithImageResources(q);
        extra->doc->setDocumentMargin(0);
        extra->doc->setBaseUrl(q->baseUrl());
        FAST_CONNECT(extra->doc, SIGNAL(imagesLoaded()), q, SLOT(q_imagesLoaded()));
    }
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

    \snippet doc/src/snippets/qml/text/onLinkActivated.qml 0

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

    if (oldFont != d->font) {
        // if the format changes the size of the text
        // with headings or <font> tag, we need to re-parse
        if (d->formatModifiesFontSize)
            d->textHasChanged = true;
        d->updateLayout();
    }

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
            d->extra->doc->setText(n);
            d->rightToLeftText = d->extra->doc->toPlainText().isRightToLeft();
        } else {
            d->rightToLeftText = d->text.isRightToLeft();
        }
        d->determineHorizontalAlignment();
    }
    d->textHasChanged = true;
    qDeleteAll(d->imgTags);
    d->imgTags.clear();
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
    return QColor::fromRgba(d->color);
}

void QQuickText::setColor(const QColor &color)
{
    Q_D(QQuickText);
    QRgb rgb = color.rgba();
    if (d->color == rgb)
        return;

    d->color = rgb;
    if (isComponentComplete())  {
        d->updateType = QQuickTextPrivate::UpdatePaintNode;
        update();
    }
    emit colorChanged();
}

/*!
    \qmlproperty color QtQuick2::Text::linkColor

    The color of links in the text.

    This property works with the StyledText \l textFormat, but not with RichText.
    Link color in RichText can be specified by including CSS style tags in the
    text.
*/

QColor QQuickText::linkColor() const
{
    Q_D(const QQuickText);
    return QColor::fromRgba(d->linkColor);
}

void QQuickText::setLinkColor(const QColor &color)
{
    Q_D(QQuickText);
    QRgb rgb = color.rgba();
    if (d->linkColor == rgb)
        return;

    d->linkColor = rgb;
    update();
    emit linkColorChanged();
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

    d->style = style;
    if (isComponentComplete()) {
        d->updateType = QQuickTextPrivate::UpdatePaintNode;
        update();
    }
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
    return QColor::fromRgba(d->styleColor);
}

void QQuickText::setStyleColor(const QColor &color)
{
    Q_D(QQuickText);
    QRgb rgb = color.rgba();
    if (d->styleColor == rgb)
        return;

    d->styleColor = rgb;
    if (isComponentComplete()) {
        d->updateType = QQuickTextPrivate::UpdatePaintNode;
        update();
    }
    emit styleColorChanged();
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
    if (isComponentComplete() && d->determineHorizontalAlignment())
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
    if (hAlignImplicit) {
        bool alignToRight = text.isEmpty() ? qApp->inputMethod()->inputDirection() == Qt::RightToLeft : rightToLeftText;
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
    \o Text.NoWrap (default) - no wrapping will be performed. If the text contains insufficient newlines, then \l contentWidth will exceed a set width.
    \o Text.WordWrap - wrapping is done on word boundaries only. If a word is too long, \l contentWidth will exceed a set width.
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
    return d->maximumLineCount();
}

void QQuickText::setMaximumLineCount(int lines)
{
    Q_D(QQuickText);

    d->maximumLineCountValid = lines==INT_MAX ? false : true;
    if (d->maximumLineCount() != lines) {
        d->extra.value().maximumLineCount = lines;
        d->updateLayout();
        emit maximumLineCountChanged();
    }
}

void QQuickText::resetMaximumLineCount()
{
    Q_D(QQuickText);
    setMaximumLineCount(INT_MAX);
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
    styled text.  This determination is made using Qt::mightBeRichText()
    which uses a fast and therefore simple heuristic. It mainly checks
    whether there is something that looks like a tag before the first
    line break. Although the result may be correct for common cases,
    there is no guarantee.

    Text.StyledText is an optimized format supporting some basic text
    styling markup, in the style of html 3.2:

    \code
    <b></b> - bold
    <strong></strong> - bold
    <i></i> - italic
    <br> - new line
    <p> - paragraph
    <u> - underlined text
    <font color="color_name" size="1-7"></font>
    <h1> to <h6> - headers
    <a href=""> - anchor
    <img src="" align="top,middle,bottom" width="" height=""> - inline images
    <ol type="">, <ul type=""> and <li> - ordered and unordered lists
    <pre></pre> - preformatted
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

    if (isComponentComplete()) {
        if (!wasRich && d->richText) {
            d->ensureDoc();
            d->extra->doc->setText(d->text);
            d->rightToLeftText = d->extra->doc->toPlainText().isRightToLeft();
        } else {
            d->rightToLeftText = d->text.isRightToLeft();
        }
        d->determineHorizontalAlignment();
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

    If this property is set to Text.ElideRight, it can be used with \l {wrapMode}{wrapped}
    text. The text will only elide if \c maximumLineCount, or \c height has been set.
    If both \c maximumLineCount and \c height are set, \c maximumLineCount will
    apply unless the lines do not fit in the height allowed.

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

    emit elideModeChanged(mode);
}

/*!
    \qmlproperty url QtQuick2::Text::baseUrl

    This property specifies a base URL which is used to resolve relative URLs
    within the text.

    Urls are resolved to be within the same directory as the target of the base
    URL meaning any portion of the path after the last '/' will be ignored.

    \table
    \header \o Base URL \o Relative URL \o Resolved URL
    \row \o http://qt-project.org/ \o images/logo.png \o http://qt-project.org/images/logo.png
    \row \o http://qt-project.org/index.html \o images/logo.png \o http://qt-project.org/images/logo.png
    \row \o http://qt-project.org/content \o images/logo.png \o http://qt-project.org/content/images/logo.png
    \row \o http://qt-project.org/content/ \o images/logo.png \o http://qt-project.org/content/images/logo.png
    \row \o http://qt-project.org/content/index.html \o images/logo.png \o http://qt-project.org/content/images/logo.png
    \row \o http://qt-project.org/content/index.html \o ../images/logo.png \o http://qt-project.org/images/logo.png
    \row \o http://qt-project.org/content/index.html \o /images/logo.png \o http://qt-project.org/images/logo.png
    \endtable

    By default is the url of the Text element.
*/

QUrl QQuickText::baseUrl() const
{
    Q_D(const QQuickText);
    if (d->baseUrl.isEmpty()) {
        if (QQmlContext *context = qmlContext(this))
            const_cast<QQuickTextPrivate *>(d)->baseUrl = context->baseUrl();
    }
    return d->baseUrl;
}

void QQuickText::setBaseUrl(const QUrl &url)
{
    Q_D(QQuickText);
    if (baseUrl() != url) {
        d->baseUrl = url;

        if (d->richText) {
            d->ensureDoc();
            d->extra->doc->setBaseUrl(url);
        }
        if (d->styledText) {
            d->textHasChanged = true;
            qDeleteAll(d->imgTags);
            d->imgTags.clear();
            d->updateLayout();
        }
        emit baseUrlChanged();
    }
}

void QQuickText::resetBaseUrl()
{
    if (QQmlContext *context = qmlContext(this))
        setBaseUrl(context->baseUrl());
    else
        setBaseUrl(QUrl());
}

/*! \internal */
QRectF QQuickText::boundingRect() const
{
    Q_D(const QQuickText);

    QRectF rect = d->layedOutTextRect;
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

    return rect;
}

/*! \internal */
void QQuickText::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickText);
    if (d->text.isEmpty()) {
        QQuickItem::geometryChanged(newGeometry, oldGeometry);
        return;
    }

    bool widthChanged = newGeometry.width() != oldGeometry.width();
    bool heightChanged = newGeometry.height() != oldGeometry.height();
    bool leftAligned = effectiveHAlign() == QQuickText::AlignLeft;
    bool wrapped = d->wrapMode != QQuickText::NoWrap;
    bool elide = d->elideMode != QQuickText::ElideNone;
    bool scaleFont = d->fontSizeMode() != QQuickText::FixedSize && (widthValid() || heightValid());

    if ((!widthChanged && !heightChanged) || d->internalWidthUpdate)
        goto geomChangeDone;

    if (leftAligned && !wrapped && !elide && !scaleFont)
        goto geomChangeDone; // left aligned unwrapped text without eliding never needs relayout

    if (!widthChanged && !wrapped && d->singleline && !scaleFont)
        goto geomChangeDone; // only height has changed which doesn't affect single line unwrapped text

    if (!widthChanged && wrapped && d->elideMode != QQuickText::ElideRight && !scaleFont)
        goto geomChangeDone; // only height changed and no multiline eliding.

    if (leftAligned && d->elideMode == QQuickText::ElideRight && !d->truncated && d->singleline
            && !wrapped && newGeometry.width() > oldGeometry.width() && !scaleFont)
        goto geomChangeDone; // Eliding not affected if we're not currently truncated and we get wider.

    if (d->elideMode == QQuickText::ElideRight && wrapped && newGeometry.height() > oldGeometry.height() && !scaleFont) {
        if (!d->truncated)
            goto geomChangeDone; // Multiline eliding not affected if we're not currently truncated and we get higher.
        if (d->maximumLineCountValid && d->lineCount == d->maximumLineCount())
            goto geomChangeDone; // Multiline eliding not affected if we're already at max line count and we get higher.
    }

    if (d->updateOnComponentComplete || d->textHasChanged) {
        // We need to re-elide
        d->updateLayout();
    } else {
        // We just need to re-layout
        d->updateSize();
    }

geomChangeDone:
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
}

void QQuickText::triggerPreprocess()
{
    Q_D(QQuickText);
    if (d->updateType == QQuickTextPrivate::UpdateNone)
        d->updateType = QQuickTextPrivate::UpdatePreprocess;
    update();
}

QSGNode *QQuickText::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    Q_D(QQuickText);

    if (d->text.isEmpty()) {
        delete oldNode;
        return 0;
    }

    if (d->updateType != QQuickTextPrivate::UpdatePaintNode && oldNode != 0) {
        // Update done in preprocess() in the nodes
        d->updateType = QQuickTextPrivate::UpdateNone;
        return oldNode;
    }

    d->updateType = QQuickTextPrivate::UpdateNone;

    QRectF bounds = boundingRect();

    // We need to make sure the layout is done in the current thread
#if defined(Q_OS_MAC)
    d->paintingThread = QThread::currentThread();
    if (d->layoutThread != d->paintingThread)
        d->updateLayout();
#endif

    QQuickTextNode *node = 0;
    if (!oldNode) {
        node = new QQuickTextNode(QQuickItemPrivate::get(this)->sceneGraphContext(), this);
    } else {
        node = static_cast<QQuickTextNode *>(oldNode);
    }

    node->deleteContent();
    node->setMatrix(QMatrix4x4());

    const QColor color = QColor::fromRgba(d->color);
    const QColor styleColor = QColor::fromRgba(d->styleColor);
    const QColor linkColor = QColor::fromRgba(d->linkColor);

    if (d->richText) {
        d->ensureDoc();
        node->addTextDocument(bounds.topLeft(), d->extra->doc, color, d->style, styleColor, linkColor);
    } else if (d->elideMode == QQuickText::ElideNone || bounds.width() > 0.) {
        node->addTextLayout(QPoint(0, bounds.y()), &d->layout, color, d->style, styleColor, linkColor);
        if (d->elideLayout)
            node->addTextLayout(QPoint(0, bounds.y()), d->elideLayout, color, d->style, styleColor, linkColor);
    }

    foreach (QQuickStyledTextImgTag *img, d->visibleImgTags) {
        QQuickPixmap *pix = img->pix;
        if (pix && pix->isReady())
            node->addImage(QRectF(img->pos.x(), img->pos.y() + bounds.y(), pix->width(), pix->height()), pix->image());
    }
    return node;
}

void QQuickText::updatePolish()
{
    Q_D(QQuickText);
    if (d->updateLayoutOnPolish)
        d->updateLayout();
    else
        d->updateSize();
}

/*!
    \qmlproperty real QtQuick2::Text::contentWidth

    Returns the width of the text, including width past the width
    which is covered due to insufficient wrapping if WrapMode is set.
*/
qreal QQuickText::contentWidth() const
{
    Q_D(const QQuickText);
    return d->layedOutTextRect.width();
}

/*!
    \qmlproperty real QtQuick2::Text::contentHeight

    Returns the height of the text, including height past the height
    which is covered due to there being more text than fits in the set height.
*/
qreal QQuickText::contentHeight() const
{
    Q_D(const QQuickText);
    return d->layedOutTextRect.height();
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
    return d->lineHeight();
}

void QQuickText::setLineHeight(qreal lineHeight)
{
    Q_D(QQuickText);

    if ((d->lineHeight() == lineHeight) || (lineHeight < 0.0))
        return;

    d->extra.value().lineHeight = lineHeight;
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
    return d->lineHeightMode();
}

void QQuickText::setLineHeightMode(LineHeightMode mode)
{
    Q_D(QQuickText);
    if (mode == d->lineHeightMode())
        return;

    d->extra.value().lineHeightMode = mode;
    d->updateLayout();

    emit lineHeightModeChanged(mode);
}

/*!
    \qmlproperty enumeration QtQuick2::Text::fontSizeMode

    This property specifies how the font size of the displayed text is determined.
    The possible values are:

    \list
    \o Text.FixedSize (default) - The size specified by \l font.pixelSize
    or \l font.pointSize is used.
    \o Text.HorizontalFit - The largest size up to the size specified that fits
    within the width of the item without wrapping is used.
    \o Text.VerticalFit - The largest size up to the size specified that fits
    the height of the item is used.
    \o Text.Fit - The largest size up to the size specified the fits within the
    width and height of the item is used.
    \endlist

    The font size of fitted text has a minimum bound specified by the
    minimumPointSize or minimumPixelSize property and maximum bound specified
    by either the \l font.pointSize or \l font.pixelSize properties.

    If the text does not fit within the item bounds with the minimum font size
    the text will be elided as per the \l elide property.
*/

QQuickText::FontSizeMode QQuickText::fontSizeMode() const
{
    Q_D(const QQuickText);
    return d->fontSizeMode();
}

void QQuickText::setFontSizeMode(FontSizeMode mode)
{
    Q_D(QQuickText);
    if (d->fontSizeMode() == mode)
        return;

    polish();

    d->extra.value().fontSizeMode = mode;
    emit fontSizeModeChanged();
}

/*!
    \qmlproperty int QtQuick2::Text::minimumPixelSize

    This property specifies the minimum font pixel size of text scaled by the
    fontSizeMode property.

    If the fontSizeMode is Text.FixedSize or the \l font.pixelSize is -1 this
    property is ignored.
*/

int QQuickText::minimumPixelSize() const
{
    Q_D(const QQuickText);
    return d->minimumPixelSize();
}

void QQuickText::setMinimumPixelSize(int size)
{
    Q_D(QQuickText);
    if (d->minimumPixelSize() == size)
        return;

    if (d->fontSizeMode() != FixedSize && (widthValid() || heightValid()))
        polish();
    d->extra.value().minimumPixelSize = size;
    emit minimumPixelSizeChanged();
}

/*!
    \qmlproperty int QtQuick2::Text::minimumPointSize

    This property specifies the minimum font point \l size of text scaled by
    the fontSizeMode property.

    If the fontSizeMode is Text.FixedSize or the \l font.pointSize is -1 this
    property is ignored.
*/

int QQuickText::minimumPointSize() const
{
    Q_D(const QQuickText);
    return d->minimumPointSize();
}

void QQuickText::setMinimumPointSize(int size)
{
    Q_D(QQuickText);
    if (d->minimumPointSize() == size)
        return;

    if (d->fontSizeMode() != FixedSize && (widthValid() || heightValid()))
        polish();
    d->extra.value().minimumPointSize = size;
    emit minimumPointSizeChanged();
}

/*!
    Returns the number of resources (images) that are being loaded asynchronously.
*/
int QQuickText::resourcesLoading() const
{
    Q_D(const QQuickText);
    if (d->richText && d->extra.isAllocated() && d->extra->doc)
        return d->extra->doc->resourcesLoading();
    return 0;
}

/*! \internal */
void QQuickText::componentComplete()
{
    Q_D(QQuickText);
    if (d->updateOnComponentComplete) {
        if (d->richText) {
            d->ensureDoc();
            d->extra->doc->setText(d->text);
            d->rightToLeftText = d->extra->doc->toPlainText().isRightToLeft();
        } else {
            d->rightToLeftText = d->text.isRightToLeft();
        }
        d->determineHorizontalAlignment();
    }
    QQuickItem::componentComplete();
    if (d->updateOnComponentComplete)
        d->updateLayout();

    // Enable accessibility for text items.
    d->setAccessibleFlagAndListener();
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

    QString link;
    if (d->isLinkActivatedConnected()) {
        if (d->styledText)
            link = d->anchorAt(event->localPos());
        else if (d->richText) {
            d->ensureDoc();
            link = d->extra->doc->documentLayout()->anchorAt(event->localPos());
        }
    }

    if (link.isEmpty()) {
        event->setAccepted(false);
    } else {
        d->extra.value().activeLink = link;
    }

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
        else if (d->richText) {
            d->ensureDoc();
            link = d->extra->doc->documentLayout()->anchorAt(event->localPos());
        }
    }

    if (!link.isEmpty() && d->extra.isAllocated() && d->extra->activeLink == link)
        emit linkActivated(d->extra->activeLink);
    else
        event->setAccepted(false);

    if (!event->isAccepted())
        QQuickItem::mouseReleaseEvent(event);
}

QT_END_NAMESPACE
