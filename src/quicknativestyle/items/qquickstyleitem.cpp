// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitem.h"

#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/qdir.h>

#include <QtQuick/qsgninepatchnode.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickrendercontrol.h>

#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>

#include <QtQml/qqml.h>

#include "qquickstyleitembutton.h"
#include "qquickstylehelper_p.h"

QT_BEGIN_NAMESPACE

QDebug operator<<(QDebug debug, const QQuickStyleMargins &padding)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "StyleMargins(";
    debug << padding.left() << ", ";
    debug << padding.top() << ", ";
    debug << padding.right() << ", ";
    debug << padding.bottom();
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const StyleItemGeometry &cg)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "StyleItemGeometry(";
    debug << "implicitSize:" << cg.implicitSize << ", ";
    debug << "contentRect:" << cg.contentRect << ", ";
    debug << "layoutRect:" << cg.layoutRect << ", ";
    debug << "minimumSize:" << cg.minimumSize << ", ";
    debug << "9patchMargins:" << cg.ninePatchMargins;
    debug << ')';
    return debug;
}

int QQuickStyleItem::dprAlignedSize(const int size) const
{
    // Return the first value equal to or bigger than size
    // that is a whole number when multiplied with the dpr.
    static int multiplier = [&]() {
        const qreal dpr = window()->effectiveDevicePixelRatio();
        for (int m = 1; m <= 10; ++m) {
            const qreal v = m * dpr;
            if (v == int(v))
                return m;
        }

        qWarning() << "The current dpr (" << dpr << ") is not supported"
                   << "by the style and might result in drawing artifacts";
        return 1;
    }();

    return int(qCeil(qreal(size) / qreal(multiplier)) * multiplier);
}

QQuickStyleItem::QQuickStyleItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents);
}

QQuickStyleItem::~QQuickStyleItem()
{
}

void QQuickStyleItem::connectToControl() const
{
    connect(m_control, &QQuickStyleItem::enabledChanged, this, &QQuickStyleItem::markImageDirty);
    connect(m_control, &QQuickItem::activeFocusChanged, this, &QQuickStyleItem::markImageDirty);

    if (QQuickWindow *win = window()) {
        connect(win, &QQuickWindow::activeChanged, this, &QQuickStyleItem::markImageDirty);
        m_connectedWindow = win;
    }
}

void QQuickStyleItem::markImageDirty()
{
    m_dirty.setFlag(DirtyFlag::Image);
    if (isComponentComplete())
        polish();
}

void QQuickStyleItem::markGeometryDirty()
{
    m_dirty.setFlag(DirtyFlag::Geometry);
    if (isComponentComplete())
        polish();
}

QSGNode *QQuickStyleItem::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    QSGNinePatchNode *node = static_cast<QSGNinePatchNode *>(oldNode);
    if (!node)
        node = window()->createNinePatchNode();

    if (m_paintedImage.isNull()) {
        // If we cannot create a texture, the node should not exist either
        // because its material requires a texture.
        delete node;
        return nullptr;
    }

    const auto texture = window()->createTextureFromImage(m_paintedImage, QQuickWindow::TextureCanUseAtlas);

    QRectF bounds = boundingRect();
    const qreal dpr = window()->effectiveDevicePixelRatio();
    const QSizeF unscaledImageSize = QSizeF(m_paintedImage.size()) / dpr;

    // We can scale the image up with a nine patch node, but should
    // avoid to scale it down. Otherwise the nine patch image will look
    // wrapped (or look truncated, in case of no padding). So if the
    // item is smaller that the image, don't scale.
    if (bounds.width() < unscaledImageSize.width())
        bounds.setWidth(unscaledImageSize.width());
    if (bounds.height() < unscaledImageSize.height())
        bounds.setHeight(unscaledImageSize.height());

#ifdef QT_DEBUG
    if (m_debugFlags.testFlag(Unscaled)) {
        bounds.setSize(unscaledImageSize);
        qqc2Info() << "Setting qsg node size to the unscaled size of m_paintedImage:" << bounds;
    }
#endif

    if (m_useNinePatchImage) {
        QMargins padding = m_styleItemGeometry.ninePatchMargins;
        if (padding.right() == -1) {
            // Special case: a padding of -1 means that
            // the image shouldn't scale in the given direction.
            padding.setLeft(0);
            padding.setRight(0);
        }
        if (padding.bottom() == -1) {
            padding.setTop(0);
            padding.setBottom(0);
        }
        node->setPadding(padding.left(), padding.top(), padding.right(), padding.bottom());
    }

    node->setBounds(bounds);
    node->setTexture(texture);
    node->setDevicePixelRatio(dpr);
    node->update();

    return node;
}

QStyle::State QQuickStyleItem::controlSize(QQuickItem *item)
{
    // TODO: add proper API for small and mini
    if (item->metaObject()->indexOfProperty("qqc2_style_small") != -1)
        return QStyle::State_Small;
    if (item->metaObject()->indexOfProperty("qqc2_style_mini") != -1)
        return QStyle::State_Mini;
    return QStyle::State_None;
}

static QWindow *effectiveWindow(QQuickWindow *window)
{
    QWindow *renderWindow = QQuickRenderControl::renderWindowFor(window);
    return renderWindow ? renderWindow : window;
}

void QQuickStyleItem::initStyleOptionBase(QStyleOption &styleOption) const
{
    Q_ASSERT(m_control);

    styleOption.control = const_cast<QQuickItem *>(control<QQuickItem>());
    styleOption.window = effectiveWindow(window());
    styleOption.palette = QQuickItemPrivate::get(m_control)->palette()->toQPalette();
    styleOption.rect = QRect(QPoint(0, 0), imageSize());

    styleOption.state = QStyle::State_None;
    styleOption.state |= controlSize(styleOption.control);

    // Note: not all controls inherit from QQuickControl (e.g QQuickTextField)
    if (const auto quickControl = dynamic_cast<QQuickControl *>(m_control.data()))
        styleOption.direction = quickControl->isMirrored() ? Qt::RightToLeft : Qt::LeftToRight;

    if (styleOption.window) {
        if (styleOption.window->isActive())
            styleOption.state |= QStyle::State_Active;
        if (m_control->isEnabled())
            styleOption.state |= QStyle::State_Enabled;
        if (m_control->hasActiveFocus())
            styleOption.state |= QStyle::State_HasFocus;
        if (m_control->isUnderMouse())
            styleOption.state |= QStyle::State_MouseOver;
        // Should this depend on the focusReason (e.g. only TabFocus) ?
        styleOption.state |= QStyle::State_KeyboardFocusChange;
    }

    if (m_overrideState != None) {
        // In Button.qml we fade between two versions of
        // the handle, depending on if it's hovered or not
        if (m_overrideState & AlwaysHovered)
            styleOption.state |= QStyle::State_MouseOver;
        else if (m_overrideState & NeverHovered)
            styleOption.state &= ~QStyle::State_MouseOver;
    }

    qqc2Info() << styleOption;
}

void QQuickStyleItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    // Ensure that we only schedule a new geometry update
    // and polish if this geometry change was caused by
    // something else than us already updating geometry.
    if (!m_polishing)
        markGeometryDirty();
}

void QQuickStyleItem::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    QQuickItem::itemChange(change, data);

    switch (change) {
    case QQuickItem::ItemVisibleHasChanged:
        if (data.boolValue)
            markImageDirty();
        break;
    case QQuickItem::ItemSceneChange: {
        markImageDirty();
        QQuickWindow *win = data.window;
        if (m_connectedWindow)
            disconnect(m_connectedWindow, &QQuickWindow::activeChanged, this, &QQuickStyleItem::markImageDirty);
        if (win)
            connect(win, &QQuickWindow::activeChanged, this, &QQuickStyleItem::markImageDirty);
        m_connectedWindow = win;
        break;}
    default:
        break;
    }
}

bool QQuickStyleItem::event(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange) {
        markImageDirty();
        if (auto *style = QQuickStyleItem::style())
            style->polish();
    }

    return QQuickItem::event(event);
}

void QQuickStyleItem::updateGeometry()
{
    qqc2InfoHeading("GEOMETRY");
    m_dirty.setFlag(DirtyFlag::Geometry, false);

    const QQuickStyleMargins oldContentPadding = contentPadding();
    const QQuickStyleMargins oldLayoutMargins = layoutMargins();
    const QSize oldMinimumSize = minimumSize();

    m_styleItemGeometry = calculateGeometry();

#ifdef QT_DEBUG
    if (m_styleItemGeometry.minimumSize.isEmpty())
        qmlWarning(this) << "(StyleItem) minimumSize is empty!";
#endif

    if (m_styleItemGeometry.implicitSize.isEmpty()) {
        // If the item has no contents (or its size is
        // empty), we just use the minimum size as implicit size.
        m_styleItemGeometry.implicitSize = m_styleItemGeometry.minimumSize;
        qqc2Info() << "implicitSize is empty, using minimumSize instead";
    }

#ifdef QT_DEBUG
    if (m_styleItemGeometry.implicitSize.width() < m_styleItemGeometry.minimumSize.width())
        qmlWarning(this) << "(StyleItem) implicit width is smaller than minimum width!";
    if (m_styleItemGeometry.implicitSize.height() < m_styleItemGeometry.minimumSize.height())
        qmlWarning(this) << "(StyleItem) implicit height is smaller than minimum height!";
#endif

    if (contentPadding() != oldContentPadding)
        emit contentPaddingChanged();
    if (layoutMargins() != oldLayoutMargins)
        emit layoutMarginsChanged();
    if (minimumSize() != oldMinimumSize)
        emit minimumSizeChanged();

    setImplicitSize(m_styleItemGeometry.implicitSize.width(), m_styleItemGeometry.implicitSize.height());

    qqc2Info() << m_styleItemGeometry
                << "bounding rect:" << boundingRect()
                << "layout margins:" << layoutMargins()
                << "content padding:" << contentPadding()
                << "input content size:" << m_contentSize;
}

void QQuickStyleItem::paintControlToImage()
{
    qqc2InfoHeading("PAINT");
    const QSize imgSize = imageSize();
    if (imgSize.isEmpty())
        return;

    m_dirty.setFlag(DirtyFlag::Image, false);

    // The size of m_paintedImage should normally be imgSize * dpr. The problem is
    // that the dpr can be e.g 1.25, which means that the size can end up having a
    // fraction. But an image cannot have a size with a fraction, so it would need
    // to be rounded. But on the flip side, rounding the size means that the size
    // of the scene graph node (which is, when the texture is not scaled,
    // m_paintedImage.size() / dpr), will end up with a fraction instead. And this
    // causes rendering artifacts in the scene graph when the texture is mapped
    // to physical screen coordinates. So for that reason we calculate an image size
    // that might be slightly larger than imgSize, so that imgSize * dpr lands on a
    // whole number. The result is that neither the image size, nor the scene graph
    // node, ends up with a size that has a fraction.
    const qreal dpr = window()->effectiveDevicePixelRatio();
    const int alignedW = int(dprAlignedSize(imgSize.width()) * dpr);
    const int alignedH = int(dprAlignedSize(imgSize.height()) * dpr);
    const QSize alignedSize = QSize(alignedW, alignedH);

    if (m_paintedImage.size() != alignedSize) {
        m_paintedImage = QImage(alignedSize, QImage::Format_ARGB32_Premultiplied);
        m_paintedImage.setDevicePixelRatio(dpr);
        qqc2Info() << "created image with dpr aligned size:" << alignedSize;
    }

    m_paintedImage.fill(Qt::transparent);

    QPainter painter(&m_paintedImage);
    paintEvent(&painter);

#ifdef QT_DEBUG
    if (m_debugFlags != NoDebug) {
        painter.setPen(QColor(255, 0, 0, 255));
        if (m_debugFlags.testFlag(ImageRect))
            painter.drawRect(QRect(QPoint(0, 0), alignedSize / dpr));
        if (m_debugFlags.testFlag(LayoutRect)) {
            const auto m = layoutMargins();
            QRect rect = QRect(QPoint(0, 0), imgSize);
            rect.adjust(m.left(), m.top(), -m.right(), -m.bottom());
            painter.drawRect(rect);
        }
        if (m_debugFlags.testFlag(ContentRect)) {
            const auto p = contentPadding();
            QRect rect = QRect(QPoint(0, 0), imgSize);
            rect.adjust(p.left(), p.top(), -p.right(), -p.bottom());
            painter.drawRect(rect);
        }
        if (m_debugFlags.testFlag(InputContentSize)) {
            const int offset = 2;
            const QPoint p = m_styleItemGeometry.contentRect.topLeft();
            painter.drawLine(p.x() - offset, p.y() - offset, p.x() + m_contentSize.width(), p.y() - offset);
            painter.drawLine(p.x() - offset, p.y() - offset, p.x() - offset, p.y() + m_contentSize.height());
        }
        if (m_debugFlags.testFlag(NinePatchMargins)) {
            const QMargins m = m_styleItemGeometry.ninePatchMargins;
            if (m.right() != -1) {
                painter.drawLine(m.left(), 0, m.left(), imgSize.height());
                painter.drawLine(imgSize.width() - m.right(), 0, imgSize.width() - m.right(), imgSize.height());
            }
            if (m.bottom() != -1) {
                painter.drawLine(0, m.top(), imgSize.width(), m.top());
                painter.drawLine(0, imgSize.height() - m.bottom(), imgSize.width(), imgSize.height() - m.bottom());
            }
        }
        if (m_debugFlags.testFlag(SaveImage)) {
            static int nr = -1;
            ++nr;
            static QString filename = QStringLiteral("styleitem_saveimage_");
            const QString path = QDir::current().absoluteFilePath(filename);
            const QString name = path + QString::number(nr) + QStringLiteral(".png");
            m_paintedImage.save(name);
            qDebug() << "image saved to:" << name;
        }
    }
#endif

    update();
}

void QQuickStyleItem::updatePolish()
{
    QScopedValueRollback<bool> guard(m_polishing, true);

    const bool dirtyGeometry = m_dirty & DirtyFlag::Geometry;
    const bool dirtyImage = isVisible() && ((m_dirty & DirtyFlag::Image) || (!m_useNinePatchImage && dirtyGeometry));

    if (dirtyGeometry)
        updateGeometry();
    if (dirtyImage)
        paintControlToImage();
}

#ifdef QT_DEBUG
void QQuickStyleItem::addDebugInfo()
{
    // Example debug strings:
    // "QQC2_NATIVESTYLE_DEBUG="myButton info contentRect"
    // "QQC2_NATIVESTYLE_DEBUG="ComboBox ninepatchmargins"
    // "QQC2_NATIVESTYLE_DEBUG="All layoutrect"

    static const auto debugString = qEnvironmentVariable("QQC2_NATIVESTYLE_DEBUG");
    static const auto matchAll = debugString.startsWith(QLatin1String("All "));
    static const auto prefix = QStringLiteral("QQuickStyleItem");
    if (debugString.isEmpty())
        return;

    const auto objectName = m_control->objectName();
    const auto typeName = QString::fromUtf8(metaObject()->className()).remove(prefix);
    const bool matchName = !objectName.isEmpty() && debugString.startsWith(objectName);
    const bool matchType = debugString.startsWith(typeName);

    if (!(matchAll || matchName || matchType))
        return;

#define QQC2_DEBUG_FLAG(FLAG) \
    if (debugString.contains(QLatin1String(#FLAG), Qt::CaseInsensitive)) m_debugFlags |= FLAG

    QQC2_DEBUG_FLAG(Info);
    QQC2_DEBUG_FLAG(ImageRect);
    QQC2_DEBUG_FLAG(ContentRect);
    QQC2_DEBUG_FLAG(LayoutRect);
    QQC2_DEBUG_FLAG(InputContentSize);
    QQC2_DEBUG_FLAG(DontUseNinePatchImage);
    QQC2_DEBUG_FLAG(NinePatchMargins);
    QQC2_DEBUG_FLAG(Unscaled);
    QQC2_DEBUG_FLAG(Debug);
    QQC2_DEBUG_FLAG(SaveImage);

    if (m_debugFlags & (DontUseNinePatchImage
                        | InputContentSize
                        | ContentRect
                        | LayoutRect
                        | NinePatchMargins)) {
        // Some rects will not fit inside the drawn image unless
        // we switch off (nine patch) image scaling.
        m_debugFlags |= DontUseNinePatchImage;
        m_useNinePatchImage = false;
    }

    if (m_debugFlags != NoDebug)
        qDebug() << "debug options set for" << typeName << "(" << objectName << "):" << m_debugFlags;
    else
        qDebug() << "available debug options:" << DebugFlags(0xFFFF);
}
#endif

void QQuickStyleItem::componentComplete()
{
    Q_ASSERT_X(m_control, Q_FUNC_INFO, "You need to assign a value to property 'control'");
#ifdef QT_DEBUG
    addDebugInfo();
#endif
    QQuickItem::componentComplete();
    updateGeometry();
    connectToControl();
    polish();
}

qreal QQuickStyleItem::contentWidth()
{
    return m_contentSize.width();
}

void QQuickStyleItem::setContentWidth(qreal contentWidth)
{
    if (qFuzzyCompare(m_contentSize.width(), contentWidth))
        return;

    m_contentSize.setWidth(contentWidth);
    markGeometryDirty();
}

qreal QQuickStyleItem::contentHeight()
{
    return m_contentSize.height();
}

void QQuickStyleItem::setContentHeight(qreal contentHeight)
{
    if (qFuzzyCompare(m_contentSize.height(), contentHeight))
        return;

    m_contentSize.setHeight(contentHeight);
    markGeometryDirty();
}

QQuickStyleMargins QQuickStyleItem::contentPadding() const
{
    const QRect outerRect(QPoint(0, 0), m_styleItemGeometry.implicitSize);
    return QQuickStyleMargins(outerRect, m_styleItemGeometry.contentRect);
}

QQuickStyleMargins QQuickStyleItem::layoutMargins() const
{
    // ### TODO: layoutRect is currently not being used for anything. But
    // eventually this information will be needed by layouts to align the controls
    // correctly. This because the images drawn by QStyle are usually a bit bigger
    // than the control(frame) itself, to e.g make room for shadow effects
    // or focus rects/glow. And this will differ from control to control. The
    // layoutRect will then inform where the frame of the control is.
    QQuickStyleMargins margins;
    if (m_styleItemGeometry.layoutRect.isValid()) {
        const QRect outerRect(QPoint(0, 0), m_styleItemGeometry.implicitSize);
        margins = QQuickStyleMargins(outerRect, m_styleItemGeometry.layoutRect);
    }
    return margins;
}

QSize QQuickStyleItem::minimumSize() const
{
    // The style item should not be scaled below this size.
    // Otherwise the image will be truncated.
    return m_styleItemGeometry.minimumSize;
}

QSize QQuickStyleItem::imageSize() const
{
    // Returns the size of the QImage (unscaled) that
    // is used to draw the control from QStyle.
    return m_useNinePatchImage ? m_styleItemGeometry.minimumSize : size().toSize();
}

qreal QQuickStyleItem::focusFrameRadius() const
{
    return m_styleItemGeometry.focusFrameRadius;
}

QFont QQuickStyleItem::styleFont(QQuickItem *control) const
{
    Q_ASSERT(control);
    // Note: This function should be treated as if it was static
    // (meaning, don't assume anything in this object to be initialized).
    // Resolving the font/font size should be done early on from QML, before we get
    // around to calculate geometry and paint. Otherwise we typically need to do it
    // all over again when/if the font changes. In practice this means that other
    // items in QML that uses a style font, and at the same time, affects our input
    // contentSize, cannot wait for this item to be fully constructed before it
    // gets the font. So we need to resolve it here and now, even if this
    // object might be in a half initialized state (hence also the control
    // argument, instead of relying on m_control to be set).
    return QGuiApplication::font();
}

QT_END_NAMESPACE

#include "moc_qquickstyleitem.cpp"
