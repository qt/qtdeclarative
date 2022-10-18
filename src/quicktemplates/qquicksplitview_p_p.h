// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSPLITVIEW_P_P_H
#define QQUICKSPLITVIEW_P_P_H

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

#include <QtQuickTemplates2/private/qquickcontainer_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickSplitView;
class QQuickSplitViewAttached;
class QQuickSplitHandleAttached;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickSplitViewPrivate : public QQuickContainerPrivate
{
    Q_DECLARE_PUBLIC(QQuickSplitView)

public:
    void updateFillIndex();
    void layoutResizeSplitItems(qreal &usedWidth, qreal &usedHeight, int &indexBeingResizedDueToDrag);
    void layoutResizeFillItem(QQuickItem *fillItem, qreal &usedWidth, qreal &usedHeight, int indexBeingResizedDueToDrag);
    void limitAndApplySizes(qreal usedWidth, qreal usedHeight);
    void layoutPositionItems(const QQuickItem *fillItem);
    void requestLayout();
    void layout();
    void createHandles();
    void createHandleItem(int index);
    void removeExcessHandles();
    void destroyHandles();
    void resizeHandle(QQuickItem *handleItem);
    void resizeHandles();
#if QT_CONFIG(cursor)
    void updateCursorHandle(QQuickItem *handleItem);
#endif
    void updateHandleVisibilities();
    void updateHoveredHandle(QQuickItem *hoveredItem);
    void setResizing(bool resizing);

    bool isHorizontal() const;
    qreal accumulatedSize(int firstIndex, int lastIndex) const;

    struct EffectiveSizeData {
        qreal effectiveMinimumWidth;
        qreal effectiveMinimumHeight;
        qreal effectivePreferredWidth;
        qreal effectivePreferredHeight;
        qreal effectiveMaximumWidth;
        qreal effectiveMaximumHeight;
    };

    // Used during the layout
    struct LayoutData {
        qreal width = 0;
        qreal height = 0;
        bool wasResizedByHandle = false;
    };

    EffectiveSizeData effectiveSizeData(const QQuickItemPrivate *itemPrivate,
        const QQuickSplitViewAttached *attached) const;

    int handleIndexForSplitIndex(int splitIndex) const;

    QQuickItem *getContentItem() override;
    bool handlePress(const QPointF &point, ulong timestamp) override;
    bool handleMove(const QPointF &point, ulong timestamp) override;
    bool handleRelease(const QPointF &point, ulong timestamp) override;

    void itemVisibilityChanged(QQuickItem *item) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

    void updatePolish() override;

    static QQuickSplitViewPrivate *get(QQuickSplitView *splitView);

    Qt::Orientation m_orientation = Qt::Horizontal;
    QQmlComponent *m_handle = nullptr;
    QList<QQuickItem*> m_handleItems;
    QHash<QQuickItem*, LayoutData> m_layoutData;
    int m_hoveredHandleIndex = -1;
    int m_pressedHandleIndex = -1;
    int m_nextVisibleIndexAfterPressedHandle = -1;
    QPointF m_pressPos;
    QPointF m_mousePos;
    QPointF m_handlePosBeforePress;
    qreal m_leftOrTopItemSizeBeforePress = 0.0;
    qreal m_rightOrBottomItemSizeBeforePress = 0.0;
    int m_fillIndex = -1;
    bool m_layingOut = false;
    bool m_ignoreNextLayoutRequest = false;
    bool m_resizing = false;
};

class QQuickSplitViewAttachedPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickSplitViewAttached)

public:
    QQuickSplitViewAttachedPrivate();

    void setView(QQuickSplitView *newView);
    void requestLayoutView();

    static QQuickSplitViewAttachedPrivate *get(QQuickSplitViewAttached *attached);
    static const QQuickSplitViewAttachedPrivate *get(const QQuickSplitViewAttached *attached);

    QQuickItem *m_splitItem = nullptr;
    QQuickSplitView *m_splitView = nullptr;

    unsigned m_fillWidth : 1;
    unsigned m_fillHeight : 1;
    unsigned m_isFillWidthSet : 1;
    unsigned m_isFillHeightSet : 1;
    unsigned m_isMinimumWidthSet : 1;
    unsigned m_isMinimumHeightSet : 1;
    unsigned m_isPreferredWidthSet : 1;
    unsigned m_isPreferredHeightSet : 1;
    unsigned m_isMaximumWidthSet : 1;
    unsigned m_isMaximumHeightSet : 1;
    qreal m_minimumWidth;
    qreal m_minimumHeight;
    qreal m_preferredWidth;
    qreal m_preferredHeight;
    qreal m_maximumWidth;
    qreal m_maximumHeight;
};

class QQuickSplitHandleAttachedPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickSplitHandleAttached)

public:
    QQuickSplitHandleAttachedPrivate();

    void setHovered(bool hovered);
    void setPressed(bool pressed);

    static QQuickSplitHandleAttachedPrivate *get(QQuickSplitHandleAttached *attached);
    static const QQuickSplitHandleAttachedPrivate *get(const QQuickSplitHandleAttached *attached);

    unsigned m_hovered : 1;
    unsigned m_pressed : 1;
};

QT_END_NAMESPACE

#endif // QQUICKSPLITVIEW_P_P_H
