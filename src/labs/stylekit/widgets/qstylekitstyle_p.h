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

#include <QtCore/qset.h>
#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/qcommonstyle.h>
#include <QtWidgets/private/qcommonstyle_p.h>
#include <QtQml/qqmlengine.h>
#include "qstylekitstyle.h"
#include <QtLabsStyleKit/private/qqstylekitreader_p.h>

#include <variant>

QT_BEGIN_NAMESPACE

class QAction;
class QQuickTransition;
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
    // Key to identify a sub-element inside a widget: item-view cells,
    // a menu/menu-bar item, etc.
    // Needed for animating sub-elements
    struct SubElementKey
    {
        const QWidget *widget = nullptr;
        // used for item-view cells
        struct ItemViewCell
        {
            const void *model = nullptr;
            quintptr internalId = 0;
            int row = -1;
            int column = -1;
            friend bool operator==(const ItemViewCell &l, const ItemViewCell &r) noexcept
            {
                return l.model == r.model && l.internalId == r.internalId
                    && l.row == r.row && l.column == r.column;
            }
        };
        // used for menu/menu-bar items
        struct Action
        {
            const QAction *action = nullptr;
            friend bool operator==(const Action &l, const Action &r) noexcept
            { return l.action == r.action; }
        };
        // used for tab-bar tabs
        struct Tab
        {
            int index = -1;
            friend bool operator==(const Tab &l, const Tab &r) noexcept
            { return l.index == r.index; }
        };
        std::variant<std::monostate, ItemViewCell, Action, Tab> id;

        bool isValid() const noexcept
        { return widget != nullptr && !std::holds_alternative<std::monostate>(id); }
        friend bool operator==(const SubElementKey &l, const SubElementKey &r) noexcept
        { return l.widget == r.widget && l.id == r.id; }
        friend size_t qHash(const SubElementKey &k, size_t seed = 0) noexcept
        {
            if (auto *c = std::get_if<ItemViewCell>(&k.id))
                return qHashMulti(seed, k.widget, 1, c->model, c->internalId, c->row, c->column);
            if (auto *a = std::get_if<Action>(&k.id))
                return qHashMulti(seed, k.widget, 2, a->action);
            if (auto *t = std::get_if<Tab>(&k.id))
                return qHashMulti(seed, k.widget, 3, t->index);
            return qHashMulti(seed, k.widget, 0);
        }
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

    QQStyleKitStyle *ensureDefaultStyle();
    QQStyleKitStyle *effectiveStyle() const;

    void unsetStyleFont(QWidget *widget);
    void setStyleFont(QWidget *widget, const QFont &styleFont);
    void refreshStyleFont(QWidget *widget);

    void unsetStylePalette(QWidget *widget);
    void setStylePalette(QWidget *widget, const QPalette &stylePalette) const;
    void refreshStylePalette(QWidget *widget);

    QQStyleKitReader *readerForWidget(const QWidget *widget) const;
    void cleanupWidgetReader(const QWidget *widget) const;
    void onWidgetDestroyed(QObject *w) const;

    QQStyleKitReader *ensureSharedReader() const;
    QQStyleKitReader *ensureSubElementReader() const;

    static SubElementKey subElementKeyForOption(const QStyleOption *opt, const QWidget *widget);
    QQStyleKitReader *startSubElementTransition(const SubElementKey &key,
                                                QQStyleKitReader::ControlType type,
                                                QQSK::State fromState, QQSK::State toState,
                                                QQuickTransition *transition) const;
    void cleanupSubElements(const QWidget *widget) const;
    void clearAllSubElements() const;

    QQSK::State resolvedStateFor(QQStyleKitReader::ControlType type, QStyle::State state,
                                 const QWidget *widget = nullptr) const;

    QQStyleKitResolved resolve(const QWidget *w, QQStyleKitReader::ControlType type, QStyle::State state) const;
    QQStyleKitResolved resolveSubElement(const QWidget *w, const QStyleOption *opt,
                                         QQStyleKitReader::ControlType type, QStyle::State state,
                                         bool track = true) const;

    QQStyleKitLayoutResolved resolveLayout(QQStyleKitReader::ControlType type,
                                           QStyle::State state) const;

    const ControlMetrics &metricsFor(QQStyleKitReader::ControlType type, QQSK::State state) const;
    void clearMetricsCache();

    ControlMetrics metricsForReader(QQStyleKitReader *reader) const;

    void drawControlIndicator(const QQStyleKitDelegateProperties *indicator, const QRectF &rect, QPainter *p) const;
    void drawControlText(const QQStyleKitTextProperties *textProps, const QFont &font,
                         const QRect &rect, const QString &text, uint textFlags,
                         QPainter *p,
                         Qt::Alignment defaultAlignment = Qt::AlignHCenter | Qt::AlignVCenter) const;
    void drawStyledItemRect(const QQStyleKitDelegateProperties *, const QRectF &rect, QPainter *p) const;
    void drawStyledItemContents(const QQStyleKitDelegateProperties *, const QRectF &rect, QPainter *p) const;
    void drawStyledItemImage(const QQStyleKitImageProperties *image, const QRectF &rect,
                             qreal opacity, QPainter *p) const;

    QRect getAlignedRectInContainer(const QRect &container, const QSize &indicatorSize,
                                       uint alignment, const QMargins &padding,
                                       const QMargins &indicatorMargins) const;

    QQmlEngine *qmlEngine = nullptr;
    QQStyleKitStyle *style = nullptr;
    QQStyleKitStyle *defaultStyle = nullptr;
    // Shared reader for static metric reads
    mutable QQStyleKitReader *sharedReader = nullptr;
    // Per-widget readers
    // Separate per-widget readers are used in order to support transitions,
    // as each reader provides interpolated property values during these transitions.
    mutable QHash<const QWidget *, QQStyleKitReader *> widgetReaders;
    // Shared reader for static (ie: not currently animating) sub-element reads
    mutable QQStyleKitReader *subElementReader = nullptr;
    // Last transitioned-to state for the sub-element
    mutable QHash<SubElementKey, QQSK::State> subElementStates;
    // Transient readers, one per animating sub-element
    mutable QHash<SubElementKey, QQStyleKitReader *> subElementAnimationReaders;
    static constexpr int kMaxSubElementAnimationReaders = 64;
    static constexpr int kMaxSubElementStates = 1024;
    // Cache of resolved metrics per control type and state
    mutable QHash<MetricsCacheKey, ControlMetrics> metricsCache;
    QHash<const QWidget *, Tampered<QFont>> customFontWidgets;
    mutable QHash<const QWidget *, Tampered<QPalette>> customPaletteWidgets;
    // Widgets whose automatic background fill we disabled
    QSet<const QWidget *> bgFillDisabledWidgets;
    QString stylePath;
};

QT_END_NAMESPACE

#endif // QSTYLEKITSTYLE_P_H
