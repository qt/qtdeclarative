// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qquicktext_p.h"
#include "qquicktext_p_p.h"

#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qqmldebugconnector_p.h>

#include <QtQuick/private/qsgcontext_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qsgadaptationlayer_p.h>
#include "qsginternaltextnode_p.h"
#include "qquicktextutil_p.h"

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
#include <QtQuick/private/qquickpixmap_p.h>

#include <qmath.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcHoverTrace)
Q_LOGGING_CATEGORY(lcText, "qt.quick.text")

using namespace Qt::StringLiterals;

const QChar QQuickTextPrivate::elideChar = QChar(0x2026);

#if !defined(QQUICKTEXT_LARGETEXT_THRESHOLD)
  #define QQUICKTEXT_LARGETEXT_THRESHOLD 10000
#endif
// if QString::size() > largeTextSizeThreshold, we render more often, but only visible lines
const int QQuickTextPrivate::largeTextSizeThreshold = QQUICKTEXT_LARGETEXT_THRESHOLD;

QQuickTextPrivate::QQuickTextPrivate()
    : fontInfo(font), lineWidth(0)
    , color(0xFF000000), linkColor(0xFF0000FF), styleColor(0xFF000000)
    , lineCount(1), multilengthEos(-1)
    , elideMode(QQuickText::ElideNone), hAlign(QQuickText::AlignLeft), vAlign(QQuickText::AlignTop)
    , format(QQuickText::AutoText), wrapMode(QQuickText::NoWrap)
    , style(QQuickText::Normal)
    , renderType(QQuickTextUtil::textRenderType<QQuickText>())
    , updateType(UpdatePaintNode)
    , maximumLineCountValid(false), updateOnComponentComplete(true), richText(false)
    , styledText(false), widthExceeded(false), heightExceeded(false), internalWidthUpdate(false)
    , requireImplicitSize(false), implicitWidthValid(false), implicitHeightValid(false)
    , truncated(false), hAlignImplicit(true), rightToLeftText(false)
    , layoutTextElided(false), textHasChanged(true), needToUpdateLayout(false), formatModifiesFontSize(false)
    , polishSize(false)
    , updateSizeRecursionGuard(false)
    , containsUnscalableGlyphs(false)
{
    implicitAntialiasing = true;
}

QQuickTextPrivate::ExtraData::ExtraData()
    : padding(0)
    , topPadding(0)
    , leftPadding(0)
    , rightPadding(0)
    , bottomPadding(0)
    , explicitTopPadding(false)
    , explicitLeftPadding(false)
    , explicitRightPadding(false)
    , explicitBottomPadding(false)
    , lineHeight(1.0)
    , doc(nullptr)
    , minimumPixelSize(12)
    , minimumPointSize(12)
    , maximumLineCount(INT_MAX)
    , renderTypeQuality(QQuickText::DefaultRenderTypeQuality)
    , lineHeightValid(false)
    , lineHeightMode(QQuickText::ProportionalHeight)
    , fontSizeMode(QQuickText::FixedSize)
{
}

void QQuickTextPrivate::init()
{
    Q_Q(QQuickText);
    q->setAcceptedMouseButtons(Qt::LeftButton);
    q->setFlag(QQuickItem::ItemHasContents);
    q->setFlag(QQuickItem::ItemObservesViewport); // default until size is known
}

QQuickTextPrivate::~QQuickTextPrivate()
{
    if (extra.isAllocated()) {
        qDeleteAll(extra->imgTags);
        extra->imgTags.clear();
    }
}

qreal QQuickTextPrivate::getImplicitWidth() const
{
    if (!requireImplicitSize) {
        // We don't calculate implicitWidth unless it is required.
        // We need to force a size update now to ensure implicitWidth is calculated
        QQuickTextPrivate *me = const_cast<QQuickTextPrivate*>(this);
        me->requireImplicitSize = true;
        me->updateSize();
    }
    return implicitWidth;
}

qreal QQuickTextPrivate::getImplicitHeight() const
{
    if (!requireImplicitSize) {
        QQuickTextPrivate *me = const_cast<QQuickTextPrivate*>(this);
        me->requireImplicitSize = true;
        me->updateSize();
    }
    return implicitHeight;
}

qreal QQuickTextPrivate::availableWidth() const
{
    Q_Q(const QQuickText);
    return q->width() - q->leftPadding() - q->rightPadding();
}

qreal QQuickTextPrivate::availableHeight() const
{
    Q_Q(const QQuickText);
    return q->height() - q->topPadding() - q->bottomPadding();
}

void QQuickTextPrivate::setTopPadding(qreal value, bool reset)
{
    Q_Q(QQuickText);
    qreal oldPadding = q->topPadding();
    if (!reset || extra.isAllocated()) {
        extra.value().topPadding = value;
        extra.value().explicitTopPadding = !reset;
    }
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding()))) {
        updateSize();
        emit q->topPaddingChanged();
    }
}

void QQuickTextPrivate::setLeftPadding(qreal value, bool reset)
{
    Q_Q(QQuickText);
    qreal oldPadding = q->leftPadding();
    if (!reset || extra.isAllocated()) {
        extra.value().leftPadding = value;
        extra.value().explicitLeftPadding = !reset;
    }
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding()))) {
        updateSize();
        emit q->leftPaddingChanged();
    }
}

void QQuickTextPrivate::setRightPadding(qreal value, bool reset)
{
    Q_Q(QQuickText);
    qreal oldPadding = q->rightPadding();
    if (!reset || extra.isAllocated()) {
        extra.value().rightPadding = value;
        extra.value().explicitRightPadding = !reset;
    }
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding()))) {
        updateSize();
        emit q->rightPaddingChanged();
    }
}

void QQuickTextPrivate::setBottomPadding(qreal value, bool reset)
{
    Q_Q(QQuickText);
    qreal oldPadding = q->bottomPadding();
    if (!reset || extra.isAllocated()) {
        extra.value().bottomPadding = value;
        extra.value().explicitBottomPadding = !reset;
    }
    if ((!reset && !qFuzzyCompare(oldPadding, value)) || (reset && !qFuzzyCompare(oldPadding, padding()))) {
        updateSize();
        emit q->bottomPaddingChanged();
    }
}

/*!
    \qmlproperty bool QtQuick::Text::antialiasing

    Used to decide if the Text should use antialiasing or not. Only Text
    with renderType of Text.NativeRendering can disable antialiasing.

    The default is \c true.
*/

void QQuickText::q_updateLayout()
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

    if (extra.isAllocated())
        extra->visibleImgTags.clear();
    needToUpdateLayout = false;

    // Setup instance of QTextLayout for all cases other than richtext
    if (!richText) {
        if (textHasChanged) {
            if (styledText && !text.isEmpty()) {
                layout.setFont(font);
                // needs temporary bool because formatModifiesFontSize is in a bit-field
                bool fontSizeModified = false;
                QList<QQuickStyledTextImgTag*> someImgTags = extra.isAllocated() ? extra->imgTags : QList<QQuickStyledTextImgTag*>();
                QQuickStyledText::parse(text, layout, someImgTags, q->baseUrl(), qmlContext(q), !maximumLineCountValid, &fontSizeModified);
                if (someImgTags.size() || extra.isAllocated())
                    extra.value().imgTags = someImgTags;
                formatModifiesFontSize = fontSizeModified;
                multilengthEos = -1;
            } else {
                QString tmp = text;
                multilengthEos = tmp.indexOf(QLatin1Char('\x9c'));
                if (multilengthEos != -1)
                    tmp = tmp.mid(0, multilengthEos);
                tmp.replace(QLatin1Char('\n'), QChar::LineSeparator);
                layout.setText(tmp);
            }
            textHasChanged = false;
        }
    } else if (extra.isAllocated() && extra->lineHeightValid) {
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

    q->polish();
}

/*! \internal
    QTextDocument::loadResource() calls this to load inline images etc.
    But if it's a local file, don't do it: let QTextDocument::loadResource()
    load it in the default way. QQuickPixmap is for QtQuick-specific uses.
*/
QVariant QQuickText::loadResource(int type, const QUrl &source)
{
    Q_D(QQuickText);
    const QUrl url = d->extra->doc->baseUrl().resolved(source);
    if (url.isLocalFile()) {
        // qmlWarning if the file doesn't exist (because QTextDocument::loadResource() can't do that)
        const QFileInfo fi(QQmlFile::urlToLocalFileOrQrc(url));
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
    for (auto it = d->extra->pixmapsInProgress.cbegin(); it != d->extra->pixmapsInProgress.cend();) {
        auto *job = *it;
        if (job->url() == url) {
            if (job->isError()) {
                qmlWarning(this) << job->error();
                delete *it;
                it = d->extra->pixmapsInProgress.erase(it);
                return QImage();
            }
            qCDebug(lcText) << "already downloading" << url;
            // existing job: return a null variant if it's not done yet
            return job->isReady() ? job->image() : QVariant();
        }
        ++it;
    }
    qCDebug(lcText) << "loading" << source << "resolved" << url
                    << "type" << static_cast<QTextDocument::ResourceType>(type);
    QQmlContext *context = qmlContext(this);
    Q_ASSERT(context);
    // don't cache it in QQuickPixmapCache, because it's cached in QTextDocumentPrivate::cachedResources
    QQuickPixmap *p = new QQuickPixmap(context->engine(), url, QQuickPixmap::Options{});
    p->connectFinished(this, SLOT(resourceRequestFinished()));
    d->extra->pixmapsInProgress.append(p);
    // the new job is probably not done; return a null variant if the caller should poll again
    return p->isReady() ? p->image() : QVariant();
}

/*! \internal
    Handle completion of a download that QQuickText::loadResource() started.
*/
void QQuickText::resourceRequestFinished()
{
    Q_D(QQuickText);
    bool allDone = true;
    for (auto it = d->extra->pixmapsInProgress.cbegin(); it != d->extra->pixmapsInProgress.cend();) {
        auto *job = *it;
        if (job->isError()) {
            // get QTextDocument::loadResource() to call QQuickText::loadResource() again, to return the placeholder
            qCDebug(lcText) << "failed to load" << job->url();
            d->extra->doc->resource(QTextDocument::ImageResource, job->url());
        } else if (job->isReady()) {
            // get QTextDocument::loadResource() to call QQuickText::loadResource() again, and cache the result
            auto res = d->extra->doc->resource(QTextDocument::ImageResource, job->url());
            // If QTextDocument::resource() returned a valid variant, it's been cached too. Either way, the job is done.
            qCDebug(lcText) << (res.isValid() ? "done downloading" : "failed to load") << job->url();
            delete *it;
            it = d->extra->pixmapsInProgress.erase(it);
        } else {
            allDone = false;
            ++it;
        }
    }
    if (allDone) {
        Q_ASSERT(d->extra->pixmapsInProgress.isEmpty());
        d->updateLayout();
    }
}

/*! \internal
    Handle completion of StyledText image downloads (there's no QTextDocument instance in that case).
*/
void QQuickText::imageDownloadFinished()
{
    Q_D(QQuickText);
    if (!d->extra.isAllocated())
        return;

    if (std::any_of(d->extra->imgTags.cbegin(), d->extra->imgTags.cend(),
                    [] (auto *image) { return image->pix && image->pix->isLoading(); })) {
        // return if we still have any active download
        return;
    }

    // when all the remote images have been downloaded,
    // if one of the sizes was not specified at parsing time
    // we use the implicit size from pixmapcache and re-layout.

    bool needToUpdateLayout = false;
    for (QQuickStyledTextImgTag *img : std::as_const(d->extra->visibleImgTags)) {
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

void QQuickTextPrivate::updateBaseline(qreal baseline, qreal dy)
{
    Q_Q(QQuickText);

    qreal yoff = 0;

    if (q->heightValid()) {
        if (vAlign == QQuickText::AlignBottom)
            yoff = dy;
        else if (vAlign == QQuickText::AlignVCenter)
            yoff = dy/2;
    }

    q->setBaselineOffset(baseline + yoff + q->topPadding());
}

void QQuickTextPrivate::signalSizeChange(const QSizeF &previousSize)
{
    Q_Q(QQuickText);
    const QSizeF contentSize(q->contentWidth(), q->contentHeight());

    if (contentSize != previousSize) {
        emit q->contentSizeChanged();
        if (contentSize.width() != previousSize.width())
            emit q->contentWidthChanged(contentSize.width());
        if (contentSize.height() != previousSize.height())
            emit q->contentHeightChanged(contentSize.height());
    }
}

void QQuickTextPrivate::updateSize()
{
    Q_Q(QQuickText);

    if (!q->isComponentComplete()) {
        updateOnComponentComplete = true;
        return;
    }

    if (!requireImplicitSize) {
        implicitWidthChanged();
        implicitHeightChanged();
        // if the implicitWidth is used, then updateSize() has already been called (recursively)
        if (requireImplicitSize)
            return;
    }

    qreal hPadding = q->leftPadding() + q->rightPadding();
    qreal vPadding = q->topPadding() + q->bottomPadding();

    const QSizeF previousSize(q->contentWidth(), q->contentHeight());

    if (text.isEmpty() && !isLineLaidOutConnected() && fontSizeMode() == QQuickText::FixedSize) {
        // How much more expensive is it to just do a full layout on an empty string here?
        // There may be subtle differences in the height and baseline calculations between
        // QTextLayout and QFontMetrics and the number of variables that can affect the size
        // and position of a line is increasing.
        QFontMetricsF fm(font);
        qreal fontHeight = qCeil(fm.height());  // QScriptLine and therefore QTextLine rounds up
        if (!richText) {                        // line height, so we will as well.
            fontHeight = lineHeightMode() == QQuickText::FixedHeight
                    ? lineHeight()
                    : fontHeight * lineHeight();
        }
        updateBaseline(fm.ascent(), q->height() - fontHeight - vPadding);
        q->setImplicitSize(hPadding, fontHeight + qMax(lineHeightOffset(), 0) + vPadding);
        layedOutTextRect = QRectF(0, 0, 0, fontHeight);
        advance = QSizeF();
        signalSizeChange(previousSize);
        lineCount = 1;
        emit q->lineCountChanged();
        updateType = UpdatePaintNode;
        q->update();
        return;
    }

    QSizeF size(0, 0);

    //setup instance of QTextLayout for all cases other than richtext
    if (!richText) {
        qreal baseline = 0;
        QRectF textRect = setupTextLayout(&baseline);

        if (internalWidthUpdate)    // probably the result of a binding loop, but by letting it
            return;      // get this far we'll get a warning to that effect if it is.

        layedOutTextRect = textRect;
        size = textRect.size();
        updateBaseline(baseline, q->height() - size.height() - vPadding);
    } else {
        widthExceeded = true; // always relayout rich text on width changes..
        heightExceeded = false; // rich text layout isn't affected by height changes.
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
        option.setUseDesignMetrics(renderType != QQuickText::NativeRendering);
        extra->doc->setDefaultTextOption(option);
        qreal naturalWidth = 0;
        if (requireImplicitSize) {
            extra->doc->setTextWidth(-1);
            naturalWidth = extra->doc->idealWidth();
            const bool wasInLayout = internalWidthUpdate;
            internalWidthUpdate = true;
            q->setImplicitWidth(naturalWidth + hPadding);
            internalWidthUpdate = wasInLayout;
        }
        if (internalWidthUpdate)
            return;

        extra->doc->setPageSize(QSizeF(q->width(), -1));
        if (q->widthValid() && (wrapMode != QQuickText::NoWrap || extra->doc->idealWidth() < availableWidth()))
            extra->doc->setTextWidth(availableWidth());
        else
            extra->doc->setTextWidth(extra->doc->idealWidth()); // ### Text does not align if width is not set (QTextDoc bug)

        QSizeF dsize = extra->doc->size();
        layedOutTextRect = QRectF(QPointF(0,0), dsize);
        size = QSizeF(extra->doc->idealWidth(),dsize.height());


        qreal baseline = QFontMetricsF(font).ascent();
        QTextBlock firstBlock = extra->doc->firstBlock();
        if (firstBlock.isValid() && firstBlock.layout() != nullptr && firstBlock.lineCount() > 0)
            baseline = firstBlock.layout()->lineAt(0).ascent();

        updateBaseline(baseline, q->height() - size.height() - vPadding);

        //### need to confirm cost of always setting these for richText
        internalWidthUpdate = true;
        qreal oldWidth = q->width();
        qreal iWidth = -1;
        if (!q->widthValid())
            iWidth = size.width();
        if (iWidth > -1)
            q->setImplicitSize(iWidth + hPadding, size.height() + qMax(lineHeightOffset(), 0) + vPadding);
        internalWidthUpdate = false;

        // If the implicit width update caused a recursive change of the width,
        // we will have skipped integral parts of the layout due to the
        // internalWidthUpdate recursion guard. To make sure everything is up
        // to date, we need to run a second pass over the layout when updateSize()
        // is done.
        if (!qFuzzyCompare(q->width(), oldWidth) && !updateSizeRecursionGuard) {
            updateSizeRecursionGuard = true;
            updateSize();
            updateSizeRecursionGuard = false;
        } else {
            if (iWidth == -1)
                q->setImplicitHeight(size.height() + lineHeightOffset() + vPadding);

            QTextBlock firstBlock = extra->doc->firstBlock();
            while (firstBlock.layout()->lineCount() == 0)
                firstBlock = firstBlock.next();

            QTextBlock lastBlock = extra->doc->lastBlock();
            while (lastBlock.layout()->lineCount() == 0)
                lastBlock = lastBlock.previous();

            if (firstBlock.lineCount() > 0 && lastBlock.lineCount() > 0) {
                QTextLine firstLine = firstBlock.layout()->lineAt(0);
                QTextLine lastLine = lastBlock.layout()->lineAt(lastBlock.layout()->lineCount() - 1);
                advance = QSizeF(lastLine.horizontalAdvance(),
                                 (lastLine.y() + lastBlock.layout()->position().y() + lastLine.ascent()) - (firstLine.y() + firstBlock.layout()->position().y() + firstLine.ascent()));
            } else {
                advance = QSizeF();
            }
        }
    }

    signalSizeChange(previousSize);
    updateType = UpdatePaintNode;
    q->update();
}

QQuickTextLine::QQuickTextLine()
    : QObject(), m_line(nullptr), m_height(0), m_lineOffset(0)
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

void QQuickTextLine::setFullLayoutTextLength(int length)
{
    m_fullLayoutTextLength = length;
}

int QQuickTextLine::number() const
{
    if (m_line)
        return m_line->lineNumber() + m_lineOffset;
    return 0;
}

qreal QQuickTextLine::implicitWidth() const
{
    if (m_line)
        return m_line->naturalTextWidth();
    return 0;
}

bool QQuickTextLine::isLast() const
{
    if (m_line && (m_line->textStart() + m_line->textLength()) == m_fullLayoutTextLength) {
        // Ensure that isLast will change if the user reduced the width of the line
        // so that the text no longer fits.
        return m_line->width() >= m_line->naturalTextWidth();
    }

    return false;
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

bool QQuickTextPrivate::isLineLaidOutConnected()
{
    Q_Q(QQuickText);
    IS_SIGNAL_CONNECTED(q, QQuickText, lineLaidOut, (QQuickTextLine *));
}

void QQuickTextPrivate::setupCustomLineGeometry(QTextLine &line, qreal &height, int fullLayoutTextLength, int lineOffset)
{
    Q_Q(QQuickText);

    if (!textLine)
        textLine.reset(new QQuickTextLine);
    textLine->setFullLayoutTextLength(fullLayoutTextLength);
    textLine->setLine(&line);
    textLine->setY(height);
    textLine->setHeight(0);
    textLine->setLineOffset(lineOffset);

    // use the text item's width by default if it has one and wrap is on or text must be aligned
    if (q->widthValid() && (q->wrapMode() != QQuickText::NoWrap ||
                            q->effectiveHAlign() != QQuickText::AlignLeft))
        textLine->setWidth(availableWidth());
    else
        textLine->setWidth(INT_MAX);
    if (lineHeight() != 1.0)
        textLine->setHeight((lineHeightMode() == QQuickText::FixedHeight) ? lineHeight() : line.height() * lineHeight());

    emit q->lineLaidOut(textLine.get());

    height += textLine->height();
}

void QQuickTextPrivate::elideFormats(
        const int start, const int length, int offset, QVector<QTextLayout::FormatRange> *elidedFormats)
{
    const int end = start + length;
    const QVector<QTextLayout::FormatRange> formats = layout.formats();
    for (int i = 0; i < formats.size(); ++i) {
        QTextLayout::FormatRange format = formats.at(i);
        const int formatLength = qMin(format.start + format.length, end) - qMax(format.start, start);
        if (formatLength > 0) {
            format.start = qMax(offset, format.start - start + offset);
            format.length = formatLength;
            elidedFormats->append(format);
        }
    }
}

QString QQuickTextPrivate::elidedText(qreal lineWidth, const QTextLine &line, const QTextLine *nextLine) const
{
    if (nextLine) {
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
            elideText[elideText.size() - 1] = elideChar;
            // Appending the elide character may push the line over the maximum width
            // in which case the elided text will need to be elided.
            QFontMetricsF metrics(layout.font());
            if (metrics.horizontalAdvance(elideChar) + line.naturalTextWidth() >= lineWidth)
                elideText = metrics.elidedText(elideText, Qt::TextElideMode(elideMode), lineWidth);
        }
        return elideText;
    }
}

void QQuickTextPrivate::clearFormats()
{
    layout.clearFormats();
    if (elideLayout)
        elideLayout->clearFormats();
}

/*!
    Lays out the QQuickTextPrivate::layout QTextLayout in the constraints of the QQuickText.

    Returns the size of the final text.  This can be used to position the text vertically (the text is
    already absolutely positioned horizontally).
*/

QRectF QQuickTextPrivate::setupTextLayout(qreal *const baseline)
{
    Q_Q(QQuickText);

    bool singlelineElide = elideMode != QQuickText::ElideNone && q->widthValid();
    bool multilineElide = elideMode == QQuickText::ElideRight
            && q->widthValid()
            && (q->heightValid() || maximumLineCountValid);

    if ((!requireImplicitSize || (implicitWidthValid && implicitHeightValid))
            && ((singlelineElide && availableWidth() <= 0.)
                || (multilineElide && q->heightValid() && availableHeight() <= 0.))) {
        // we are elided and we have a zero width or height
        widthExceeded = q->widthValid() && availableWidth() <= 0.;
        heightExceeded = q->heightValid() && availableHeight() <= 0.;

        if (!truncated) {
            truncated = true;
            emit q->truncatedChanged();
        }
        if (lineCount) {
            lineCount = 0;
            q->setFlag(QQuickItem::ItemObservesViewport, false);
            emit q->lineCountChanged();
        }

        if (qFuzzyIsNull(q->width())) {
            layout.setText(QString());
            textHasChanged = true;
        }

        QFontMetricsF fm(font);
        qreal height = (lineHeightMode() == QQuickText::FixedHeight) ? lineHeight() : qCeil(fm.height()) * lineHeight();
        *baseline = fm.ascent();
        return QRectF(0, 0, 0, height);
    }

    bool shouldUseDesignMetrics = renderType != QQuickText::NativeRendering;
    if (extra.isAllocated())
        extra->visibleImgTags.clear();
    layout.setCacheEnabled(true);
    QTextOption textOption = layout.textOption();
    if (textOption.alignment() != q->effectiveHAlign()
            || textOption.wrapMode() != QTextOption::WrapMode(wrapMode)
            || textOption.useDesignMetrics() != shouldUseDesignMetrics) {
        textOption.setAlignment(Qt::Alignment(q->effectiveHAlign()));
        textOption.setWrapMode(QTextOption::WrapMode(wrapMode));
        textOption.setUseDesignMetrics(shouldUseDesignMetrics);
        layout.setTextOption(textOption);
    }
    if (layout.font() != font)
        layout.setFont(font);

    lineWidth = (q->widthValid() || implicitWidthValid) && q->width() > 0
            ? q->width()
            : FLT_MAX;
    qreal maxHeight = q->heightValid() ? availableHeight() : FLT_MAX;

    const bool customLayout = isLineLaidOutConnected();
    const bool wasTruncated = truncated;

    bool canWrap = wrapMode != QQuickText::NoWrap && q->widthValid();

    bool horizontalFit = fontSizeMode() & QQuickText::HorizontalFit && q->widthValid();
    bool verticalFit = fontSizeMode() & QQuickText::VerticalFit
            && (q->heightValid() || (maximumLineCountValid && canWrap));

    const bool pixelSize = font.pixelSize() != -1;
    QString layoutText = layout.text();

    const qreal minimumSize = pixelSize
                            ? static_cast<qreal>(minimumPixelSize())
                            : minimumPointSize();
    qreal largeFont = pixelSize ? font.pixelSize() : font.pointSizeF();
    qreal smallFont = fontSizeMode() != QQuickText::FixedSize
                    ? qMin<qreal>(minimumSize, largeFont)
                    : largeFont;
    qreal scaledFontSize = largeFont;
    const qreal sizeFittingThreshold(0.01);

    bool widthChanged = false;
    widthExceeded = availableWidth() <= 0 && (singlelineElide || canWrap || horizontalFit);
    heightExceeded = availableHeight() <= 0 && (multilineElide || verticalFit);

    QRectF br;

    QFont scaledFont = font;

    int visibleCount = 0;
    bool elide;
    qreal height = 0;
    QString elideText;
    bool once = true;
    int elideStart = 0;
    int elideEnd = 0;
    bool noBreakLastLine = multilineElide && (wrapMode == QQuickText::Wrap || wrapMode == QQuickText::WordWrap);

    int eos = multilengthEos;

    // Repeated layouts with reduced font sizes or abbreviated strings may be required if the text
    // doesn't fit within the item dimensions,  or a binding to implicitWidth/Height changes
    // the item dimensions.
    for (;;) {
        if (!once) {
            if (pixelSize)
                scaledFont.setPixelSize(scaledFontSize);
            else
                scaledFont.setPointSizeF(scaledFontSize);
            if (layout.font() != scaledFont)
                layout.setFont(scaledFont);
        }

        layout.beginLayout();

        bool wrapped = false;
        bool truncateHeight = false;
        truncated = false;
        elide = false;
        int unwrappedLineCount = 1;
        const int maxLineCount = maximumLineCount();
        height = 0;
        qreal naturalHeight = 0;
        qreal previousHeight = 0;
        br = QRectF();

        QRectF unelidedRect;
        QTextLine line;
        for (visibleCount = 1; ; ++visibleCount) {
            line = layout.createLine();

            if (noBreakLastLine && visibleCount == maxLineCount)
                layout.engine()->option.setWrapMode(QTextOption::WrapAnywhere);
            if (customLayout) {
                setupCustomLineGeometry(line, naturalHeight, layoutText.size());
            } else {
                setLineGeometry(line, lineWidth, naturalHeight);
            }
            if (noBreakLastLine && visibleCount == maxLineCount)
                layout.engine()->option.setWrapMode(QTextOption::WrapMode(wrapMode));

            unelidedRect = br.united(line.naturalTextRect());

            // Elide the previous line if the accumulated height of the text exceeds the height
            // of the element.
            if (multilineElide && naturalHeight > maxHeight && visibleCount > 1) {
                elide = true;
                heightExceeded = true;
                if (eos != -1)  // There's an abbreviated string available, skip the rest as it's
                    break;      // all going to be discarded.

                truncated = true;
                truncateHeight = true;

                visibleCount -= 1;

                const QTextLine previousLine = layout.lineAt(visibleCount - 1);
                elideText = layoutText.at(line.textStart() - 1) != QChar::LineSeparator
                        ? elidedText(line.width(), previousLine, &line)
                        : elidedText(line.width(), previousLine);
                elideStart = previousLine.textStart();
                // elideEnd isn't required for right eliding.

                height = previousHeight;
                break;
            }

            const bool isLastLine = line.textStart() + line.textLength() >= layoutText.size();
            if (isLastLine) {
                if (singlelineElide && visibleCount == 1 && line.naturalTextWidth() > line.width()) {
                    // Elide a single previousLine of text if its width exceeds the element width.
                    elide = true;
                    widthExceeded = true;
                    if (eos != -1) // There's an abbreviated string available.
                        break;

                    truncated = true;
                    elideText = layout.engine()->elidedText(
                            Qt::TextElideMode(elideMode),
                            QFixed::fromReal(line.width()),
                            0,
                            line.textStart(),
                            line.textLength());
                    elideStart = line.textStart();
                    elideEnd = elideStart + line.textLength();
                } else {
                    br = unelidedRect;
                    height = naturalHeight;
                }
                break;
            } else {
                const bool wrappedLine = layoutText.at(line.textStart() + line.textLength() - 1) != QChar::LineSeparator;
                wrapped |= wrappedLine;

                if (!wrappedLine)
                    ++unwrappedLineCount;

                // Stop if the maximum number of lines has been reached
                if (visibleCount == maxLineCount) {
                    truncated = true;
                    heightExceeded |= wrapped;

                    if (multilineElide) {
                        elide = true;
                        if (eos != -1)  // There's an abbreviated string available
                            break;

                        const QTextLine nextLine = layout.createLine();
                        elideText = wrappedLine
                                ? elidedText(line.width(), line, &nextLine)
                                : elidedText(line.width(), line);
                        elideStart = line.textStart();
                        // elideEnd isn't required for right eliding.
                    } else {
                        br = unelidedRect;
                        height = naturalHeight;
                    }
                    break;
                }
            }
            br = unelidedRect;
            previousHeight = height;
            height = naturalHeight;
        }
        widthExceeded |= wrapped;

        // Save the implicit size of the text on the first layout only.
        if (once) {
            once = false;

            // If implicit sizes are required layout any additional lines up to the maximum line
            // count.
            if ((requireImplicitSize) && line.isValid() && unwrappedLineCount < maxLineCount) {
                // Layout the remainder of the wrapped lines up to maxLineCount to get the implicit
                // height.
                for (int lineCount = layout.lineCount(); lineCount < maxLineCount; ++lineCount) {
                    line = layout.createLine();
                    if (!line.isValid())
                        break;
                    if (layoutText.at(line.textStart() - 1) == QChar::LineSeparator)
                        ++unwrappedLineCount;
                    setLineGeometry(line, lineWidth, naturalHeight);
                }

                // Create the remainder of the unwrapped lines up to maxLineCount to get the
                // implicit width.
                const int eol = line.isValid()
                        ? line.textStart() + line.textLength()
                        : layoutText.size();
                if (eol < layoutText.size() && layoutText.at(eol) != QChar::LineSeparator)
                    line = layout.createLine();
                for (; line.isValid() && unwrappedLineCount <= maxLineCount; ++unwrappedLineCount)
                    line = layout.createLine();
            }
            layout.endLayout();

            const qreal naturalWidth = layout.maximumWidth();

            bool wasInLayout = internalWidthUpdate;
            internalWidthUpdate = true;
            q->setImplicitSize(naturalWidth + q->leftPadding() + q->rightPadding(), naturalHeight + qMax(lineHeightOffset(), 0) + q->topPadding() + q->bottomPadding());
            internalWidthUpdate = wasInLayout;

            // Update any variables that are dependent on the validity of the width or height.
            singlelineElide = elideMode != QQuickText::ElideNone && q->widthValid();
            multilineElide = elideMode == QQuickText::ElideRight
                    && q->widthValid()
                    && (q->heightValid() || maximumLineCountValid);
            canWrap = wrapMode != QQuickText::NoWrap && q->widthValid();

            horizontalFit = fontSizeMode() & QQuickText::HorizontalFit && q->widthValid();
            verticalFit = fontSizeMode() & QQuickText::VerticalFit
                    && (q->heightValid() || (maximumLineCountValid && canWrap));

            const qreal oldWidth = lineWidth;
            const qreal oldHeight = maxHeight;

            const qreal availWidth = availableWidth();
            const qreal availHeight = availableHeight();

            lineWidth = q->widthValid() && q->width() > 0 ? availWidth : naturalWidth;
            maxHeight = q->heightValid() ? availHeight : FLT_MAX;

            // If the width of the item has changed and it's possible the result of wrapping,
            // eliding, scaling has changed, or the text is not left aligned do another layout.
            if ((!qFuzzyCompare(lineWidth, oldWidth) || (widthExceeded && lineWidth > oldWidth))
                    && (singlelineElide || multilineElide || canWrap || horizontalFit
                        || q->effectiveHAlign() != QQuickText::AlignLeft)) {
                widthChanged = true;
                widthExceeded = lineWidth >= qMin(oldWidth, naturalWidth);
                heightExceeded = false;
                continue;
            }

            // If the height of the item has changed and it's possible the result of eliding,
            // line count truncation or scaling has changed, do another layout.
            if ((maxHeight < qMin(oldHeight, naturalHeight) || (heightExceeded && maxHeight > oldHeight))
                    && (multilineElide || (canWrap && maximumLineCountValid))) {
                widthExceeded = false;
                heightExceeded = false;
                continue;
            }

            // If the horizontal alignment is not left and the width was not valid we need to relayout
            // now that we know the maximum line width.
            if (!q->widthValid() && !implicitWidthValid && unwrappedLineCount > 1 && q->effectiveHAlign() != QQuickText::AlignLeft) {
                widthExceeded = false;
                heightExceeded = false;
                continue;
            }
        } else if (widthChanged) {
            widthChanged = false;
            if (line.isValid()) {
                for (int lineCount = layout.lineCount(); lineCount < maxLineCount; ++lineCount) {
                    line = layout.createLine();
                    if (!line.isValid())
                        break;
                    setLineGeometry(line, lineWidth, naturalHeight);
                }
            }
            layout.endLayout();

            bool wasInLayout = internalWidthUpdate;
            internalWidthUpdate = true;
            q->setImplicitHeight(naturalHeight + qMax(lineHeightOffset(), 0) + q->topPadding() + q->bottomPadding());
            internalWidthUpdate = wasInLayout;

            multilineElide = elideMode == QQuickText::ElideRight
                    && q->widthValid()
                    && (q->heightValid() || maximumLineCountValid);
            verticalFit = fontSizeMode() & QQuickText::VerticalFit
                    && (q->heightValid() || (maximumLineCountValid && canWrap));

            const qreal oldHeight = maxHeight;
            maxHeight = q->heightValid() ? availableHeight() : FLT_MAX;
            // If the height of the item has changed and it's possible the result of eliding,
            // line count truncation or scaling has changed, do another layout.
            if ((maxHeight < qMin(oldHeight, naturalHeight) || (heightExceeded && maxHeight > oldHeight))
                    && (multilineElide || (canWrap && maximumLineCountValid))) {
                widthExceeded = false;
                heightExceeded = false;
                continue;
            }
        } else {
            layout.endLayout();
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

        br.moveTop(0);

        // Find the advance of the text layout
        if (layout.lineCount() > 0) {
            QTextLine firstLine = layout.lineAt(0);
            QTextLine lastLine = layout.lineAt(layout.lineCount() - 1);
            advance = QSizeF(lastLine.horizontalAdvance(),
                             lastLine.y() - firstLine.y());
        } else {
            advance = QSizeF();
        }

        if (!horizontalFit && !verticalFit)
            break;

        // Can't find a better fit
        if (qFuzzyCompare(smallFont, largeFont))
            break;

        // Try and find a font size that better fits the dimensions of the element.
        if (horizontalFit) {
            if (unelidedRect.width() > lineWidth || (!verticalFit && wrapped)) {
                widthExceeded = true;
                largeFont = scaledFontSize;

                scaledFontSize = (smallFont + largeFont) / 2;

                continue;
            } else if (!verticalFit) {
                smallFont = scaledFontSize;

                // Check to see if the current scaledFontSize is acceptable
                if ((largeFont - smallFont) < sizeFittingThreshold)
                    break;

                scaledFontSize = (smallFont + largeFont) / 2;
            }
        }

        if (verticalFit) {
            if (truncateHeight || unelidedRect.height() > maxHeight) {
                heightExceeded = true;
                largeFont = scaledFontSize;

                scaledFontSize = (smallFont + largeFont) / 2;

            } else {
                smallFont = scaledFontSize;

                // Check to see if the current scaledFontSize is acceptable
                if ((largeFont - smallFont) < sizeFittingThreshold)
                    break;

                scaledFontSize = (smallFont + largeFont) / 2;
            }
        }
    }

    implicitWidthValid = true;
    implicitHeightValid = true;

    QFontInfo scaledFontInfo(scaledFont);
    if (fontInfo.weight() != scaledFontInfo.weight()
            || fontInfo.pixelSize() != scaledFontInfo.pixelSize()
            || fontInfo.italic() != scaledFontInfo.italic()
            || !qFuzzyCompare(fontInfo.pointSizeF(), scaledFontInfo.pointSizeF())
            || fontInfo.family() != scaledFontInfo.family()
            || fontInfo.styleName() != scaledFontInfo.styleName()) {
        fontInfo = scaledFontInfo;
        emit q->fontInfoChanged();
    }

    if (eos != multilengthEos)
        truncated = true;

    assignedFont = QFontInfo(font).family();

    if (elide) {
        if (!elideLayout) {
            elideLayout.reset(new QTextLayout);
            elideLayout->setCacheEnabled(true);
        }
        QTextEngine *engine = layout.engine();
        if (engine && engine->hasFormats()) {
            QVector<QTextLayout::FormatRange> formats;
            switch (elideMode) {
            case QQuickText::ElideRight:
                elideFormats(elideStart, elideText.size() - 1, 0, &formats);
                break;
            case QQuickText::ElideLeft:
                elideFormats(elideEnd - elideText.size() + 1, elideText.size() - 1, 1, &formats);
                break;
            case QQuickText::ElideMiddle: {
                const int index = elideText.indexOf(elideChar);
                if (index != -1) {
                    elideFormats(elideStart, index, 0, &formats);
                    elideFormats(
                            elideEnd - elideText.size() + index + 1,
                            elideText.size() - index - 1,
                            index + 1,
                            &formats);
                }
                break;
            }
            default:
                break;
            }
            elideLayout->setFormats(formats);
        }

        elideLayout->setFont(layout.font());
        elideLayout->setTextOption(layout.textOption());
        elideLayout->setText(elideText);
        elideLayout->beginLayout();

        QTextLine elidedLine = elideLayout->createLine();
        elidedLine.setPosition(QPointF(0, height));
        if (customLayout) {
            setupCustomLineGeometry(elidedLine, height, elideText.size(), visibleCount - 1);
        } else {
            setLineGeometry(elidedLine, lineWidth, height);
        }
        elideLayout->endLayout();

        br = br.united(elidedLine.naturalTextRect());

        if (visibleCount == 1)
            layout.clearLayout();
    } else {
        elideLayout.reset();
    }

    QTextLine firstLine = visibleCount == 1 && elideLayout
            ? elideLayout->lineAt(0)
            : layout.lineAt(0);
    if (firstLine.isValid())
        *baseline = firstLine.y() + firstLine.ascent();

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

    if (extra.isAllocated() && extra->imgTags.isEmpty()) {
        line.setPosition(QPointF(line.position().x(), height));
        height += (lineHeightMode() == QQuickText::FixedHeight) ? lineHeight() : line.height() * lineHeight();
        return;
    }

    qreal textTop = 0;
    qreal textHeight = line.height();
    qreal totalLineHeight = textHeight;

    QList<QQuickStyledTextImgTag *> imagesInLine;

    if (extra.isAllocated()) {
        for (QQuickStyledTextImgTag *image : std::as_const(extra->imgTags)) {
            if (image->position >= line.textStart() &&
                image->position < line.textStart() + line.textLength()) {

                if (!image->pix) {
                    const QQmlContext *context = qmlContext(q);
                    const QUrl url = context->resolvedUrl(q->baseUrl()).resolved(image->url);
                    image->pix.reset(new QQuickPixmap(context->engine(), url, QRect(), image->size * devicePixelRatio()));

                    if (image->pix->isLoading()) {
                        image->pix->connectFinished(q, SLOT(imageDownloadFinished()));
                    } else if (image->pix->isReady()) {
                        if (!image->size.isValid()) {
                            image->size = image->pix->implicitSize();
                            // if the size of the image was not explicitly set, we need to
                            // call updateLayout() once again.
                            needToUpdateLayout = true;
                        }
                    } else if (image->pix->isError()) {
                        qmlWarning(q) << image->pix->error();
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
    }

    for (QQuickStyledTextImgTag *image : std::as_const(imagesInLine)) {
        totalLineHeight = qMax(totalLineHeight, textTop + image->pos.y() + image->size.height());
        const int leadX = line.cursorToX(image->position);
        const int trailX = line.cursorToX(image->position, QTextLine::Trailing);
        const bool rtl = trailX < leadX;
        image->pos.setX(leadX + (rtl ? (-image->offset - image->size.width()) : image->offset));
        image->pos.setY(image->pos.y() + height + textTop);
        extra->visibleImgTags << image;
    }

    line.setPosition(QPointF(line.position().x(), height + textTop));
    height += (lineHeightMode() == QQuickText::FixedHeight) ? lineHeight() : totalLineHeight * lineHeight();
}

/*!
    Returns the y offset when aligning text with a non-1.0 lineHeight
*/
int QQuickTextPrivate::lineHeightOffset() const
{
    QFontMetricsF fm(font);
    qreal fontHeight = qCeil(fm.height());  // QScriptLine and therefore QTextLine rounds up
    return lineHeightMode() == QQuickText::FixedHeight ? fontHeight - lineHeight()
                                                       : (1.0 - lineHeight()) * fontHeight;
}

/*!
    Ensures the QQuickTextPrivate::doc variable is set to a valid text document
*/
void QQuickTextPrivate::ensureDoc()
{
    if (!extra.isAllocated() || !extra->doc) {
        Q_Q(QQuickText);
        extra.value().doc = new QTextDocument(q);
        auto *doc = extra->doc;
        extra->imageHandler = new QQuickTextImageHandler(doc);
        doc->documentLayout()->registerHandler(QTextFormat::ImageObject, extra->imageHandler);
        doc->setPageSize(QSizeF(0, 0));
        doc->setDocumentMargin(0);
        const QQmlContext *context = qmlContext(q);
        doc->setBaseUrl(context ? context->resolvedUrl(q->baseUrl()) : q->baseUrl());
    }
}

void QQuickTextPrivate::updateDocumentText()
{
    ensureDoc();
#if QT_CONFIG(textmarkdownreader)
    if (markdownText)
        extra->doc->setMarkdown(text);
    else
#endif
#if QT_CONFIG(texthtmlparser)
        extra->doc->setHtml(text);
#else
        extra->doc->setPlainText(text);
#endif
    rightToLeftText = extra->doc->toPlainText().isRightToLeft();
}

qreal QQuickTextPrivate::devicePixelRatio() const
{
    return (window ? window->effectiveDevicePixelRatio() : qApp->devicePixelRatio());
}

/*!
    \qmltype Text
    \nativetype QQuickText
    \inqmlmodule QtQuick
    \ingroup qtquick-visual
    \inherits Item
    \brief Specifies how to add formatted text to a scene.

    Text items can display both plain and rich text. For example, you can define
    red text with a specific font and size like this:

    \qml
    Text {
        text: "Hello World!"
        font.family: "Helvetica"
        font.pointSize: 24
        color: "red"
    }
    \endqml

    Use HTML-style markup or Markdown to define rich text:

    \if defined(onlinedocs)
      \tab {build-qt-app}{tab-html}{HTML-style}{checked}
      \tab {build-qt-app}{tab-md}{Markdown}{}
      \tabcontent {tab-html}
    \else
      \section1 Using HTML-style
    \endif
    \qml
    Text {
        text: "<b>Hello</b> <i>World!</i>"
    }
    \endqml
    \if defined(onlinedocs)
      \endtabcontent
      \tabcontent {tab-md}
    \else
      \section1 Using Markdown
    \endif
    \qml
    Text {
        text: "**Hello** *World!*"
    }
    \endqml
    \if defined(onlinedocs)
      \endtabcontent
    \endif

    \image declarative-text.png

    If height and width are not explicitly set, Text will try to determine how
    much room is needed and set it accordingly. Unless \l wrapMode is set, it
    will always prefer width to height (all text will be placed on a single
    line).

    To fit a single line of plain text to a set width, you can use the \l elide
    property.

    Note that the \l{Supported HTML Subset} is limited. Also, if the text
    contains HTML img tags that load remote images, the text is reloaded.

    Text provides read-only text. For editable text, see \l TextEdit.

    \sa {Qt Quick Examples - Text#Fonts}{Fonts example}
*/
QQuickText::QQuickText(QQuickItem *parent)
: QQuickImplicitSizeItem(*(new QQuickTextPrivate), parent)
{
    Q_D(QQuickText);
    d->init();
}

QQuickText::QQuickText(QQuickTextPrivate &dd, QQuickItem *parent)
: QQuickImplicitSizeItem(dd, parent)
{
    Q_D(QQuickText);
    d->init();
}

QQuickText::~QQuickText()
{
    Q_D(QQuickText);
    if (d->extra.isAllocated()) {
        qDeleteAll(d->extra->pixmapsInProgress);
        d->extra->pixmapsInProgress.clear();
    }
}

/*!
  \qmlproperty bool QtQuick::Text::clip
  This property holds whether the text is clipped.

  Note that if the text does not fit in the bounding rectangle, it will be abruptly chopped.

  If you want to display potentially long text in a limited space, you probably want to use \c elide instead.
*/

/*!
    \qmlsignal QtQuick::Text::lineLaidOut(object line)

    This signal is emitted for each line of text that is laid out during the layout
    process in plain text or styled text mode. It is not emitted in rich text mode.
    The specified \a line object provides more details about the line that
    is currently being laid out.

    This gives the opportunity to position and resize a line as it is being laid out.
    It can for example be used to create columns or lay out text around objects.

    The properties of the specified \a line object are:

    \table
    \header
        \li Property name
        \li Description
    \row
        \li number (read-only)
        \li Line number, starts with zero.
    \row
        \li x
        \li Specifies the line's x position inside the \c Text element.
    \row
        \li y
        \li Specifies the line's y position inside the \c Text element.
    \row
        \li width
        \li Specifies the width of the line.
    \row
        \li height
        \li Specifies the height of the line.
    \row
        \li implicitWidth (read-only)
        \li The width that the line would naturally occupy based on its contents,
            not taking into account any modifications made to \e width.
    \row
        \li isLast (read-only)
        \li Whether the line is the last. This property can change if you
            set the \e width property to a different value.
    \endtable

    For example, this will move the first 5 lines of a Text item by 100 pixels to the right:
    \code
    onLineLaidOut: (line)=> {
        if (line.number < 5) {
            line.x = line.x + 100
            line.width = line.width - 100
        }
    }
    \endcode

    The following example will allow you to position an item at the end of the last line:
    \code
    onLineLaidOut: (line)=> {
        if (line.isLast) {
            lastLineMarker.x = line.x + line.implicitWidth
            lastLineMarker.y = line.y + (line.height - lastLineMarker.height) / 2
        }
    }
    \endcode
*/

/*!
    \qmlsignal QtQuick::Text::linkActivated(string link)

    This signal is emitted when the user clicks on a link embedded in the text.
    The link must be in rich text or HTML format and the
    \a link string provides access to the particular link.

    \snippet qml/text/onLinkActivated.qml 0

    The example code will display the text
    "See the \l{http://qt-project.org}{Qt Project website}."

    Clicking on the highlighted link will output
    \tt{http://qt-project.org link activated} to the console.
*/

/*!
    \qmlproperty string QtQuick::Text::font.family

    Sets the family name of the font.

    The family name is case insensitive and may optionally include a foundry
    name, for example "Helvetica [Cronyx]".
    If the family is available from more than one foundry and the foundry isn't specified, an arbitrary foundry is chosen.
    If the family isn't available a family will be set using the font matching algorithm.
*/

/*!
    \qmlproperty string QtQuick::Text::font.styleName
    \since 5.6

    Sets the style name of the font.

    The style name is case insensitive. If set, the font will be matched against style name instead
    of the font properties \l font.weight, \l font.bold and \l font.italic.
*/

/*!
    \qmlproperty bool QtQuick::Text::font.bold

    Sets whether the font weight is bold.
*/

/*!
    \qmlproperty int QtQuick::Text::font.weight

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
    Text { text: "Hello"; font.weight: Font.DemiBold }
    \endqml
*/

/*!
    \qmlproperty bool QtQuick::Text::font.italic

    Sets whether the font has an italic style.
*/

/*!
    \qmlproperty bool QtQuick::Text::font.underline

    Sets whether the text is underlined.
*/

/*!
    \qmlproperty bool QtQuick::Text::font.strikeout

    Sets whether the font has a strikeout style.
*/

/*!
    \qmlproperty real QtQuick::Text::font.pointSize

    Sets the font size in points. The point size must be greater than zero.
*/

/*!
    \qmlproperty int QtQuick::Text::font.pixelSize

    Sets the font size in pixels.

    Using this function makes the font device dependent.
    Use \c pointSize to set the size of the font in a device independent manner.
*/

/*!
    \qmlproperty real QtQuick::Text::font.letterSpacing

    Sets the letter spacing for the font.

    Letter spacing changes the default spacing between individual letters in the font.
    A positive value increases the letter spacing by the corresponding pixels; a negative value decreases the spacing.
*/

/*!
    \qmlproperty real QtQuick::Text::font.wordSpacing

    Sets the word spacing for the font.

    Word spacing changes the default spacing between individual words.
    A positive value increases the word spacing by a corresponding amount of pixels,
    while a negative value decreases the inter-word spacing accordingly.
*/

/*!
    \qmlproperty enumeration QtQuick::Text::font.capitalization

    Sets the capitalization for the text.

    \value Font.MixedCase       the normal case: no capitalization change is applied
    \value Font.AllUppercase    alters the text to be rendered in all uppercase type
    \value Font.AllLowercase    alters the text to be rendered in all lowercase type
    \value Font.SmallCaps       alters the text to be rendered in small-caps type
    \value Font.Capitalize      alters the text to be rendered with the first character of
                                each word as an uppercase character

    \qml
    Text { text: "Hello"; font.capitalization: Font.AllLowercase }
    \endqml
*/

/*!
    \qmlproperty enumeration QtQuick::Text::font.hintingPreference
    \since 5.8

    Sets the preferred hinting on the text. This is a hint to the underlying text rendering system
    to use a certain level of hinting, and has varying support across platforms. See the table in
    the documentation for QFont::HintingPreference for more details.

    \note This property only has an effect when used together with render type Text.NativeRendering.

    \value Font.PreferDefaultHinting    Use the default hinting level for the target platform.
    \value Font.PreferNoHinting         If possible, render text without hinting the outlines
           of the glyphs. The text layout will be typographically accurate, using the same metrics
           as are used, for example, when printing.
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
    Text { text: "Hello"; renderType: Text.NativeRendering; font.hintingPreference: Font.PreferVerticalHinting }
    \endqml
*/

/*!
    \qmlproperty bool QtQuick::Text::font.kerning
    \since 5.10

    Enables or disables the kerning OpenType feature when shaping the text. Disabling this may
    improve performance when creating or changing the text, at the expense of some cosmetic
    features. The default value is true.

    \qml
    Text { text: "OATS FLAVOUR WAY"; font.kerning: false }
    \endqml
*/

/*!
    \qmlproperty bool QtQuick::Text::font.preferShaping
    \since 5.10

    Sometimes, a font will apply complex rules to a set of characters in order to
    display them correctly. In some writing systems, such as Brahmic scripts, this is
    required in order for the text to be legible, but in for example Latin script, it is merely
    a cosmetic feature. Setting the \c preferShaping property to false will disable all
    such features when they are not required, which will improve performance in most cases.

    The default value is true.

    \qml
    Text { text: "Some text"; font.preferShaping: false }
    \endqml
*/

/*!
    \qmlproperty object QtQuick::Text::font.variableAxes
    \since 6.7

//! [qml-font-variable-axes]
    Applies floating point values to variable axes in variable fonts.

    Variable fonts provide a way to store multiple variations (with different weights, widths
    or styles) in the same font file. The variations are given as floating point values for
    a pre-defined set of parameters, called "variable axes". Specific instances are typically
    given names by the font designer, and, in Qt, these can be selected using setStyleName()
    just like traditional sub-families.

    In some cases, it is also useful to provide arbitrary values for the different axes. For
    instance, if a font has a Regular and Bold sub-family, you may want a weight in-between these.
    You could then manually request this by supplying a custom value for the "wght" axis in the
    font.

    \qml
        Text {
            text: "Foobar"
            font.family: "MyVariableFont"
            font.variableAxes: { "wght": (Font.Normal + Font.Bold) / 2.0 }
        }
    \endqml

    If the "wght" axis is supported by the font and the given value is within its defined range,
    a font corresponding to the weight 550.0 will be provided.

    There are a few standard axes than many fonts provide, such as "wght" (weight), "wdth" (width),
    "ital" (italic) and "opsz" (optical size). They each have indivdual ranges defined in the font
    itself. For instance, "wght" may span from 100 to 900 (QFont::Thin to QFont::Black) whereas
    "ital" can span from 0 to 1 (from not italic to fully italic).

    A font may also choose to define custom axes; the only limitation is that the name has to
    meet the requirements for a QFont::Tag (sequence of four latin-1 characters.)

    By default, no variable axes are set.

    \note On Windows, variable axes are not supported if the optional GDI font backend is in use.

    \sa QFont::setVariableAxis()
//! [qml-font-variable-axes]
*/


/*!
    \qmlproperty object QtQuick::Text::font.features
    \since 6.6

//! [qml-font-features]
    Applies integer values to specific OpenType features when shaping the text based on the contents
    in \a features. This provides advanced access to the font shaping process, and can be used
    to support font features that are otherwise not covered in the API.

    The font features are represented by a map from four-letter tags to integer values. This integer
    value passed along with the tag in most cases represents a boolean value: A zero value means the
    feature is disabled, and a non-zero value means it is enabled. For certain font features,
    however, it may have other interpretations. For example, when applied to the \c salt feature, the
    value is an index that specifies the stylistic alternative to use.

    For example, the \c frac font feature will convert diagonal fractions separated with a slash
    (such as \c 1/2) with a different representation. Typically this will involve baking the full
    fraction into a single character width (such as \c ½).

    If a font supports the \c frac feature, then it can be enabled in the shaper as in the following
    code:

    \qml
    Text {
        text: "One divided by two is 1/2"
        font.family: "MyFractionFont"
        font.features: { "frac": 1 }
    }
    \endqml

    Multiple features can be assigned values in the same mapping. For instance,
    if you would like to also disable kerning for the font, you can explicitly
    disable this as follows:

    \qml
    Text {
        text: "One divided by two is 1/2"
        font.family: "MyFractionFont"
        font.features: { "frac": 1, "kern": 0 }
    }
    \endqml

    You can also collect the font properties in an object:

    \qml
    Text {
        text: "One divided by two is 1/2"
        font: {
            family: "MyFractionFont"
            features: { "frac": 1, "kern": 0 }
        }
    }
    \endqml

    \note By default, Qt will enable and disable certain font features based on other font
    properties. In particular, the \c kern feature will be enabled/disabled depending on the
    \l font.kerning property of the QFont. In addition, all ligature features (\c liga, \c clig,
    \c dlig, \c hlig) will be disabled if a \l font.letterSpacing is set, but only for writing
    systems where the use of ligature is cosmetic. For writing systems where ligatures are required,
    the features will remain in their default state. The values set using \c font.features will
    override the default behavior. If, for instance, \c{"kern"} is set to 1, then kerning will
    always be enabled, regardless of whether the \l font.kerning property is set to false. Similarly,
    if it is set to \c 0, it will always be disabled.

    \sa QFont::setFeature()
//! [qml-font-features]
*/

/*!
    \qmlproperty bool QtQuick::Text::font.contextFontMerging
    \since 6.8

//! [qml-font-context-font-merging]
    If the selected font does not contain a certain character, Qt automatically chooses a
    similar-looking fallback font that contains the character. By default this is done on a
    character-by-character basis.

    This means that in certain uncommon cases, many different fonts may be used to represent one
    string of text even if it's in the same script. Setting \c contextFontMerging to true will try
    finding the fallback font that matches the largest subset of the input string instead. This
    will be more expensive for strings where missing glyphs occur, but may give more consistent
    results. By default, \c contextFontMerging is \c{false}.

    \sa QFont::StyleStrategy
//! [qml-font-context-font-merging]
*/

/*!
    \qmlproperty bool QtQuick::Text::font.preferTypoLineMetrics
    \since 6.8

//! [qml-font-prefer-typo-line-metrics] For compatibility reasons, OpenType fonts contain two
    competing sets of the vertical line metrics that provide the \l{QFontMetricsF::ascent()}{ascent},
    \l{QFontMetricsF::descent()}{descent} and \l{QFontMetricsF::leading()}{leading} of the font. These
    are often referred to as the
    \l{https://learn.microsoft.com/en-us/typography/opentype/spec/os2#uswinascent}{win} (Windows)
    metrics and the \l{https://learn.microsoft.com/en-us/typography/opentype/spec/os2#sta}{typo}
    (typographical) metrics. While the specification recommends using the \c typo metrics for line
    spacing, many applications prefer the \c win metrics unless the \c{USE_TYPO_METRICS} flag is set in
    the \l{https://learn.microsoft.com/en-us/typography/opentype/spec/os2#fsselection}{fsSelection}
    field of the font. For backwards-compatibility reasons, this is also the case for Qt applications.
    This is not an issue for fonts that set the \c{USE_TYPO_METRICS} flag to indicate that the \c{typo}
    metrics are valid, nor for fonts where the \c{win} metrics and \c{typo} metrics match up. However,
    for certain fonts the \c{win} metrics may be larger than the preferable line spacing and the
    \c{USE_TYPO_METRICS} flag may be unset by mistake. For such fonts, setting
    \c{font.preferTypoLineMetrics} may give superior results.

    By default, \c preferTypoLineMetrics is \c{false}.

    \sa QFont::StyleStrategy
//! [qml-font-prefer-typo-line-metrics]
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

    if (!antialiasing())
        d->font.setStyleStrategy(QFont::NoAntialias);

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
        d->implicitWidthValid = false;
        d->implicitHeightValid = false;
        d->updateLayout();
    }

    emit fontChanged(d->sourceFont);
}

void QQuickText::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickText);
    Q_UNUSED(value);
    switch (change) {
    case ItemAntialiasingHasChanged:
        if (!antialiasing())
            d->font.setStyleStrategy(QFont::NoAntialias);
        else
            d->font.setStyleStrategy(QFont::PreferAntialias);
        d->implicitWidthValid = false;
        d->implicitHeightValid = false;
        d->updateLayout();
        break;

    case ItemDevicePixelRatioHasChanged:
        {
            bool needUpdateLayout = false;
            if (d->containsUnscalableGlyphs) {
                // Native rendering optimizes for a given pixel grid, so its results must not be scaled.
                // Text layout code respects the current device pixel ratio automatically, we only need
                // to rerun layout after the ratio changed.
                // Changes of implicit size should be minimal; they are hard to avoid.
                d->implicitWidthValid = false;
                d->implicitHeightValid = false;
                needUpdateLayout = true;
            }

            if (d->extra.isAllocated()) {
                // check if we have scalable inline images with explicit size set, which should be reloaded
                for (QQuickStyledTextImgTag *image : std::as_const(d->extra->visibleImgTags)) {
                    if (image->size.isValid() && QQuickPixmap::isScalableImageFormat(image->url)) {
                        image->pix.reset();
                        needUpdateLayout = true;
                    }
                }
            }

            if (needUpdateLayout)
                d->updateLayout();
        }
        break;

    default:
        break;
    }
    QQuickItem::itemChange(change, value);
}

/*!
    \qmlproperty string QtQuick::Text::text

    The text to display. Text supports both plain and rich text strings.

    The item will try to automatically determine whether the text should
    be treated as styled text. This determination is made using Qt::mightBeRichText().
    However, detection of Markdown is not automatic.

    \sa textFormat
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

    d->markdownText = d->format == MarkdownText;
    d->richText = d->format == RichText || d->markdownText;
    d->styledText = d->format == StyledText || (d->format == AutoText && Qt::mightBeRichText(n));
    d->text = n;
    if (isComponentComplete()) {
        if (d->richText) {
            d->updateDocumentText();
        } else {
            d->clearFormats();
            d->rightToLeftText = d->text.isRightToLeft();
        }
        d->determineHorizontalAlignment();
    }
    d->textHasChanged = true;
    d->implicitWidthValid = false;
    d->implicitHeightValid = false;

    if (d->extra.isAllocated()) {
        qDeleteAll(d->extra->imgTags);
        d->extra->imgTags.clear();
    }
    setFlag(QQuickItem::ItemObservesViewport, n.size() > QQuickTextPrivate::largeTextSizeThreshold);
    d->updateLayout();
    setAcceptHoverEvents(d->richText || d->styledText);
    emit textChanged(d->text);
}

/*!
    \qmlproperty color QtQuick::Text::color

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
    \qmlproperty color QtQuick::Text::linkColor

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
    if (isComponentComplete()) {
        d->updateType = QQuickTextPrivate::UpdatePaintNode;
        update();
    }
    emit linkColorChanged();
}

/*!
    \qmlproperty enumeration QtQuick::Text::style

    Set an additional text style.

    Supported text styles are:

    \value Text.Normal - the default
    \value Text.Outline
    \value Text.Raised
    \value Text.Sunken

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
    \qmlproperty color QtQuick::Text::styleColor

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
    \qmlproperty enumeration QtQuick::Text::horizontalAlignment
    \qmlproperty enumeration QtQuick::Text::verticalAlignment
    \qmlproperty enumeration QtQuick::Text::effectiveHorizontalAlignment

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
#if QT_CONFIG(im)
        bool alignToRight = text.isEmpty() ? QGuiApplication::inputMethod()->inputDirection() == Qt::RightToLeft : rightToLeftText;
#else
        bool alignToRight = rightToLeftText;
#endif
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

    if (isComponentComplete())
        d->updateLayout();

    emit verticalAlignmentChanged(align);
}

/*!
    \qmlproperty enumeration QtQuick::Text::wrapMode

    Set this property to wrap the text to the Text item's width.  The text will only
    wrap if an explicit width has been set.  wrapMode can be one of:

    \value Text.NoWrap
       (default) no wrapping will be performed. If the text contains
       insufficient newlines, then \l contentWidth will exceed a set width.
    \value Text.WordWrap
        wrapping is done on word boundaries only. If a word is too long,
        \l contentWidth will exceed a set width.
    \value Text.WrapAnywhere
        wrapping is done at any point on a line, even if it occurs in the middle of a word.
    \value Text.Wrap
        if possible, wrapping occurs at a word boundary; otherwise it will occur
        at the appropriate point on the line, even in the middle of a word.
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
    \qmlproperty int QtQuick::Text::lineCount

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
    \qmlproperty bool QtQuick::Text::truncated

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
    \qmlproperty int QtQuick::Text::maximumLineCount

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
        d->implicitHeightValid = false;
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
    \qmlproperty enumeration QtQuick::Text::textFormat

    The way the \l text property should be displayed.

    Supported text formats are:

    \value Text.AutoText        (default) detected via the Qt::mightBeRichText() heuristic
    \value Text.PlainText       all styling tags are treated as plain text
    \value Text.StyledText      optimized basic rich text as in HTML 3.2
    \value Text.RichText        \l {Supported HTML Subset} {a subset of HTML 4}
    \value Text.MarkdownText    \l {https://commonmark.org/help/}{CommonMark} plus the
                                \l {https://guides.github.com/features/mastering-markdown/}{GitHub}
                                extensions for tables and task lists (since 5.14)

    If the text format is \c Text.AutoText, the Text item
    will automatically determine whether the text should be treated as
    styled text.  This determination is made using Qt::mightBeRichText(),
    which can detect the presence of an HTML tag on the first line of text,
    but cannot distinguish Markdown from plain text.

    \c Text.StyledText is an optimized format supporting some basic text
    styling markup, in the style of HTML 3.2:

    \code
    <b></b> - bold
    <del></del> - strike out (removed content)
    <s></s> - strike out (no longer accurate or no longer relevant content)
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
    All entities
    \endcode

    \c Text.StyledText parser is strict, requiring tags to be correctly nested.

    \table
    \row
    \li
    \snippet qml/text/textFormats.qml 0
    \li \image declarative-textformat.png
    \endtable

    \c Text.RichText supports a larger subset of HTML 4, as described on the
    \l {Supported HTML Subset} page. You should prefer using \c Text.PlainText,
    \c Text.StyledText or \c Text.MarkdownText instead, as they offer better performance.

    \note With \c Text.MarkdownText, and with the supported subset of HTML,
    some decorative elements are not rendered as they would be in a web browser:
    \list
    \li code blocks use the \l {QFontDatabase::FixedFont}{default monospace font} but without a surrounding highlight box
    \li block quotes are indented, but there is no vertical line alongside the quote
    \endlist
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
    d->markdownText = format == MarkdownText;
    d->richText = format == RichText || d->markdownText;
    d->styledText = format == StyledText || (format == AutoText && Qt::mightBeRichText(d->text));

    if (isComponentComplete()) {
        if (!wasRich && d->richText) {
            d->updateDocumentText();
        } else {
            d->clearFormats();
            d->rightToLeftText = d->text.isRightToLeft();
            d->textHasChanged = true;
        }
        d->determineHorizontalAlignment();
    }
    d->updateLayout();
    setAcceptHoverEvents(d->richText || d->styledText);
    setAcceptedMouseButtons(d->richText || d->styledText ? Qt::LeftButton : Qt::NoButton);

    emit textFormatChanged(d->format);
}

/*!
    \qmlproperty enumeration QtQuick::Text::elide

    Set this property to elide parts of the text fit to the Text item's width.
    The text will only elide if an explicit width has been set.

    This property cannot be used with rich text.

    Eliding can be:

    \value Text.ElideNone  - the default
    \value Text.ElideLeft
    \value Text.ElideMiddle
    \value Text.ElideRight

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
    \qmlproperty url QtQuick::Text::baseUrl

    This property specifies a base URL that is used to resolve relative URLs
    within the text.

    Urls are resolved to be within the same directory as the target of the base
    URL meaning any portion of the path after the last '/' will be ignored.

    \table
    \header \li Base URL \li Relative URL \li Resolved URL
    \row \li http://qt-project.org/ \li images/logo.png \li http://qt-project.org/images/logo.png
    \row \li http://qt-project.org/index.html \li images/logo.png \li http://qt-project.org/images/logo.png
    \row \li http://qt-project.org/content \li images/logo.png \li http://qt-project.org/content/images/logo.png
    \row \li http://qt-project.org/content/ \li images/logo.png \li http://qt-project.org/content/images/logo.png
    \row \li http://qt-project.org/content/index.html \li images/logo.png \li http://qt-project.org/content/images/logo.png
    \row \li http://qt-project.org/content/index.html \li ../images/logo.png \li http://qt-project.org/images/logo.png
    \row \li http://qt-project.org/content/index.html \li /images/logo.png \li http://qt-project.org/images/logo.png
    \endtable

    The default value is the url of the QML file instantiating the Text item.
*/

QUrl QQuickText::baseUrl() const
{
    Q_D(const QQuickText);
    if (!d->extra.isAllocated() || d->extra->baseUrl.isEmpty()) {
        if (QQmlContext *context = qmlContext(this))
            return context->baseUrl();
        else
            return QUrl();
    } else {
        return d->extra->baseUrl;
    }
}

void QQuickText::setBaseUrl(const QUrl &url)
{
    Q_D(QQuickText);
    if (baseUrl() != url) {
        d->extra.value().baseUrl = url;

        if (d->richText) {
            d->ensureDoc();
            d->extra->doc->setBaseUrl(url);
        }
        if (d->styledText) {
            d->textHasChanged = true;
            if (d->extra.isAllocated()) {
                qDeleteAll(d->extra->imgTags);
                d->extra->imgTags.clear();
            }
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

/*!
    Returns the extents of the text after layout.
    If the \l style() is not \c Text.Normal, a margin is added to ensure
    that the rendering effect will fit within this rectangle.

    \sa contentWidth(), contentHeight(), clipRect()
*/
QRectF QQuickText::boundingRect() const
{
    Q_D(const QQuickText);

    QRectF rect = d->layedOutTextRect;
    rect.moveLeft(QQuickTextUtil::alignedX(rect.width(), width(), effectiveHAlign()));
    rect.moveTop(QQuickTextUtil::alignedY(rect.height() + d->lineHeightOffset(), height(), d->vAlign));

    if (d->style != Normal)
        rect.adjust(-1, 0, 1, 2);
    // Could include font max left/right bearings to either side of rectangle.

    return rect;
}

/*!
    Returns a rectangular area slightly larger than what is currently visible
    in \l viewportItem(); otherwise, the rectangle \c (0, 0, width, height).
    The text will be clipped to fit if \l clip is \c true.

    \note If the \l style is not \c Text.Normal, the clip rectangle is adjusted
    to be slightly larger, to limit clipping of the outline effect at the edges.
    But it still looks better to set \l clip to \c false in that case.

    \sa contentWidth(), contentHeight(), boundingRect()
*/
QRectF QQuickText::clipRect() const
{
    Q_D(const QQuickText);

    QRectF rect = QQuickImplicitSizeItem::clipRect();
    if (d->style != Normal)
        rect.adjust(-1, 0, 1, 2);
    return rect;
}

/*! \internal */
void QQuickText::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickText);
    if (d->text.isEmpty()) {
        QQuickItem::geometryChange(newGeometry, oldGeometry);
        return;
    }

    bool widthChanged = newGeometry.width() != oldGeometry.width();
    bool heightChanged = newGeometry.height() != oldGeometry.height();
    bool wrapped = d->wrapMode != QQuickText::NoWrap;
    bool elide = d->elideMode != QQuickText::ElideNone;
    bool scaleFont = d->fontSizeMode() != QQuickText::FixedSize && (widthValid() || heightValid());
    bool verticalScale = (d->fontSizeMode() & QQuickText::VerticalFit) && heightValid();

    bool widthMaximum = newGeometry.width() >= oldGeometry.width() && !d->widthExceeded;
    bool heightMaximum = newGeometry.height() >= oldGeometry.height() && !d->heightExceeded;

    bool verticalPositionChanged = heightChanged && d->vAlign != AlignTop;

    if ((!widthChanged && !heightChanged) || d->internalWidthUpdate)
        goto geomChangeDone;

    if ((effectiveHAlign() != QQuickText::AlignLeft && widthChanged) || verticalPositionChanged) {
        // If the width has changed and we're not left aligned do an update so the text is
        // repositioned even if a full layout isn't required. And the same for vertical.
        d->updateType = QQuickTextPrivate::UpdatePaintNode;
        update();
    }

    if (!wrapped && !elide && !scaleFont && !verticalPositionChanged)
        goto geomChangeDone; // left aligned unwrapped text without eliding never needs relayout

    if (elide // eliding and dimensions were and remain invalid;
            && ((widthValid() && oldGeometry.width() <= 0 && newGeometry.width() <= 0)
            || (heightValid() && oldGeometry.height() <= 0 && newGeometry.height() <= 0))) {
        goto geomChangeDone;
    }

    if (widthMaximum && heightMaximum && !d->isLineLaidOutConnected() && !verticalPositionChanged)  // Size is sufficient and growing.
        goto geomChangeDone;

    if (!(widthChanged || widthMaximum) && !d->isLineLaidOutConnected()) { // only height has changed
        if (!verticalPositionChanged) {
            if (newGeometry.height() > oldGeometry.height()) {
                if (!d->heightExceeded && !qFuzzyIsNull(oldGeometry.height())) {
                    // Height is adequate and growing, and it wasn't 0 previously.
                    goto geomChangeDone;
                }
                if (d->lineCount == d->maximumLineCount())  // Reached maximum line and height is growing.
                    goto geomChangeDone;
            } else if (newGeometry.height() < oldGeometry.height()) {
                if (d->lineCount < 2 && !verticalScale && newGeometry.height() > 0)  // A single line won't be truncated until the text is 0 height.
                    goto geomChangeDone;

                if (!verticalScale // no scaling, no eliding, and either unwrapped, or no maximum line count.
                        && d->elideMode != QQuickText::ElideRight
                        && !(d->maximumLineCountValid && d->widthExceeded)) {
                    goto geomChangeDone;
                }
            }
        }
    } else if (!heightChanged && widthMaximum) {
        if (oldGeometry.width() > 0) {
            // no change to height, width is adequate and wasn't 0 before
            // (old width could also be negative if it was 0 and the margins
            // were set)
            goto geomChangeDone;
        }
    }

    if (d->updateOnComponentComplete || d->textHasChanged) {
        // We need to re-elide
        d->updateLayout();
    } else {
        // We just need to re-layout
        d->updateSize();
    }

geomChangeDone:
    QQuickItem::geometryChange(newGeometry, oldGeometry);
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
        d->containsUnscalableGlyphs = false;
        delete oldNode;
        return nullptr;
    }

    if (d->updateType != QQuickTextPrivate::UpdatePaintNode && oldNode != nullptr) {
        // Update done in preprocess() in the nodes
        d->updateType = QQuickTextPrivate::UpdateNone;
        return oldNode;
    }

    d->updateType = QQuickTextPrivate::UpdateNone;

    const qreal dy = QQuickTextUtil::alignedY(d->layedOutTextRect.height() + d->lineHeightOffset(), d->availableHeight(), d->vAlign) + topPadding();

    QSGInternalTextNode *node = nullptr;
    if (!oldNode)
        node = d->sceneGraphContext()->createInternalTextNode(d->sceneGraphRenderContext());
    else
        node = static_cast<QSGInternalTextNode *>(oldNode);

    node->setFiltering(smooth() ? QSGTexture::Linear : QSGTexture::Nearest);

    node->setTextStyle(QSGTextNode::TextStyle(d->style));
    node->setRenderType(QSGTextNode::RenderType(d->renderType));
    node->setRenderTypeQuality(d->renderTypeQuality());
    node->clear();
    node->setMatrix(QMatrix4x4());

    node->setColor(QColor::fromRgba(d->color));
    node->setStyleColor(QColor::fromRgba(d->styleColor));
    node->setLinkColor(QColor::fromRgba(d->linkColor));

    if (d->richText) {
        node->setViewport(clipRect());
        const qreal dx = QQuickTextUtil::alignedX(d->layedOutTextRect.width(), d->availableWidth(), effectiveHAlign()) + leftPadding();
        d->ensureDoc();
        node->addTextDocument(QPointF(dx, dy), d->extra->doc);
    } else if (d->layedOutTextRect.width() > 0) {
        if (flags().testFlag(ItemObservesViewport))
            node->setViewport(clipRect());
        else
            node->setViewport(QRectF{});
        const qreal dx = QQuickTextUtil::alignedX(d->lineWidth, d->availableWidth(), effectiveHAlign()) + leftPadding();
        int unelidedLineCount = d->lineCount;
        if (d->elideLayout)
            unelidedLineCount -= 1;
        if (unelidedLineCount > 0)
            node->addTextLayout(QPointF(dx, dy), &d->layout, -1, -1,0, unelidedLineCount);

        if (d->elideLayout)
            node->addTextLayout(QPointF(dx, dy), d->elideLayout.get());

        if (d->extra.isAllocated()) {
            for (QQuickStyledTextImgTag *img : std::as_const(d->extra->visibleImgTags)) {
                if (img->pix && img->pix->isReady())
                    node->addImage(QRectF(img->pos.x() + dx, img->pos.y() + dy, img->size.width(), img->size.height()), img->pix->image());
            }
        }
    }

    d->containsUnscalableGlyphs = node->containsUnscalableGlyphs();

    // The font caches have now been initialized on the render thread, so they have to be
    // invalidated before we can use them from the main thread again.
    invalidateFontCaches();

    return node;
}

void QQuickText::updatePolish()
{
    Q_D(QQuickText);
    const bool clipNodeChanged =
            d->componentComplete && d->clipNode() && d->clipNode()->rect() != clipRect();
    if (clipNodeChanged)
        d->dirty(QQuickItemPrivate::Clip);

    // If the fonts used for rendering are different from the ones used in the GUI thread,
    // it means we will get warnings and corrupted text. If this case is detected, we need
    // to update the text layout before creating the scenegraph nodes.
    if (!d->assignedFont.isEmpty() && QFontInfo(d->font).family() != d->assignedFont)
        d->polishSize = true;

    if (d->polishSize) {
        d->updateSize();
        d->polishSize = false;
    }
    invalidateFontCaches();
}

/*!
    \qmlproperty real QtQuick::Text::contentWidth

    Returns the width of the text, including width past the width
    that is covered due to insufficient wrapping if WrapMode is set.
*/
qreal QQuickText::contentWidth() const
{
    Q_D(const QQuickText);
    return d->layedOutTextRect.width();
}

/*!
    \qmlproperty real QtQuick::Text::contentHeight

    Returns the height of the text, including height past the height
    that is covered due to there being more text than fits in the set height.
*/
qreal QQuickText::contentHeight() const
{
    Q_D(const QQuickText);
    return d->layedOutTextRect.height() + qMax(d->lineHeightOffset(), 0);
}

/*!
    \qmlproperty real QtQuick::Text::lineHeight

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

    d->extra.value().lineHeightValid = true;
    d->extra.value().lineHeight = lineHeight;
    d->implicitHeightValid = false;
    d->updateLayout();
    emit lineHeightChanged(lineHeight);
}

/*!
    \qmlproperty enumeration QtQuick::Text::lineHeightMode

    This property determines how the line height is specified.
    The possible values are:

    \value Text.ProportionalHeight  (default) sets the spacing proportional to the line
                                    (as a multiplier). For example, set to 2 for double spacing.
    \value Text.FixedHeight         sets the line height to a fixed line height (in pixels).
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

    d->implicitHeightValid = false;
    d->extra.value().lineHeightValid = true;
    d->extra.value().lineHeightMode = mode;
    d->updateLayout();

    emit lineHeightModeChanged(mode);
}

/*!
    \qmlproperty enumeration QtQuick::Text::fontSizeMode

    This property specifies how the font size of the displayed text is determined.
    The possible values are:

    \value Text.FixedSize
        (default) The size specified by \l font.pixelSize or \l font.pointSize is used.
    \value Text.HorizontalFit
        The largest size up to the size specified that fits within the width of the item
        without wrapping is used.
    \value Text.VerticalFit
        The largest size up to the size specified that fits the height of the item is used.
    \value Text.Fit
        The largest size up to the size specified that fits within the width and height
        of the item is used.

    The font size of fitted text has a minimum bound specified by the
    minimumPointSize or minimumPixelSize property and maximum bound specified
    by either the \l font.pointSize or \l font.pixelSize properties.

    \qml
    Text { text: "Hello"; fontSizeMode: Text.Fit; minimumPixelSize: 10; font.pixelSize: 72 }
    \endqml

    If the text does not fit within the item bounds with the minimum font size
    the text will be elided as per the \l elide property.

    If the \l textFormat property is set to \c Text.RichText, this will have no effect at all as the
    property will be ignored completely. If \l textFormat is set to \c Text.StyledText, then the
    property will be respected provided there is no font size tags inside the text. If there are
    font size tags, the property will still respect those. This can cause it to not fully comply with
    the fontSizeMode setting.
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

    d->polishSize = true;
    polish();

    d->extra.value().fontSizeMode = mode;
    emit fontSizeModeChanged();
}

/*!
    \qmlproperty int QtQuick::Text::minimumPixelSize

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

    if (d->fontSizeMode() != FixedSize && (widthValid() || heightValid())) {
        d->polishSize = true;
        polish();
    }
    d->extra.value().minimumPixelSize = size;
    emit minimumPixelSizeChanged();
}

/*!
    \qmlproperty int QtQuick::Text::minimumPointSize

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

    if (d->fontSizeMode() != FixedSize && (widthValid() || heightValid())) {
        d->polishSize = true;
        polish();
    }
    d->extra.value().minimumPointSize = size;
    emit minimumPointSizeChanged();
}

/*!
    Returns the number of resources (images) that are being loaded asynchronously.
*/
int QQuickText::resourcesLoading() const
{
    Q_D(const QQuickText);
    if (d->richText && d->extra.isAllocated())
        return d->extra->pixmapsInProgress.size();
    return 0;
}

/*! \internal */
void QQuickText::componentComplete()
{
    Q_D(QQuickText);
    if (d->updateOnComponentComplete) {
        if (d->richText) {
            d->updateDocumentText();
        } else {
            d->rightToLeftText = d->text.isRightToLeft();
        }
        d->determineHorizontalAlignment();
    }
    QQuickItem::componentComplete();
    if (d->updateOnComponentComplete)
        d->updateLayout();
}

QString QQuickTextPrivate::anchorAt(const QTextLayout *layout, const QPointF &mousePos)
{
    for (int i = 0; i < layout->lineCount(); ++i) {
        QTextLine line = layout->lineAt(i);
        if (line.naturalTextRect().contains(mousePos)) {
            int charPos = line.xToCursor(mousePos.x(), QTextLine::CursorOnCharacter);
            const auto formats = layout->formats();
            for (const QTextLayout::FormatRange &formatRange : formats) {
                if (formatRange.format.isAnchor()
                        && charPos >= formatRange.start
                        && charPos < formatRange.start + formatRange.length) {
                    return formatRange.format.anchorHref();
                }
            }
            break;
        }
    }
    return QString();
}

QString QQuickTextPrivate::anchorAt(const QPointF &mousePos) const
{
    Q_Q(const QQuickText);
    QPointF translatedMousePos = mousePos;
    translatedMousePos.rx() -= q->leftPadding();
    translatedMousePos.ry() -= q->topPadding() + QQuickTextUtil::alignedY(layedOutTextRect.height() + lineHeightOffset(), availableHeight(), vAlign);
    if (styledText) {
        QString link = anchorAt(&layout, translatedMousePos);
        if (link.isEmpty() && elideLayout)
            link = anchorAt(elideLayout.get(), translatedMousePos);
        return link;
    } else if (richText && extra.isAllocated() && extra->doc) {
        translatedMousePos.rx() -= QQuickTextUtil::alignedX(layedOutTextRect.width(), availableWidth(), q->effectiveHAlign());
        return extra->doc->documentLayout()->anchorAt(translatedMousePos);
    }
    return QString();
}

bool QQuickTextPrivate::isLinkActivatedConnected()
{
    Q_Q(QQuickText);
    IS_SIGNAL_CONNECTED(q, QQuickText, linkActivated, (const QString &));
}

/*!  \internal */
void QQuickText::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickText);

    QString link;
    if (d->isLinkActivatedConnected())
        link = d->anchorAt(event->position());

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
    if (d->isLinkActivatedConnected())
        link = d->anchorAt(event->position());

    if (!link.isEmpty() && d->extra.isAllocated() && d->extra->activeLink == link)
        emit linkActivated(d->extra->activeLink);
    else
        event->setAccepted(false);

    if (!event->isAccepted())
        QQuickItem::mouseReleaseEvent(event);
}

bool QQuickTextPrivate::isLinkHoveredConnected()
{
    Q_Q(QQuickText);
    IS_SIGNAL_CONNECTED(q, QQuickText, linkHovered, (const QString &));
}

static void getLinks_helper(const QTextLayout *layout, QVector<QQuickTextPrivate::LinkDesc> *links)
{
    for (const QTextLayout::FormatRange &formatRange : layout->formats()) {
        if (formatRange.format.isAnchor()) {
            const int start = formatRange.start;
            const int len = formatRange.length;
            QTextLine line = layout->lineForTextPosition(start);
            QRectF r;
            r.setTop(line.y());
            r.setLeft(line.cursorToX(start, QTextLine::Leading));
            r.setHeight(line.height());
            r.setRight(line.cursorToX(start + len, QTextLine::Trailing));
            // ### anchorNames() is empty?! Not sure why this doesn't work
            // QString anchorName = formatRange.format.anchorNames().value(0); //### pick the first?
            // Therefore, we resort to QString::mid()
            QString anchorName = layout->text().mid(start, len);
            const QString anchorHref = formatRange.format.anchorHref();
            if (anchorName.isEmpty())
                anchorName = anchorHref;
            links->append( { anchorName, anchorHref, start, start + len, r.toRect()} );
        }
    }
}

QVector<QQuickTextPrivate::LinkDesc> QQuickTextPrivate::getLinks() const
{
    QVector<QQuickTextPrivate::LinkDesc> links;
    getLinks_helper(&layout, &links);
    return links;
}


/*!
    \qmlsignal QtQuick::Text::linkHovered(string link)
    \since 5.2

    This signal is emitted when the user hovers a link embedded in the
    text. The link must be in rich text or HTML format and the \a link
    string provides access to the particular link.

    \sa hoveredLink, linkAt()
*/

/*!
    \qmlproperty string QtQuick::Text::hoveredLink
    \since 5.2

    This property contains the link string when the user hovers a link
    embedded in the text. The link must be in rich text or HTML format
    and the \a hoveredLink string provides access to the particular link.

    \sa linkHovered, linkAt()
*/

QString QQuickText::hoveredLink() const
{
    Q_D(const QQuickText);
    if (const_cast<QQuickTextPrivate *>(d)->isLinkHoveredConnected()) {
        if (d->extra.isAllocated())
            return d->extra->hoveredLink;
    } else {
#if QT_CONFIG(cursor)
        if (QQuickWindow *wnd = window()) {
            QPointF pos = QCursor::pos(wnd->screen()) - wnd->position() - mapToScene(QPointF(0, 0));
            return d->anchorAt(pos);
        }
#endif // cursor
    }
    return QString();
}

void QQuickTextPrivate::processHoverEvent(QHoverEvent *event)
{
    Q_Q(QQuickText);
    qCDebug(lcHoverTrace) << q;
    QString link;
    if (isLinkHoveredConnected()) {
        if (event->type() != QEvent::HoverLeave)
            link = anchorAt(event->position());

        if ((!extra.isAllocated() && !link.isEmpty()) || (extra.isAllocated() && extra->hoveredLink != link)) {
            extra.value().hoveredLink = link;
            emit q->linkHovered(extra->hoveredLink);
        }
    }
    event->ignore();
}

void QQuickText::hoverEnterEvent(QHoverEvent *event)
{
    Q_D(QQuickText);
    d->processHoverEvent(event);
}

void QQuickText::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(QQuickText);
    d->processHoverEvent(event);
}

void QQuickText::hoverLeaveEvent(QHoverEvent *event)
{
    Q_D(QQuickText);
    d->processHoverEvent(event);
}

void QQuickText::invalidate()
{
    Q_D(QQuickText);
    d->textHasChanged = true;
    QMetaObject::invokeMethod(this,[&]{q_updateLayout();});
}

bool QQuickTextPrivate::transformChanged(QQuickItem *transformedItem)
{
    // If there's a lot of text, we may need QQuickText::updatePaintNode() to call
    // QSGInternalTextNode::addTextLayout() again to populate a different range of lines
    if (flags & QQuickItem::ItemObservesViewport) {
        updateType = UpdatePaintNode;
        dirty(QQuickItemPrivate::Content);
    }
    return QQuickImplicitSizeItemPrivate::transformChanged(transformedItem);
}

/*!
    \qmlproperty int QtQuick::Text::renderTypeQuality
    \since 6.0

    Override the default rendering type quality for this component. This is a low-level
    customization which can be ignored in most cases. It currently only has an effect
    when \l renderType is \c Text.QtRendering.

    The rasterization algorithm used by Text.QtRendering may give artifacts at
    large text sizes, such as sharp corners looking rounder than they should. If
    this is an issue for specific text items, increase the \c renderTypeQuality to
    improve rendering quality, at the expense of memory consumption.

    The \c renderTypeQuality may be any integer over 0, or one of the following
    predefined values

    \value Text.DefaultRenderTypeQuality    -1 (default)
    \value Text.LowRenderTypeQuality        26
    \value Text.NormalRenderTypeQuality     52
    \value Text.HighRenderTypeQuality       104
    \value Text.VeryHighRenderTypeQuality   208
*/
int QQuickText::renderTypeQuality() const
{
    Q_D(const QQuickText);
    return d->renderTypeQuality();
}

void QQuickText::setRenderTypeQuality(int renderTypeQuality)
{
    Q_D(QQuickText);
    if (renderTypeQuality == d->renderTypeQuality())
        return;
    d->extra.value().renderTypeQuality = renderTypeQuality;

    if (isComponentComplete()) {
        d->updateType = QQuickTextPrivate::UpdatePaintNode;
        update();
    }

    emit renderTypeQualityChanged();
}

/*!
    \qmlproperty enumeration QtQuick::Text::renderType

    Override the default rendering type for this component.

    Supported render types are:

    \value Text.QtRendering     Text is rendered using a scalable distance field for each glyph.
    \value Text.NativeRendering Text is rendered using a platform-specific technique.
    \value Text.CurveRendering  Text is rendered using a curve rasterizer running directly on the
                                graphics hardware. (Introduced in Qt 6.7.0.)

    Select \c Text.NativeRendering if you prefer text to look native on the target platform and do
    not require advanced features such as transformation of the text. Using such features in
    combination with the NativeRendering render type will lend poor and sometimes pixelated
    results.

    Both \c Text.QtRendering and \c Text.CurveRendering are hardware-accelerated techniques.
    \c QtRendering is the faster of the two, but uses more memory and will exhibit rendering
    artifacts at large sizes. \c CurveRendering should be considered as an alternative in cases
    where \c QtRendering does not give good visual results or where reducing graphics memory
    consumption is a priority.

    The default rendering type is determined by \l QQuickWindow::textRenderType().
*/
QQuickText::RenderType QQuickText::renderType() const
{
    Q_D(const QQuickText);
    return d->renderType;
}

void QQuickText::setRenderType(QQuickText::RenderType renderType)
{
    Q_D(QQuickText);
    if (d->renderType == renderType)
        return;

    d->renderType = renderType;
    emit renderTypeChanged();

    if (isComponentComplete())
        d->updateLayout();
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#if QT_DEPRECATED_SINCE(5, 15)
/*!
    \qmlmethod QtQuick::Text::doLayout()
    \deprecated

    Use \l forceLayout() instead.
*/
void QQuickText::doLayout()
{
    forceLayout();
}

#endif
#endif
/*!
    \qmlmethod QtQuick::Text::forceLayout()
    \since 5.9

    Triggers a re-layout of the displayed text.
*/
void QQuickText::forceLayout()
{
    Q_D(QQuickText);
    d->updateSize();
}

/*!
    \qmlmethod QtQuick::Text::linkAt(real x, real y)
    \since 5.3

    Returns the link string at point \a x, \a y in content coordinates,
    or an empty string if no link exists at that point.

    \sa hoveredLink
*/
QString QQuickText::linkAt(qreal x, qreal y) const
{
    Q_D(const QQuickText);
    return d->anchorAt(QPointF(x, y));
}

/*!
 * \internal
 *
 * Invalidates font caches owned by the text objects owned by the element
 * to work around the fact that text objects cannot be used from multiple threads.
 */
void QQuickText::invalidateFontCaches()
{
    Q_D(QQuickText);

    if (d->richText && d->extra.isAllocated() && d->extra->doc != nullptr) {
        QTextBlock block;
        for (block = d->extra->doc->firstBlock(); block.isValid(); block = block.next()) {
            if (block.layout() != nullptr && block.layout()->engine() != nullptr)
                block.layout()->engine()->resetFontEngineCache();
        }
    } else {
        if (d->layout.engine() != nullptr)
            d->layout.engine()->resetFontEngineCache();
    }
}

/*!
    \since 5.6
    \qmlproperty real QtQuick::Text::padding
    \qmlproperty real QtQuick::Text::topPadding
    \qmlproperty real QtQuick::Text::leftPadding
    \qmlproperty real QtQuick::Text::bottomPadding
    \qmlproperty real QtQuick::Text::rightPadding

    These properties hold the padding around the content. This space is reserved
    in addition to the contentWidth and contentHeight.
*/
qreal QQuickText::padding() const
{
    Q_D(const QQuickText);
    return d->padding();
}

void QQuickText::setPadding(qreal padding)
{
    Q_D(QQuickText);
    if (qFuzzyCompare(d->padding(), padding))
        return;

    d->extra.value().padding = padding;
    d->updateSize();
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

void QQuickText::resetPadding()
{
    setPadding(0);
}

qreal QQuickText::topPadding() const
{
    Q_D(const QQuickText);
    if (d->extra.isAllocated() && d->extra->explicitTopPadding)
        return d->extra->topPadding;
    return d->padding();
}

void QQuickText::setTopPadding(qreal padding)
{
    Q_D(QQuickText);
    d->setTopPadding(padding);
}

void QQuickText::resetTopPadding()
{
    Q_D(QQuickText);
    d->setTopPadding(0, true);
}

qreal QQuickText::leftPadding() const
{
    Q_D(const QQuickText);
    if (d->extra.isAllocated() && d->extra->explicitLeftPadding)
        return d->extra->leftPadding;
    return d->padding();
}

void QQuickText::setLeftPadding(qreal padding)
{
    Q_D(QQuickText);
    d->setLeftPadding(padding);
}

void QQuickText::resetLeftPadding()
{
    Q_D(QQuickText);
    d->setLeftPadding(0, true);
}

qreal QQuickText::rightPadding() const
{
    Q_D(const QQuickText);
    if (d->extra.isAllocated() && d->extra->explicitRightPadding)
        return d->extra->rightPadding;
    return d->padding();
}

void QQuickText::setRightPadding(qreal padding)
{
    Q_D(QQuickText);
    d->setRightPadding(padding);
}

void QQuickText::resetRightPadding()
{
    Q_D(QQuickText);
    d->setRightPadding(0, true);
}

qreal QQuickText::bottomPadding() const
{
    Q_D(const QQuickText);
    if (d->extra.isAllocated() && d->extra->explicitBottomPadding)
        return d->extra->bottomPadding;
    return d->padding();
}

void QQuickText::setBottomPadding(qreal padding)
{
    Q_D(QQuickText);
    d->setBottomPadding(padding);
}

void QQuickText::resetBottomPadding()
{
    Q_D(QQuickText);
    d->setBottomPadding(0, true);
}

/*!
    \qmlproperty string QtQuick::Text::fontInfo.family
    \since 5.9

    The family name of the font that has been resolved for the current font
    and fontSizeMode.
*/

/*!
    \qmlproperty string QtQuick::Text::fontInfo.styleName
    \since 5.9

    The style name of the font info that has been resolved for the current font
    and fontSizeMode.
*/

/*!
    \qmlproperty bool QtQuick::Text::fontInfo.bold
    \since 5.9

    The bold state of the font info that has been resolved for the current font
    and fontSizeMode. This is true if the weight of the resolved font is bold or higher.
*/

/*!
    \qmlproperty int QtQuick::Text::fontInfo.weight
    \since 5.9

    The weight of the font info that has been resolved for the current font
    and fontSizeMode.
*/

/*!
    \qmlproperty bool QtQuick::Text::fontInfo.italic
    \since 5.9

    The italic state of the font info that has been resolved for the current font
    and fontSizeMode.
*/

/*!
    \qmlproperty real QtQuick::Text::fontInfo.pointSize
    \since 5.9

    The pointSize of the font info that has been resolved for the current font
    and fontSizeMode.
*/

/*!
    \qmlproperty int QtQuick::Text::fontInfo.pixelSize
    \since 5.9

    The pixel size of the font info that has been resolved for the current font
    and fontSizeMode.
*/
QJSValue QQuickText::fontInfo() const
{
    Q_D(const QQuickText);

    QJSEngine *engine = qjsEngine(this);
    if (!engine) {
        qmlWarning(this) << "fontInfo: item has no JS engine";
        return QJSValue();
    }

    QJSValue value = engine->newObject();
    value.setProperty(QStringLiteral("family"), d->fontInfo.family());
    value.setProperty(QStringLiteral("styleName"), d->fontInfo.styleName());
    value.setProperty(QStringLiteral("bold"), d->fontInfo.bold());
    value.setProperty(QStringLiteral("weight"), d->fontInfo.weight());
    value.setProperty(QStringLiteral("italic"), d->fontInfo.italic());
    value.setProperty(QStringLiteral("pointSize"), d->fontInfo.pointSizeF());
    value.setProperty(QStringLiteral("pixelSize"), d->fontInfo.pixelSize());
    return value;
}

/*!
    \qmlproperty size QtQuick::Text::advance
    \since 5.10

    The distance, in pixels, from the baseline origin of the first
    character of the text item, to the baseline origin of the first
    character in a text item occurring directly after this one
    in a text flow.

    Note that the advance can be negative if the text flows from
    right to left.
*/
QSizeF QQuickText::advance() const
{
    Q_D(const QQuickText);
    return d->advance;
}

QT_END_NAMESPACE

#include "moc_qquicktext_p.cpp"
