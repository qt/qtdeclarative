// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSTYLEOPTION_H
#define QSTYLEOPTION_H

#include "qquickstyle.h"

#include <QtCore/qlocale.h>
#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>
#include <QtCore/qabstractitemmodel.h>

#include <QtGui/qicon.h>
#include <QtGui/qfontmetrics.h>

QT_BEGIN_NAMESPACE

class QQuickItem;

namespace QQC2 {

class QStyleOption
{
public:
    enum OptionType {
        SO_Default, SO_FocusRect, SO_Button, SO_Tab, SO_MenuItem,
        SO_Frame, SO_ProgressBar, SO_ToolBox, SO_Header,
        SO_DockWidget, SO_ViewItem, SO_TabWidgetFrame,
        SO_TabBarBase, SO_RubberBand, SO_ToolBar, SO_GraphicsItem,

        SO_Complex = 0xf0000, SO_Slider, SO_SpinBox, SO_ToolButton, SO_ComboBox,
        SO_TitleBar, SO_GroupBox, SO_SizeGrip,

        SO_CustomBase = 0xf00,
        SO_ComplexCustomBase = 0xf000000
    };

    enum StyleOptionType { Type = SO_Default };
    enum StyleOptionVersion { Version = 1 };

    int version; // TODO: Remove version information
    int type;
    QStyle::State state;
    Qt::LayoutDirection direction;
    QRect rect;
    QFontMetrics fontMetrics;
    QPalette palette;
    QObject *styleObject;

    // QQC2 additions. Remember to also update copy
    // constructor and assignment operator when adding
    // new variables here.
    QQuickItem *control;
    QWindow *window;

    QStyleOption(int version = QStyleOption::Version, int type = SO_Default);
    QStyleOption(const QStyleOption &other);
    ~QStyleOption();

    QStyleOption &operator=(const QStyleOption &other);
};

class QStyleOptionFocusRect : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_FocusRect };
    enum StyleOptionVersion { Version = 1 };

    QColor backgroundColor;

    QStyleOptionFocusRect();
    QStyleOptionFocusRect(const QStyleOptionFocusRect &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionFocusRect &operator=(const QStyleOptionFocusRect &) = default;

protected:
    QStyleOptionFocusRect(int version);
};

class QStyleOptionFrame : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_Frame };
    enum StyleOptionVersion { Version = 3 };
    enum FrameFeature {
        None = 0x00,
        Flat = 0x01,
        Rounded = 0x02
    };
    Q_DECLARE_FLAGS(FrameFeatures, FrameFeature)
    enum Shape {
        NoFrame  = 0, // no frame
        Box = 0x0001, // rectangular box
        Panel = 0x0002, // rectangular panel
        WinPanel = 0x0003, // rectangular panel (Windows)
        HLine = 0x0004, // horizontal line
        VLine = 0x0005, // vertical line
        StyledPanel = 0x0006 // rectangular panel depending on the GUI style
    };
    enum Shadow {
        Plain = 0x0010, // plain line
        Raised = 0x0020, // raised shadow effect
        Sunken = 0x0030 // sunken shadow effect
    };

    int lineWidth;
    int midLineWidth;
    FrameFeatures features;
    Shape frameShape;

    QStyleOptionFrame();
    QStyleOptionFrame(const QStyleOptionFrame &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionFrame &operator=(const QStyleOptionFrame &) = default;

protected:
    QStyleOptionFrame(int version);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QStyleOptionFrame::FrameFeatures)
Q_DECLARE_MIXED_ENUM_OPERATORS_SYMMETRIC(int, QStyleOptionFrame::Shape, QStyleOptionFrame::Shadow)

Q_DECL_DEPRECATED typedef QStyleOptionFrame QStyleOptionFrameV2;
Q_DECL_DEPRECATED typedef QStyleOptionFrame QStyleOptionFrameV3;

class QStyleOptionTab : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_Tab };
    enum StyleOptionVersion { Version = 3 };

    enum TabPosition { Beginning, Middle, End, OnlyOneTab };
    enum SelectedPosition { NotAdjacent, NextIsSelected, PreviousIsSelected };
    enum CornerWidget { NoCornerWidgets = 0x00, LeftCornerWidget = 0x01,
                        RightCornerWidget = 0x02 };
    enum TabFeature { None = 0x00, HasFrame = 0x01 };
    enum Shape { RoundedNorth, RoundedSouth, RoundedWest, RoundedEast,
                 TriangularNorth, TriangularSouth, TriangularWest, TriangularEast
    };
    Q_DECLARE_FLAGS(CornerWidgets, CornerWidget)
    Q_DECLARE_FLAGS(TabFeatures, TabFeature)

    QString text;
    QIcon icon;
    int row;
    TabPosition position;
    Shape shape = RoundedNorth;
    SelectedPosition selectedPosition;
    CornerWidgets cornerWidgets;
    QSize iconSize;
    bool documentMode;
    QSize leftButtonSize;
    QSize rightButtonSize;
    TabFeatures features;

    QStyleOptionTab();
    QStyleOptionTab(const QStyleOptionTab &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionTab &operator=(const QStyleOptionTab &) = default;

protected:
    QStyleOptionTab(int version);
};

class QStyleOptionTabWidgetFrame : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_TabWidgetFrame };
    enum StyleOptionVersion { Version = 2 };

    int lineWidth;
    int midLineWidth;
    QStyleOptionTab::Shape shape;
    QSize tabBarSize;
    QSize rightCornerWidgetSize;
    QSize leftCornerWidgetSize;
    QRect tabBarRect;
    QRect selectedTabRect;

    QStyleOptionTabWidgetFrame();
    inline QStyleOptionTabWidgetFrame(const QStyleOptionTabWidgetFrame &other)
        : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionTabWidgetFrame &operator=(const QStyleOptionTabWidgetFrame &) = default;

protected:
    QStyleOptionTabWidgetFrame(int version);
};

Q_DECL_DEPRECATED typedef QStyleOptionTabWidgetFrame QStyleOptionTabWidgetFrameV2;


class QStyleOptionTabBarBase : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_TabBarBase };

    enum TabBarPosition { North, South, West, East };
    enum ButtonPosition { LeftSide, RightSide };

    QRect tabBarRect;
    QRect selectedTabRect;
    bool documentMode;
    QStyleOptionTab::Shape shape;

    QStyleOptionTabBarBase();
    QStyleOptionTabBarBase(const QStyleOptionTabBarBase &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionTabBarBase &operator=(const QStyleOptionTabBarBase &) = default;

protected:
    QStyleOptionTabBarBase(int version);
};

class QStyleOptionHeader : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_Header };
    enum StyleOptionVersion { Version = 1 };
    enum SectionPosition { Beginning, Middle, End, OnlyOneSection };
    enum SelectedPosition { NotAdjacent, NextIsSelected, PreviousIsSelected,
                            NextAndPreviousAreSelected };
    enum SortIndicator { None, SortUp, SortDown };

    int section;
    QString text;
    Qt::Alignment textAlignment;
    QIcon icon;
    Qt::Alignment iconAlignment;
    SectionPosition position;
    SelectedPosition selectedPosition;
    SortIndicator sortIndicator;
    Qt::Orientation orientation;

    QStyleOptionHeader();
    QStyleOptionHeader(const QStyleOptionHeader &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionHeader &operator=(const QStyleOptionHeader &) = default;

protected:
    QStyleOptionHeader(int version);
};

class QStyleOptionButton : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_Button };
    enum StyleOptionVersion { Version = 1 };

    enum ButtonFeature { None = 0x00, Flat = 0x01, HasMenu = 0x02, DefaultButton = 0x04,
                         AutoDefaultButton = 0x08, CommandLinkButton = 0x10  };
    Q_DECLARE_FLAGS(ButtonFeatures, ButtonFeature)

    ButtonFeatures features;
    QString text;
    QIcon icon;
    QSize iconSize;

    QStyleOptionButton();
    QStyleOptionButton(const QStyleOptionButton &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionButton &operator=(const QStyleOptionButton &) = default;

protected:
    QStyleOptionButton(int version);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QStyleOptionButton::ButtonFeatures)

class QStyleOptionTabV4 : public QStyleOptionTab
{
public:
    enum StyleOptionVersion { Version = 4 };
    QStyleOptionTabV4();
    int tabIndex = -1;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QStyleOptionTab::CornerWidgets)

Q_DECL_DEPRECATED typedef QStyleOptionTab QStyleOptionTabV2;
Q_DECL_DEPRECATED typedef QStyleOptionTab QStyleOptionTabV3;


class QStyleOptionToolBar : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_ToolBar };
    enum StyleOptionVersion { Version = 1 };
    enum ToolBarPosition { Beginning, Middle, End, OnlyOne };
    enum ToolBarFeature { None = 0x0, Movable = 0x1 };
    Q_DECLARE_FLAGS(ToolBarFeatures, ToolBarFeature)

    ToolBarPosition positionOfLine; // The toolbar line position
    ToolBarPosition positionWithinLine; // The position within a toolbar
    Qt::ToolBarArea toolBarArea; // The toolbar docking area
    ToolBarFeatures features;
    int lineWidth;
    int midLineWidth;

    QStyleOptionToolBar();
    QStyleOptionToolBar(const QStyleOptionToolBar &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionToolBar &operator=(const QStyleOptionToolBar &) = default;

protected:
    QStyleOptionToolBar(int version);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QStyleOptionToolBar::ToolBarFeatures)

class QStyleOptionProgressBar : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_ProgressBar };
    enum StyleOptionVersion { Version = 2 };

    int minimum;
    int maximum;
    int progress;
    QString text;
    Qt::Alignment textAlignment;
    bool textVisible;
    bool invertedAppearance;
    bool bottomToTop;

    QStyleOptionProgressBar();
    QStyleOptionProgressBar(const QStyleOptionProgressBar &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionProgressBar &operator=(const QStyleOptionProgressBar &) = default;

protected:
    QStyleOptionProgressBar(int version);
};

class QStyleOptionMenuItem : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_MenuItem };
    enum StyleOptionVersion { Version = 1 };

    enum MenuItemType { Normal, DefaultItem, Separator, SubMenu, Scroller, TearOff, Margin,
                        EmptyArea };
    enum CheckType { NotCheckable, Exclusive, NonExclusive };

    MenuItemType menuItemType;
    CheckType checkType;
    bool checked;
    bool menuHasCheckableItems;
    QRect menuRect;
    QString text;
    QIcon icon;
    int maxIconWidth;
    int tabWidth; // ### Qt 6: rename to reservedShortcutWidth
    QFont font;

    QStyleOptionMenuItem();
    QStyleOptionMenuItem(const QStyleOptionMenuItem &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionMenuItem &operator=(const QStyleOptionMenuItem &) = default;

protected:
    QStyleOptionMenuItem(int version);
};

class QStyleOptionDockWidget : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_DockWidget };
    enum StyleOptionVersion { Version = 2 };

    QString title;
    bool closable;
    bool movable;
    bool floatable;
    bool verticalTitleBar;

    QStyleOptionDockWidget();
    QStyleOptionDockWidget(const QStyleOptionDockWidget &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionDockWidget &operator=(const QStyleOptionDockWidget &) = default;

protected:
    QStyleOptionDockWidget(int version);
};

Q_DECL_DEPRECATED typedef QStyleOptionDockWidget QStyleOptionDockWidgetV2;

class QStyleOptionViewItem : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_ViewItem };
    enum StyleOptionVersion { Version = 4 };

    enum Position { Left, Right, Top, Bottom };
    enum ScrollMode { ScrollPerItem, ScrollPerPixel }; // Doesn't really belong in this class.

    Qt::Alignment displayAlignment;
    Qt::Alignment decorationAlignment;
    Qt::TextElideMode textElideMode;
    Position decorationPosition;
    QSize decorationSize;
    QFont font;
    bool showDecorationSelected;

    enum ViewItemFeature {
        None = 0x00,
        WrapText = 0x01,
        Alternate = 0x02,
        HasCheckIndicator = 0x04,
        HasDisplay = 0x08,
        HasDecoration = 0x10
    };
    Q_DECLARE_FLAGS(ViewItemFeatures, ViewItemFeature)

    ViewItemFeatures features;

    QLocale locale;

    enum ViewItemPosition { Invalid, Beginning, Middle, End, OnlyOne };

    QModelIndex index;
    Qt::CheckState checkState;
    QIcon icon;
    QString text;
    ViewItemPosition viewItemPosition;
    QBrush backgroundBrush;

    QStyleOptionViewItem();
    QStyleOptionViewItem(const QStyleOptionViewItem &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionViewItem &operator=(const QStyleOptionViewItem &) = default;

protected:
    QStyleOptionViewItem(int version);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QStyleOptionViewItem::ViewItemFeatures)

Q_DECL_DEPRECATED typedef QStyleOptionViewItem QStyleOptionViewItemV2;
Q_DECL_DEPRECATED typedef QStyleOptionViewItem QStyleOptionViewItemV3;
Q_DECL_DEPRECATED typedef QStyleOptionViewItem QStyleOptionViewItemV4;

class QStyleOptionToolBox : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_ToolBox };
    enum StyleOptionVersion { Version = 2 };

    QString text;
    QIcon icon;

    enum TabPosition { Beginning, Middle, End, OnlyOneTab };
    enum SelectedPosition { NotAdjacent, NextIsSelected, PreviousIsSelected };

    TabPosition position;
    SelectedPosition selectedPosition;

    QStyleOptionToolBox();
    QStyleOptionToolBox(const QStyleOptionToolBox &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionToolBox &operator=(const QStyleOptionToolBox &) = default;

protected:
    QStyleOptionToolBox(int version);
};

Q_DECL_DEPRECATED typedef QStyleOptionToolBox QStyleOptionToolBoxV2;

class QStyleOptionRubberBand : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_RubberBand };
    enum StyleOptionVersion { Version = 1 };
    enum Shape { Line, Rectangle };

    bool opaque;
    Shape shape;

    QStyleOptionRubberBand();
    QStyleOptionRubberBand(const QStyleOptionRubberBand &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionRubberBand &operator=(const QStyleOptionRubberBand &) = default;

protected:
    QStyleOptionRubberBand(int version);
};

// -------------------------- Complex style options -------------------------------
class QStyleOptionComplex : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_Complex };
    enum StyleOptionVersion { Version = 1 };

    QStyle::SubControls subControls;
    QStyle::SubControls activeSubControls;

    QStyleOptionComplex(int version = QStyleOptionComplex::Version, int type = SO_Complex);
    QStyleOptionComplex(const QStyleOptionComplex &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionComplex &operator=(const QStyleOptionComplex &) = default;
};

class QStyleOptionSlider : public QStyleOptionComplex
{
public:
    enum StyleOptionType { Type = SO_Slider };
    enum StyleOptionVersion { Version = 1 };
    enum TickPosition {
        NoTicks = 0,
        TicksAbove = 1,
        TicksLeft = TicksAbove,
        TicksBelow = 2,
        TicksRight = TicksBelow,
        TicksBothSides = 3
    };

    Qt::Orientation orientation;
    int minimum;
    int maximum;
    TickPosition tickPosition;
    int tickInterval;
    bool upsideDown;
    int sliderPosition;
    int sliderValue;
    int singleStep;
    int pageStep;
    qreal notchTarget;
    bool dialWrapping;
    qreal startAngle;
    qreal endAngle;

    QStyleOptionSlider();
    QStyleOptionSlider(const QStyleOptionSlider &other) : QStyleOptionComplex(Version, Type) { *this = other; }
    QStyleOptionSlider &operator=(const QStyleOptionSlider &) = default;

protected:
    QStyleOptionSlider(int version);
};

class QStyleOptionSpinBox : public QStyleOptionComplex
{
public:
    enum StyleOptionType { Type = SO_SpinBox };
    enum StyleOptionVersion { Version = 1 };
    enum StepEnabledFlag { StepNone = 0x00, StepUpEnabled = 0x01, StepDownEnabled = 0x02, StepEnabled = 0xFF };
    enum ButtonSymbols { UpDownArrows, PlusMinus, NoButtons };

    ButtonSymbols buttonSymbols;
    StepEnabledFlag stepEnabled;
    bool frame;

    QStyleOptionSpinBox();
    QStyleOptionSpinBox(const QStyleOptionSpinBox &other) : QStyleOptionComplex(Version, Type) { *this = other; }
    QStyleOptionSpinBox &operator=(const QStyleOptionSpinBox &) = default;

protected:
    QStyleOptionSpinBox(int version);
};

class QStyleOptionToolButton : public QStyleOptionComplex
{
public:
    enum StyleOptionType { Type = SO_ToolButton };
    enum StyleOptionVersion { Version = 1 };

    enum ToolButtonFeature { None = 0x00, Arrow = 0x01, Menu = 0x04, MenuButtonPopup = Menu, PopupDelay = 0x08,
                             HasMenu = 0x10 };
    Q_DECLARE_FLAGS(ToolButtonFeatures, ToolButtonFeature)

    ToolButtonFeatures features;
    QIcon icon;
    QSize iconSize;
    QString text;
    Qt::ArrowType arrowType;
    Qt::ToolButtonStyle toolButtonStyle;
    QPoint pos;
    QFont font;

    QStyleOptionToolButton();
    QStyleOptionToolButton(const QStyleOptionToolButton &other) : QStyleOptionComplex(Version, Type) { *this = other; }
    QStyleOptionToolButton &operator=(const QStyleOptionToolButton &) = default;

protected:
    QStyleOptionToolButton(int version);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QStyleOptionToolButton::ToolButtonFeatures)

class QStyleOptionComboBox : public QStyleOptionComplex
{
public:
    enum StyleOptionType { Type = SO_ComboBox };
    enum StyleOptionVersion { Version = 1 };

    bool editable;
    QRect popupRect;
    bool frame;
    QString currentText;
    QIcon currentIcon;
    QSize iconSize;

    QStyleOptionComboBox();
    QStyleOptionComboBox(const QStyleOptionComboBox &other) : QStyleOptionComplex(Version, Type) { *this = other; }
    QStyleOptionComboBox &operator=(const QStyleOptionComboBox &) = default;

protected:
    QStyleOptionComboBox(int version);
};

class QStyleOptionTitleBar : public QStyleOptionComplex
{
public:
    enum StyleOptionType { Type = SO_TitleBar };
    enum StyleOptionVersion { Version = 1 };

    QString text;
    QIcon icon;
    int titleBarState;
    Qt::WindowFlags titleBarFlags;

    QStyleOptionTitleBar();
    QStyleOptionTitleBar(const QStyleOptionTitleBar &other) : QStyleOptionComplex(Version, Type) { *this = other; }
    QStyleOptionTitleBar &operator=(const QStyleOptionTitleBar &) = default;

protected:
    QStyleOptionTitleBar(int version);
};

class QStyleOptionGroupBox : public QStyleOptionComplex
{
public:
    enum StyleOptionType { Type = SO_GroupBox };
    enum StyleOptionVersion { Version = 1 };

    QStyleOptionFrame::FrameFeatures features;
    QString text;
    Qt::Alignment textAlignment;
    QColor textColor;
    int lineWidth;
    int midLineWidth;

    QStyleOptionGroupBox();
    QStyleOptionGroupBox(const QStyleOptionGroupBox &other) : QStyleOptionComplex(Version, Type) { *this = other; }
    QStyleOptionGroupBox &operator=(const QStyleOptionGroupBox &) = default;
protected:
    QStyleOptionGroupBox(int version);
};

class QStyleOptionSizeGrip : public QStyleOptionComplex
{
public:
    enum StyleOptionType { Type = SO_SizeGrip };
    enum StyleOptionVersion { Version = 1 };

    Qt::Corner corner;

    QStyleOptionSizeGrip();
    QStyleOptionSizeGrip(const QStyleOptionSizeGrip &other) : QStyleOptionComplex(Version, Type) { *this = other; }
    QStyleOptionSizeGrip &operator=(const QStyleOptionSizeGrip &) = default;
protected:
    QStyleOptionSizeGrip(int version);
};

class QStyleOptionGraphicsItem : public QStyleOption
{
public:
    enum StyleOptionType { Type = SO_GraphicsItem };
    enum StyleOptionVersion { Version = 1 };

    QRectF exposedRect;
    qreal levelOfDetail;

    QStyleOptionGraphicsItem();
    QStyleOptionGraphicsItem(const QStyleOptionGraphicsItem &other) : QStyleOption(Version, Type) { *this = other; }
    QStyleOptionGraphicsItem &operator=(const QStyleOptionGraphicsItem &) = default;
    static qreal levelOfDetailFromTransform(const QTransform &worldTransform);
protected:
    QStyleOptionGraphicsItem(int version);
};

template <typename T>
T qstyleoption_cast(const QStyleOption *opt)
{
    typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type Opt;
    if (opt && opt->version >= Opt::Version && (opt->type == Opt::Type
        || int(Opt::Type) == QStyleOption::SO_Default
        || (int(Opt::Type) == QStyleOption::SO_Complex
            && opt->type > QStyleOption::SO_Complex)))
        return static_cast<T>(opt);
    return nullptr;
}

template <typename T>
T qstyleoption_cast(QStyleOption *opt)
{
    typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type Opt;
    if (opt && opt->version >= Opt::Version && (opt->type == Opt::Type
        || int(Opt::Type) == QStyleOption::SO_Default
        || (int(Opt::Type) == QStyleOption::SO_Complex
            && opt->type > QStyleOption::SO_Complex)))
        return static_cast<T>(opt);
    return nullptr;
}

// -------------------------- QStyleHintReturn -------------------------------

class QStyleHintReturn
{
public:
    enum HintReturnType {
        SH_Default=0xf000, SH_Mask, SH_Variant
    };

    enum StyleOptionType { Type = SH_Default };
    enum StyleOptionVersion { Version = 1 };

    QStyleHintReturn(int version = QStyleOption::Version, int type = SH_Default);
    ~QStyleHintReturn();

    int version;
    int type;
};

class QStyleHintReturnMask : public QStyleHintReturn
{
public:
    enum StyleOptionType { Type = SH_Mask };
    enum StyleOptionVersion { Version = 1 };

    QStyleHintReturnMask();
    ~QStyleHintReturnMask();

    QRegion region;
};

class QStyleHintReturnVariant : public QStyleHintReturn
{
public:
    enum StyleOptionType { Type = SH_Variant };
    enum StyleOptionVersion { Version = 1 };

    QStyleHintReturnVariant();
    ~QStyleHintReturnVariant();

    QVariant variant;
};

template <typename T>
T qstyleoption_cast(const QStyleHintReturn *hint)
{
    typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type Opt;
    if (hint && hint->version <= Opt::Version &&
        (hint->type == Opt::Type || int(Opt::Type) == QStyleHintReturn::SH_Default))
        return static_cast<T>(hint);
    return nullptr;
}

template <typename T>
T qstyleoption_cast(QStyleHintReturn *hint)
{
    typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type Opt;
    if (hint && hint->version <= Opt::Version &&
        (hint->type == Opt::Type || int(Opt::Type) == QStyleHintReturn::SH_Default))
        return static_cast<T>(hint);
    return nullptr;
}

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug debug, const QStyleOption::OptionType &optionType);
QDebug operator<<(QDebug debug, const QStyleOption &option);
#endif

} // namespace QQC2

QT_END_NAMESPACE

#endif // QSTYLEOPTION_H
