// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitlayout_p.h"

QT_BEGIN_NAMESPACE

static qreal width(QQStyleKitLayoutItem *li, qreal availableWidth = .0)
{
    Q_ASSERT(li);
    Q_ASSERT(li->item());
    QQuickItem *item = li->item();
    qreal w = item->implicitWidth();
    if (li->fillWidth())
        w = qMax(.0, availableWidth - li->margins().left() - li->margins().right());
    return qMax(.0, w);
}

static qreal height(QQStyleKitLayoutItem *li, qreal availableHeight = .0)
{
    Q_ASSERT(li);
    Q_ASSERT(li->item());
    QQuickItem *item = li->item();
    qreal h = item->implicitHeight();
    if (li->fillHeight())
        h = qMax(.0, availableHeight - li->margins().top() - li->margins().bottom());
    return qMax(.0, h);
}

static qreal totalWidth(const QList<QQStyleKitLayoutItem *> &items, qreal spacing)
{
    qreal total = .0;
    for (QQStyleKitLayoutItem *li : items) {
        if (li->item() && li->item()->isVisible())
            total += width(li) + li->margins().left() + li->margins().right() + spacing;
    }
    return total;
}

static qreal totalHeight(const QList<QQStyleKitLayoutItem *> &items)
{
    qreal maxHeight = .0;
    for (QQStyleKitLayoutItem *li : items) {
        if (li->item() && li->item()->isVisible()) {
            const qreal h = height(li) + li->margins().top() + li->margins().bottom();
            if (h > maxHeight)
                maxHeight = h;
        }
    }
    return maxHeight;
}

static qreal vAlignY(QQStyleKitLayoutItem *li, qreal containerY, qreal containerHeight)
{
    Q_ASSERT(li);
    Q_ASSERT(li->item());

    const auto itemHeight = height(li, containerHeight);
    const auto vAlign = li->alignment() & Qt::AlignVertical_Mask;
    const auto margins = li->margins();
    switch (vAlign) {
    case Qt::AlignTop:
        return containerY + margins.top();
    case Qt::AlignBottom:
        return containerY + containerHeight - itemHeight - margins.bottom();
    default: // AlignVCenter
        return containerY + margins.top()
            + (containerHeight - margins.top() - margins.bottom() - itemHeight) / 2.0;
    }
}

QQStyleKitLayoutItem::QQStyleKitLayoutItem(QObject *parent)
    : QObject(parent)
{
}

QQuickItem *QQStyleKitLayoutItem::item() const
{
    return m_item;
}

void QQStyleKitLayoutItem::setItem(QQuickItem *item)
{
    if (m_item == item)
        return;

    if (m_item)
        disconnect(m_item, nullptr, this, nullptr);

    m_item = item;
    if (m_item) {
        connect(m_item, &QQuickItem::implicitWidthChanged, this, [this]() { emit itemChanged(); });
        connect(m_item, &QQuickItem::implicitHeightChanged, this, [this]() { emit itemChanged(); });
        connect(m_item, &QQuickItem::visibleChanged, this, [this]() { emit itemChanged(); });
    }
    // TODO: parentchanged
    emit itemChanged();
}

qreal QQStyleKitLayoutItem::x() const
{
    return m_x;
}

void QQStyleKitLayoutItem::setX(qreal x)
{
    if (qFuzzyCompare(m_x, x))
        return;

    m_x = x;
    emit xChanged();
}

qreal QQStyleKitLayoutItem::y() const
{
    return m_y;
}

void QQStyleKitLayoutItem::setY(qreal y)
{
    if (qFuzzyCompare(m_y, y))
        return;

    m_y = y;
    emit yChanged();
}

qreal QQStyleKitLayoutItem::width() const
{
    return m_width;
}

void QQStyleKitLayoutItem::setWidth(qreal width)
{
    if (qFuzzyCompare(m_width, width))
        return;

    m_width = width;
    emit widthChanged();
}

qreal QQStyleKitLayoutItem::height() const
{
    return m_height;
}

void QQStyleKitLayoutItem::setHeight(qreal height)
{
    if (qFuzzyCompare(m_height, height))
        return;

    m_height = height;
    emit heightChanged();
}

Qt::Alignment QQStyleKitLayoutItem::alignment() const
{
    return m_alignment;
}

void QQStyleKitLayoutItem::setAlignment(Qt::Alignment alignment)
{
    if (m_alignment == alignment)
        return;

    m_alignment = alignment;
    emit alignmentChanged();
}

QMarginsF QQStyleKitLayoutItem::margins() const
{
    return m_margins;
}

void QQStyleKitLayoutItem::setMargins(const QMarginsF &margins)
{
    if (m_margins == margins)
        return;

    m_margins = margins;
    emit marginsChanged();
}

bool QQStyleKitLayoutItem::fillWidth() const
{
    return m_fillWidth;
}

void QQStyleKitLayoutItem::setFillWidth(bool fill)
{
    if (m_fillWidth == fill)
        return;

    m_fillWidth = fill;
    emit fillWidthChanged();
}

bool QQStyleKitLayoutItem::fillHeight() const
{
    return m_fillHeight;
}

void QQStyleKitLayoutItem::setFillHeight(bool fill)
{
    if (m_fillHeight == fill)
        return;

    m_fillHeight = fill;
    emit fillHeightChanged();
}

QQStyleKitLayout::QQStyleKitLayout(QObject *parent)
    : QObject(parent)
    , m_mirrored(false)
    , m_enabled(true)
    , m_updatingLayout(false)
{
    m_updateTimer.setSingleShot(true);
    connect(&m_updateTimer, &QTimer::timeout, this, &QQStyleKitLayout::updateLayout);
}

QQuickItem *QQStyleKitLayout::container() const
{
    return m_container;
}

void QQStyleKitLayout::setContainer(QQuickItem *container)
{
    if (m_container == container)
        return;

    m_container = container;
    emit containerChanged();
    connect(m_container, &QQuickItem::widthChanged, this, &QQStyleKitLayout::scheduleUpdate);
    connect(m_container, &QQuickItem::heightChanged, this, &QQStyleKitLayout::scheduleUpdate);

    scheduleUpdate();
}

QQmlListProperty<QQStyleKitLayoutItem> QQStyleKitLayout::layoutItems()
{
    return QQmlListProperty<QQStyleKitLayoutItem>(const_cast<QQStyleKitLayout *>(this),
                                                          nullptr,
                                                          &QQStyleKitLayout::layoutItem_append,
                                                          &QQStyleKitLayout::layoutItem_count,
                                                          &QQStyleKitLayout::layoutItem_at,
                                                          &QQStyleKitLayout::layoutItem_clear);
}

void QQStyleKitLayout::layoutItem_append(QQmlListProperty<QQStyleKitLayoutItem> *list, QQStyleKitLayoutItem *item)
{
    QQStyleKitLayout *layout = qobject_cast<QQStyleKitLayout *>(list->object);
    if (layout && item) {
        layout->m_layoutItems.append(item);
        connect(item, &QQStyleKitLayoutItem::itemChanged, layout, &QQStyleKitLayout::scheduleUpdate);
        connect(item, &QQStyleKitLayoutItem::alignmentChanged, layout, &QQStyleKitLayout::scheduleUpdate);
        connect(item, &QQStyleKitLayoutItem::marginsChanged, layout, &QQStyleKitLayout::scheduleUpdate);
        connect(item, &QQStyleKitLayoutItem::fillWidthChanged, layout, &QQStyleKitLayout::scheduleUpdate);
        connect(item, &QQStyleKitLayoutItem::fillHeightChanged, layout, &QQStyleKitLayout::scheduleUpdate);
        emit layout->layoutItemsChanged();
        layout->scheduleUpdate();
    }
}

qsizetype QQStyleKitLayout::layoutItem_count(QQmlListProperty<QQStyleKitLayoutItem> *list)
{
    QQStyleKitLayout *layout = qobject_cast<QQStyleKitLayout *>(list->object);
    if (layout)
        return layout->m_layoutItems.size();
    return 0;
}

QQStyleKitLayoutItem *QQStyleKitLayout::layoutItem_at(QQmlListProperty<QQStyleKitLayoutItem> *list, qsizetype index)
{
    QQStyleKitLayout *layout = qobject_cast<QQStyleKitLayout *>(list->object);
    if (layout)
        return layout->m_layoutItems.value(index);
    return nullptr;
}

void QQStyleKitLayout::layoutItem_clear(QQmlListProperty<QQStyleKitLayoutItem> *list)
{
    QQStyleKitLayout *layout = qobject_cast<QQStyleKitLayout *>(list->object);
    if (layout) {
        layout->m_layoutItems.clear();
        emit layout->layoutItemsChanged();
        layout->scheduleUpdate();
    }
}

QMarginsF QQStyleKitLayout::padding() const
{
    return m_padding;
}

QMarginsF QQStyleKitLayout::contentMargins() const
{
    return m_contentMargins;
}

void QQStyleKitLayout::setContentMargins(const QMarginsF &margins)
{
    if (m_contentMargins == margins)
        return;

    m_contentMargins = margins;
    emit contentMarginsChanged();
    scheduleUpdate();
}

qreal QQStyleKitLayout::spacing() const
{
    return m_spacing;
}

void QQStyleKitLayout::setSpacing(qreal spacing)
{
    if (qFuzzyCompare(m_spacing, spacing))
        return;

    m_spacing = spacing;
    emit spacingChanged();
    scheduleUpdate();
}

bool QQStyleKitLayout::isMirrored() const
{
    return m_mirrored;
}

void QQStyleKitLayout::setMirrored(bool mirrored)
{
    if (m_mirrored == mirrored)
        return;

    m_mirrored = mirrored;
    emit mirroredChanged();
    scheduleUpdate();
}

qreal QQStyleKitLayout::implicitWidth() const
{
    return m_implicitWidth;
}

qreal QQStyleKitLayout::implicitHeight() const
{
    return m_implicitHeight;
}

void QQStyleKitLayout::setImplicitWidth(qreal width)
{
    if (qFuzzyCompare(m_implicitWidth, width))
        return;

    m_implicitWidth = width;
    emit implicitWidthChanged();
    scheduleUpdate();
}

void QQStyleKitLayout::setImplicitHeight(qreal height)
{
    if (qFuzzyCompare(m_implicitHeight, height))
        return;

    m_implicitHeight = height;
    emit implicitHeightChanged();
    scheduleUpdate();
}

bool QQStyleKitLayout::isEnabled() const
{
    return m_enabled;
}

void QQStyleKitLayout::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    emit enabledChanged();

    if (m_enabled)
        scheduleUpdate();
}

void QQStyleKitLayout::updateLayout()
{
    if (!m_enabled)
        return;

    if (!m_container || m_container->width() <= 0 || m_container->height() <= 0)
        return;

    if (m_updatingLayout)
        return;
    m_updatingLayout = true;

    QList<QQStyleKitLayoutItem *> left;
    QList<QQStyleKitLayoutItem *> right;
    QList<QQStyleKitLayoutItem *> center;

    for (QQStyleKitLayoutItem *li : m_layoutItems) {
        if (!li->item() || !li->item()->isVisible())
            continue;
        const auto hAlign = li->alignment() & Qt::AlignHorizontal_Mask;
        const bool isMirrored = m_mirrored && !(hAlign & Qt::AlignAbsolute);
        switch (hAlign) {
            case Qt::AlignLeft:
            if (isMirrored)
                right.append(li);
            else
                left.append(li);
            break;
        case Qt::AlignRight:
            if (isMirrored)
                left.append(li);
            else
                right.append(li);
            break;
        default:
            center.append(li);
            break;
        }
    }

    const qreal containerWidth = m_container->width() ? m_container->width() : m_container->implicitWidth();
    const qreal containerHeight = m_container->height() ? m_container->height() : m_container->implicitHeight();
    const qreal paddedX = m_contentMargins.left();
    const qreal paddedY = m_contentMargins.top();
    const qreal paddedWidth = qMax(containerWidth - m_contentMargins.left() - m_contentMargins.right(), .0);
    const qreal paddedHeight = qMax(containerHeight - m_contentMargins.top() - m_contentMargins.bottom(), .0);

    // Position left-aligned items
    {
        qreal x = paddedX;
        for (QQStyleKitLayoutItem *li : left) {
            QQuickItem *item = li->item();
            if (!item || !item->isVisible())
                continue;

            const QMarginsF margins = li->margins();
            const qreal itemWidth = width(li, paddedWidth);
            const qreal itemHeight = height(li, paddedHeight);
            auto y = vAlignY(li, paddedY, paddedHeight);
            li->setX(x + margins.left());
            li->setY(y);
            li->setWidth(itemWidth);
            li->setHeight(itemHeight);
            x += itemWidth + margins.left() + margins.right() + m_spacing;
        }
    }

    // Position right-aligned items
    {
        qreal x = paddedX + paddedWidth;
        for (QQStyleKitLayoutItem *li : right) {
            QQuickItem *item = li->item();
            if (!item || !item->isVisible())
                continue;

            const QMarginsF margins = li->margins();
            const qreal itemWidth = width(li, paddedWidth);
            const qreal itemHeight = height(li, paddedHeight);
            x -= itemWidth + margins.right() + margins.left();
            auto y = vAlignY(li, paddedY, paddedHeight);
            li->setX(x + margins.left());
            li->setY(y);
            li->setWidth(itemWidth);
            li->setHeight(itemHeight);
            x -= m_spacing;
        }
    }

    // Position center-aligned items
    {
        qreal x = paddedX + (paddedWidth - totalWidth(center, m_spacing)) / 2;
        for (QQStyleKitLayoutItem *li : center) {
            QQuickItem *item = li->item();
            if (!item || !item->isVisible())
                continue;

            const QMarginsF margins = li->margins();
            const qreal itemWidth = width(li, paddedWidth);
            const qreal itemHeight = height(li, paddedHeight);
            auto y = vAlignY(li, paddedY, paddedHeight);
            li->setX(x + margins.left());
            li->setY(y);
            li->setWidth(itemWidth);
            li->setHeight(itemHeight);
            x += itemWidth + margins.left() + margins.right() + m_spacing;
        }
    }

    const auto leftWidth = totalWidth(left, m_spacing);
    const auto leftHeight = totalHeight(left);
    const auto rightWidth = totalWidth(right, m_spacing);
    const auto rightHeight = totalHeight(right);
    const auto centerWidth = totalWidth(center, m_spacing);
    const auto centerHeight = totalHeight(center);

    const auto implicitWidth = leftWidth + rightWidth + centerWidth
        - m_spacing * (left.isEmpty() ? 0 : 1)
        - m_spacing * (right.isEmpty() ? 0 : 1)
        - m_spacing * (center.isEmpty() ? 0 : 1)
        + m_contentMargins.left() + m_contentMargins.right();
    setImplicitWidth(implicitWidth);
    const auto implicitHeight = qMax(qMax(leftHeight, rightHeight), centerHeight)
        + m_contentMargins.top() + m_contentMargins.bottom();
    setImplicitHeight(implicitHeight);

    // HACK for control's contentItem
    // QQuickControl determines the contentItem geometry based on the control size and padding
    // So we include the layout's left/right widths into the padding calculation
    auto leftPadding = m_contentMargins.left() + leftWidth;
    auto topPadding = m_contentMargins.top(); // TODO: support vertical layout items
    auto rightPadding = m_contentMargins.right() + rightWidth;
    auto bottomPadding = m_contentMargins.bottom(); // TODO: support vertical layout items
    if (isMirrored())
        std::swap(leftPadding, rightPadding);
    QMarginsF newPadding = QMarginsF(leftPadding, topPadding, rightPadding, bottomPadding);
    if (m_padding != newPadding) {
        m_padding = newPadding;
        emit paddingChanged();
    }
    m_updatingLayout = false;
}

void QQStyleKitLayout::scheduleUpdate()
{
    if (!m_updateTimer.isActive())
        m_updateTimer.start(0);
}

QT_END_NAMESPACE

#include "moc_qqstylekitlayout_p.cpp"
