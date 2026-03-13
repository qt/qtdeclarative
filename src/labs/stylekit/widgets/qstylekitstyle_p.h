// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSTYLEKITSTYLE_P_H
#define QSTYLEKITSTYLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/qcommonstyle.h>
#include <QtWidgets/private/qcommonstyle_p.h>
#include <QtQml/qqmlengine.h>
#include "qstylekitstyle.h"
#include <QtLabsStyleKit/private/qqstylekitreader_p.h>

QT_BEGIN_NAMESPACE

class QQStyleKitDelegateProperties;
class QQStyleKitHandleProperties;
class QQStyleKitImageProperties;
class QQStyleKitIndicatorWithSubTypes;
class QQStyleKitTextProperties;
class QQStyleKitReader;
class QQStyleKitStyle;
class QStyleKitStylePrivate : public QCommonStylePrivate
{
    Q_DECLARE_PUBLIC(QStyleKitStyle)

    // Stores all the metrics needed to draw and size a control.
    // Cached per-control type and state so that we don't have to do multiple
    // lookups on the reader for each property when drawing
    struct ControlMetrics
    {
        QMargins textPadding;
        QMargins padding;
        QMargins margins;
        QSize bgImplicitSize;
        QSize indicatorImplicitSize;
        QMargins indicatorMargins;
        QSize foregroundImplicitSize;
        QMargins foregroundMargins;
        QSize handleImplicitSize;
        QMargins handleMargins;
        int spacing;
    };
    // Key for the metrics cache
    struct MetricsCacheKey
    {
        QQStyleKitReader::ControlType type;
        QQSK::State state;

        friend bool operator==(MetricsCacheKey lhs, MetricsCacheKey rhs) noexcept
        { return lhs.type == rhs.type && lhs.state == rhs.state; }
        friend size_t qHash(MetricsCacheKey key, size_t seed = 0) noexcept
        { return qHashMulti(seed, key.type, key.state); }
    };

    // Stores the resolved properties for a control in a specific state.
    // Used by painters to read properties when drawing.
    // The reader stays internal and we expose only the property accessors
    // to avoid mutating the reader and reconfigure resolution mid-paint.
    struct QQStyleKitResolved
    {
        const QQStyleKitReader *reader = nullptr;
        const ControlMetrics *metrics = nullptr;
        const QWidget *widget = nullptr;

        bool isValid() const { return reader && metrics; }
        const QQStyleKitTextProperties *text() const;
        const QQStyleKitDelegateProperties *background() const;
        const QQStyleKitHandleProperties *handle() const;
        const QQStyleKitIndicatorWithSubTypes *indicator() const;
        QFont font() const;
    };
    // Stores the resolved properties for layout purposes.
    // Accessors read static values via the shared reader (transitions disabled),
    // so layout never depends on animation state.
    struct QQStyleKitLayoutResolved
    {
        const ControlMetrics *metrics = nullptr;
        const QQStyleKitControlProperties *staticProps = nullptr;
        QFont staticFont;

        bool isValid() const { return metrics; }
        const QQStyleKitTextProperties *text() const;
        const QQStyleKitDelegateProperties *background() const;
        const QQStyleKitHandleProperties *handle() const;
        const QQStyleKitIndicatorWithSubTypes *indicator() const;
        QFont font() const { return staticFont; }
    };

    // Copied from qstylesheetstyle_p.h
    template <typename T>
    struct Tampered {
        T oldWidgetValue;
        decltype(std::declval<T>().resolveMask()) resolveMask;

        // only call this function on an rvalue *this (it mangles oldWidgetValue)
        T reverted(T current) && {
            oldWidgetValue.setResolveMask(oldWidgetValue.resolveMask() & resolveMask);
            current.setResolveMask(current.resolveMask() & ~resolveMask);
            current.resolve(oldWidgetValue);
            current.setResolveMask(current.resolveMask() | oldWidgetValue.resolveMask());
            return current;
        }
    };

private:
    QStyleKitStylePrivate();

    bool loadStyle();
    void updateStyle();

    void unsetStyleFont(QWidget *widget);
    void setStyleFont(QWidget *widget);

    QQStyleKitReader *readerForWidget(const QWidget *widget) const;
    void cleanupWidgetReader(const QWidget *widget) const;

    QQStyleKitReader *readerForItemViewItem(const QWidget *widget, quint64 itemKey) const;
    void cleanupItemViewItemReaders(const QWidget *widget) const;
    static quint64 itemViewItemKeyForOption(const QStyleOption *opt);

    QQStyleKitReader *ensureSharedReader() const;

    QQSK::State resolvedStateFor(QQStyleKitReader::ControlType type, QStyle::State state) const;

    QQStyleKitResolved resolve(const QWidget *w, QQStyleKitReader::ControlType type, QStyle::State state) const;
    QQStyleKitResolved resolveItemViewItem(const QWidget *w, const QStyleOption *opt,
                                   QQStyleKitReader::ControlType type, QStyle::State state) const;

    QQStyleKitLayoutResolved resolveLayout(QQStyleKitReader::ControlType type,
                                           QStyle::State state) const;

    const ControlMetrics &metricsFor(QQStyleKitReader::ControlType type, QQSK::State state) const;
    void clearMetricsCache();

    ControlMetrics metricsForReader(QQStyleKitReader *reader) const;

    void drawControlIndicator(const QQStyleKitDelegateProperties *indicator, const QRectF &rect, QPainter *p) const;
    void drawControlText(const QQStyleKitTextProperties *textProps, const QFont &font,
                         const QRect &rect, const QString &text, uint textFlags, QPainter *p) const;
    void drawStyledItemRect(const QQStyleKitDelegateProperties *, const QRectF &rect, QPainter *p) const;
    void drawStyledItemImage(const QQStyleKitImageProperties *image, const QRectF &rect,
                             qreal opacity, QPainter *p) const;

    QRect getAlignedRectInContainer(const QRect &container, const QSize &indicatorSize,
                                       uint alignment, const QMargins &padding,
                                       const QMargins &indicatorMargins) const;

    QQmlEngine *qmlEngine = nullptr;
    QQStyleKitStyle *style = nullptr;
    // Reader for static metric reads
    mutable QQStyleKitReader *sharedReader = nullptr;
    // Per-widget readers
    // Separate per-widget readers are used in order to support transitions,
    // as each reader provides interpolated property values during these transitions.
    mutable QHash<const QWidget *, QQStyleKitReader *> widgetReaders;
    // Per-item readers for item-view items
    mutable QHash<const QWidget *, QHash<quint64, QQStyleKitReader *>> itemViewItemReaders;
    // Cache of resolved metrics per control type and state
    mutable QHash<MetricsCacheKey, ControlMetrics> metricsCache;
    QHash<const QWidget *, Tampered<QFont>> customFontWidgets;
    QString stylePath;
};

QT_END_NAMESPACE

#endif // QSTYLEKITSTYLE_P_H
