// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QMACSTYLE_MAC_P_P_H
#define QMACSTYLE_MAC_P_P_H

#include "qquickmacstyle_mac_p.h"
#include "qquickcommonstyle_p.h"
#include "qquickstylehelper_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qhash.h>
#include <QtCore/qmap.h>
#include <QtCore/qmath.h>
#include <QtCore/qpair.h>
#include <QtCore/qpointer.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qvector.h>

#include <QtGui/qbitmap.h>
#include <QtGui/qevent.h>
#include <QtGui/qpaintdevice.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpixmapcache.h>

#include <QtQuick/qquickitem.h>

#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/private/qpainter_p.h>

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

Q_FORWARD_DECLARE_MUTABLE_CG_TYPE(CGContext);

Q_FORWARD_DECLARE_OBJC_CLASS(NSView);
Q_FORWARD_DECLARE_OBJC_CLASS(NSCell);

QT_BEGIN_NAMESPACE

namespace QQC2 {

/*
    AHIG:
        macOS Human Interface Guidelines
        https://developer.apple.com/macos/human-interface-guidelines/overview/themes/

    Builder:
        Interface Builder in Xcode 8 or later
*/

// this works as long as we have at most 16 different control types
#define CT1(c) CT2(c, c)
#define CT2(c1, c2) ((uint(c1) << 16) | uint(c2))

#define SIZE(large, small, mini) \
    (controlSize == QStyleHelper::SizeLarge ? (large) : controlSize == QStyleHelper::SizeSmall ? (small) : (mini))

// same as return SIZE(...) but optimized
#define return_SIZE(large, small, mini) \
    do { \
        static const int sizes[] = { (large), (small), (mini) }; \
        return sizes[controlSize]; \
    } while (false)

class QMacStylePrivate : public QCommonStylePrivate
{
    Q_DECLARE_PUBLIC(QMacStyle)

public:
    enum Direction {
        North, South, East, West
    };

    enum CocoaControlType {
        NoControl,    // For when there's no such a control in Cocoa
        Box,          // QGroupBox
        Box_Dark,     // FIXME See render code in drawPrimitive(PE_FrameTabWidget)
        Button_CheckBox,
        Button_Disclosure,  // Disclosure triangle, like in QTreeView
        Button_PopupButton,  // Non-editable QComboBox
        Button_PullDown, // QPushButton with menu
        Button_PushButton, // Plain QPushButton and QTabBar buttons
        Button_RadioButton,
        Button_SquareButton, // Oversized QPushButton
        Button_WindowClose,
        Button_WindowMiniaturize,
        Button_WindowZoom,
        ComboBox,     // Editable QComboBox
        ProgressIndicator_Determinate,
        ProgressIndicator_Indeterminate,
        Scroller_Horizontal,
        Scroller_Vertical,
        SegmentedControl_First, // QTabBar buttons focus ring
        SegmentedControl_Middle,
        SegmentedControl_Last,
        SegmentedControl_Single,
        Slider_Horizontal,
        Slider_Vertical,
        SplitView_Horizontal,
        SplitView_Vertical,
        Stepper,      // QSpinBox buttons
        TextField
    };

    struct CocoaControl {
        CocoaControl();
        CocoaControl(CocoaControlType t, QStyleHelper::WidgetSizePolicy s);

        CocoaControlType type;
        QStyleHelper::WidgetSizePolicy size;

        bool operator==(const CocoaControl &other) const;

        QSizeF defaultFrameSize() const;
        QRectF adjustedControlFrame(const QRectF &rect) const;
        QMarginsF titleMargins() const;

        bool getCocoaButtonTypeAndBezelStyle(NSButtonType *buttonType, NSBezelStyle *bezelStyle) const;
    };

    typedef void (^DrawRectBlock)(CGContextRef, const CGRect &);

    QMacStylePrivate();
    ~QMacStylePrivate();

    // Ideally these wouldn't exist, but since they already exist we need some accessors.
    static const int PushButtonLeftOffset;
    static const int PushButtonRightOffset;
    static const int PushButtonContentPadding;

    enum Animates { AquaPushButton, AquaProgressBar, AquaListViewItemOpen, AquaScrollBar };
    QStyleHelper::WidgetSizePolicy aquaSizeConstrain(const QStyleOption *option,
                                                     QStyle::ContentsType ct = QStyle::CT_CustomBase,
                                                     QSize szHint=QSize(-1, -1), QSize *insz = 0) const;
    QStyleHelper::WidgetSizePolicy effectiveAquaSizeConstrain(const QStyleOption *option,
                                                              QStyle::ContentsType ct = QStyle::CT_CustomBase,
                                                              QSize szHint=QSize(-1, -1), QSize *insz = 0) const;
    inline int animateSpeed(Animates) const { return 33; }

    // Utility functions
    static CGRect comboboxInnerBounds(const CGRect &outterBounds, const CocoaControl &cocoaWidget);

    static QRectF comboboxEditBounds(const QRectF &outterBounds, const CocoaControl &cw);

    void setAutoDefaultButton(QObject *button) const;

    NSView *cocoaControl(CocoaControl cocoaControl) const;
    NSCell *cocoaCell(CocoaControl cocoaControl) const;

    void setupNSGraphicsContext(CGContextRef cg, bool flipped) const;
    void restoreNSGraphicsContext(CGContextRef cg) const;

    void setupVerticalInvertedXform(CGContextRef cg, bool reverse, bool vertical, const CGRect &rect) const;

    void drawNSViewInRect(NSView *view, const QRectF &rect, QPainter *p, DrawRectBlock drawRectBlock = nil) const;
    void resolveCurrentNSView(QWindow *window) const;

    void drawToolbarButtonArrow(const QStyleOption *opt, QPainter *p) const;

    QPainterPath windowPanelPath(const QRectF &r) const;

    CocoaControlType windowButtonCocoaControl(QStyle::SubControl sc) const;

    void tabLayout(const QStyleOptionTab *opt, QRect *textRect, QRect *iconRect) const override;
    static Direction tabDirection(QStyleOptionTab::Shape shape);
    static bool verticalTabs(QMacStylePrivate::Direction tabDirection);

public:
    mutable QPointer<QObject> autoDefaultButton;
    static  QVector<QPointer<QObject> > scrollBars;

    mutable QPointer<QQuickItem> focusWidget; // TODO: rename to focusItem
    mutable NSView *backingStoreNSView;
    mutable QHash<CocoaControl, NSView *> cocoaControls;
    mutable QHash<CocoaControl, NSCell *> cocoaCells;

    QFont smallSystemFont;
    QFont miniSystemFont;

    QMacKeyValueObserver appearanceObserver;
};

} // namespace QQC2

QT_END_NAMESPACE

#endif // QMACSTYLE_MAC_P_P_H
