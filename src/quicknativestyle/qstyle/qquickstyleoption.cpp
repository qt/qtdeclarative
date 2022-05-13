// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleoption.h"

#include <QtGui/private/qguiapplication_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

namespace QQC2 {

QStyleOption::QStyleOption(int version, int type)
    : version(version), type(type), state(QStyle::State_None),
      direction(QGuiApplication::layoutDirection()), fontMetrics(QFont()),
      styleObject(nullptr), control(nullptr), window(nullptr)
{
}

/*!
    Destroys this style option object.
*/
QStyleOption::~QStyleOption()
{
}

/*!
   Constructs a copy of \a other.
*/
QStyleOption::QStyleOption(const QStyleOption &other)
    : version(Version), type(Type), state(other.state),
      direction(other.direction), rect(other.rect), fontMetrics(other.fontMetrics),
      palette(other.palette), styleObject(other.styleObject),
      control(other.control), window(other.window)
{
}

/*!
    Assign \a other to this QStyleOption.
*/
QStyleOption &QStyleOption::operator=(const QStyleOption &other)
{
    control = other.control;
    window = other.window;
    state = other.state;
    direction = other.direction;
    rect = other.rect;
    fontMetrics = other.fontMetrics;
    palette = other.palette;
    styleObject = other.styleObject;
    return *this;
}

/*!
    Constructs a QStyleOptionFocusRect, initializing the members
    variables to their default values.
*/
QStyleOptionFocusRect::QStyleOptionFocusRect()
    : QStyleOption(Version, SO_FocusRect)
{
    state |= QStyle::State_KeyboardFocusChange; // assume we had one, will be corrected in initFrom()
}

/*!
    \internal
*/
QStyleOptionFocusRect::QStyleOptionFocusRect(int versionIn)
    : QStyleOption(versionIn, SO_FocusRect)
{
    state |= QStyle::State_KeyboardFocusChange;  // assume we had one, will be corrected in initFrom()
}

/*!
    Constructs a QStyleOptionFrame, initializing the members
    variables to their default values.
*/
QStyleOptionFrame::QStyleOptionFrame()
    : QStyleOption(Version, SO_Frame), lineWidth(0), midLineWidth(0),
      features(None), frameShape(NoFrame)
{
}

/*!
    \internal
*/
QStyleOptionFrame::QStyleOptionFrame(int versionIn)
    : QStyleOption(versionIn, SO_Frame), lineWidth(0), midLineWidth(0),
      features(None)
{
}

/*!
    Constructs a QStyleOptionGroupBox, initializing the members
    variables to their default values.
*/
QStyleOptionGroupBox::QStyleOptionGroupBox()
    : QStyleOptionComplex(Version, Type), features(QStyleOptionFrame::None),
      textAlignment(Qt::AlignLeft), lineWidth(0), midLineWidth(0)
{
}

QStyleOptionGroupBox::QStyleOptionGroupBox(int versionIn)
    : QStyleOptionComplex(versionIn, Type), features(QStyleOptionFrame::None),
      textAlignment(Qt::AlignLeft), lineWidth(0), midLineWidth(0)
{
}

/*!
    Constructs a QStyleOptionHeader, initializing the members
    variables to their default values.
*/
QStyleOptionHeader::QStyleOptionHeader()
    : QStyleOption(QStyleOptionHeader::Version, SO_Header),
      section(0), textAlignment(Qt::AlignLeft), iconAlignment(Qt::AlignLeft),
      position(QStyleOptionHeader::Beginning),
      selectedPosition(QStyleOptionHeader::NotAdjacent), sortIndicator(None),
      orientation(Qt::Horizontal)
{
}

/*!
    \internal
*/
QStyleOptionHeader::QStyleOptionHeader(int versionIn)
    : QStyleOption(versionIn, SO_Header),
      section(0), textAlignment(Qt::AlignLeft), iconAlignment(Qt::AlignLeft),
      position(QStyleOptionHeader::Beginning),
      selectedPosition(QStyleOptionHeader::NotAdjacent), sortIndicator(None),
      orientation(Qt::Horizontal)
{
}

/*!
    Constructs a QStyleOptionButton, initializing the members
    variables to their default values.
*/
QStyleOptionButton::QStyleOptionButton()
    : QStyleOption(QStyleOptionButton::Version, SO_Button), features(None)
{
}

/*!
    \internal
*/
QStyleOptionButton::QStyleOptionButton(int versionIn)
    : QStyleOption(versionIn, SO_Button), features(None)
{
}

/*!
    Constructs a QStyleOptionToolBar, initializing the members
    variables to their default values.
*/
QStyleOptionToolBar::QStyleOptionToolBar()
    : QStyleOption(Version, SO_ToolBar), positionOfLine(OnlyOne), positionWithinLine(OnlyOne),
      toolBarArea(Qt::TopToolBarArea), features(None), lineWidth(0), midLineWidth(0)
{
}

/*!
    \fn QStyleOptionToolBar::QStyleOptionToolBar(const QStyleOptionToolBar &other)

    Constructs a copy of the \a other style option.
*/
QStyleOptionToolBar::QStyleOptionToolBar(int versionIn)
: QStyleOption(versionIn, SO_ToolBar), positionOfLine(OnlyOne), positionWithinLine(OnlyOne),
  toolBarArea(Qt::TopToolBarArea), features(None), lineWidth(0), midLineWidth(0)
{

}

/*!
    Constructs a QStyleOptionTab object, initializing the members
    variables to their default values.
*/
QStyleOptionTab::QStyleOptionTab()
    : QStyleOption(QStyleOptionTab::Version, SO_Tab),
      row(0),
      position(Beginning),
      selectedPosition(NotAdjacent), cornerWidgets(QStyleOptionTab::NoCornerWidgets),
      documentMode(false),
      features(QStyleOptionTab::None)
{
}

QStyleOptionTab::QStyleOptionTab(int versionIn)
    : QStyleOption(versionIn, SO_Tab),
      row(0),
      position(Beginning),
      selectedPosition(NotAdjacent), cornerWidgets(QStyleOptionTab::NoCornerWidgets),
      documentMode(false),
      features(QStyleOptionTab::None)
{
}

/*!
    Constructs a QStyleOptionTabV4 object, initializing the members
    variables to their default values.
 */
QStyleOptionTabV4::QStyleOptionTabV4() : QStyleOptionTab(QStyleOptionTabV4::Version)
{
}

/*!
    Constructs a QStyleOptionProgressBar, initializing the members
    variables to their default values.
*/
QStyleOptionProgressBar::QStyleOptionProgressBar()
    : QStyleOption(QStyleOptionProgressBar::Version, SO_ProgressBar),
      minimum(0), maximum(0), progress(0), textAlignment(Qt::AlignLeft), textVisible(false),
      invertedAppearance(false), bottomToTop(false)
{
}

QStyleOptionProgressBar::QStyleOptionProgressBar(int versionIn)
    : QStyleOption(versionIn, SO_ProgressBar),
      minimum(0), maximum(0), progress(0), textAlignment(Qt::AlignLeft), textVisible(false),
      invertedAppearance(false), bottomToTop(false)
{
}

/*!
    Constructs a QStyleOptionMenuItem, initializing the members
    variables to their default values.
*/
QStyleOptionMenuItem::QStyleOptionMenuItem()
    : QStyleOption(QStyleOptionMenuItem::Version, SO_MenuItem), menuItemType(Normal),
      checkType(NotCheckable), checked(false), menuHasCheckableItems(true), maxIconWidth(0), tabWidth(0)
{
}

/*!
    \internal
*/
QStyleOptionMenuItem::QStyleOptionMenuItem(int versionIn)
    : QStyleOption(versionIn, SO_MenuItem), menuItemType(Normal),
      checkType(NotCheckable), checked(false), menuHasCheckableItems(true), maxIconWidth(0), tabWidth(0)
{
}

/*!
    Constructs a QStyleOptionComplex of the specified \a type and \a
    version, initializing the member variables to their default
    values. This constructor is usually called by subclasses.
*/
QStyleOptionComplex::QStyleOptionComplex(int versionIn, int typeIn)
    : QStyleOption(versionIn, typeIn), subControls(QStyle::SC_All), activeSubControls(QStyle::SC_None)
{
}


/*!
    Constructs a QStyleOptionSlider, initializing the members
    variables to their default values.
*/
QStyleOptionSlider::QStyleOptionSlider()
    : QStyleOptionComplex(Version, SO_Slider), orientation(Qt::Horizontal), minimum(0), maximum(0),
      tickPosition(NoTicks), tickInterval(0), upsideDown(false),
      sliderPosition(0), sliderValue(0), singleStep(0), pageStep(0), notchTarget(0.0),
      dialWrapping(false)
{
}

/*!
    \internal
*/
QStyleOptionSlider::QStyleOptionSlider(int versionIn)
    : QStyleOptionComplex(versionIn, SO_Slider), orientation(Qt::Horizontal), minimum(0), maximum(0),
      tickPosition(NoTicks), tickInterval(0), upsideDown(false),
      sliderPosition(0), sliderValue(0), singleStep(0), pageStep(0), notchTarget(0.0),
      dialWrapping(false)
{
}

/*!
    Constructs a QStyleOptionSpinBox, initializing the members
    variables to their default values.
*/
QStyleOptionSpinBox::QStyleOptionSpinBox()
    : QStyleOptionComplex(Version, SO_SpinBox), buttonSymbols(UpDownArrows),
      stepEnabled(StepNone), frame(false)
{
}

/*!
    \internal
*/
QStyleOptionSpinBox::QStyleOptionSpinBox(int versionIn)
    : QStyleOptionComplex(versionIn, SO_SpinBox), buttonSymbols(UpDownArrows),
      stepEnabled(StepNone), frame(false)
{
}

/*!
    Constructs a QStyleOptionDockWidget, initializing the member
    variables to their default values.
*/
QStyleOptionDockWidget::QStyleOptionDockWidget()
    : QStyleOption(Version, SO_DockWidget), closable(false),
      movable(false), floatable(false), verticalTitleBar(false)
{
}

/*!
    \internal
*/
QStyleOptionDockWidget::QStyleOptionDockWidget(int versionIn)
    : QStyleOption(versionIn, SO_DockWidget), closable(false),
      movable(false), floatable(false), verticalTitleBar(false)
{
}

/*!
    Constructs a QStyleOptionToolButton, initializing the members
    variables to their default values.
*/
QStyleOptionToolButton::QStyleOptionToolButton()
    : QStyleOptionComplex(Version, SO_ToolButton), features(None), arrowType(Qt::DownArrow)
    , toolButtonStyle(Qt::ToolButtonIconOnly)
{
}

QStyleOptionToolButton::QStyleOptionToolButton(int versionIn)
    : QStyleOptionComplex(versionIn, SO_ToolButton), features(None), arrowType(Qt::DownArrow)
    , toolButtonStyle(Qt::ToolButtonIconOnly)

{
}

/*!
    Creates a QStyleOptionComboBox, initializing the members variables
    to their default values.
*/
QStyleOptionComboBox::QStyleOptionComboBox()
    : QStyleOptionComplex(Version, SO_ComboBox), editable(false), frame(true)
{
}

QStyleOptionComboBox::QStyleOptionComboBox(int versionIn)
    : QStyleOptionComplex(versionIn, SO_ComboBox), editable(false), frame(true)
{
}

/*!
    Creates a QStyleOptionToolBox, initializing the members variables
    to their default values.
*/
QStyleOptionToolBox::QStyleOptionToolBox()
    : QStyleOption(Version, SO_ToolBox), position(Beginning), selectedPosition(NotAdjacent)
{
}

QStyleOptionToolBox::QStyleOptionToolBox(int versionIn)
    : QStyleOption(versionIn, SO_ToolBox), position(Beginning), selectedPosition(NotAdjacent)
{
}


/*!
    Creates a QStyleOptionRubberBand, initializing the members
    variables to their default values.
*/
QStyleOptionRubberBand::QStyleOptionRubberBand()
    : QStyleOption(Version, SO_RubberBand), opaque(false), shape(Line)
{
}

QStyleOptionRubberBand::QStyleOptionRubberBand(int versionIn)
    : QStyleOption(versionIn, SO_RubberBand), opaque(false)
{
}

/*!
    Constructs a QStyleOptionTitleBar, initializing the members
    variables to their default values.
*/
QStyleOptionTitleBar::QStyleOptionTitleBar()
    : QStyleOptionComplex(Version, SO_TitleBar), titleBarState(0)
{
}

QStyleOptionTitleBar::QStyleOptionTitleBar(int versionIn)
    : QStyleOptionComplex(versionIn, SO_TitleBar), titleBarState(0)
{
}

/*!
    Constructs a QStyleOptionViewItem, initializing the members
    variables to their default values.
*/
QStyleOptionViewItem::QStyleOptionViewItem()
    : QStyleOption(Version, SO_ViewItem),
      displayAlignment(Qt::AlignLeft), decorationAlignment(Qt::AlignLeft),
      textElideMode(Qt::ElideMiddle), decorationPosition(Left),
      showDecorationSelected(false), features(None),
      checkState(Qt::Unchecked), viewItemPosition(QStyleOptionViewItem::Invalid)
{
}

QStyleOptionViewItem::QStyleOptionViewItem(int versionIn)
    : QStyleOption(versionIn, SO_ViewItem),
      displayAlignment(Qt::AlignLeft), decorationAlignment(Qt::AlignLeft),
      textElideMode(Qt::ElideMiddle), decorationPosition(Left),
      showDecorationSelected(false), features(None),
      checkState(Qt::Unchecked), viewItemPosition(QStyleOptionViewItem::Invalid)
{
}

/*!
    Constructs a QStyleOptionTabWidgetFrame, initializing the members
    variables to their default values.
*/
QStyleOptionTabWidgetFrame::QStyleOptionTabWidgetFrame()
    : QStyleOption(Version, SO_TabWidgetFrame), lineWidth(0), midLineWidth(0), shape(QStyleOptionTab::RoundedNorth)
{
}

QStyleOptionTabWidgetFrame::QStyleOptionTabWidgetFrame(int versionIn)
    : QStyleOption(versionIn, SO_TabWidgetFrame), lineWidth(0), midLineWidth(0)
{
}

/*!
    Construct a QStyleOptionTabBarBase, initializing the members
    vaiables to their default values.
*/
QStyleOptionTabBarBase::QStyleOptionTabBarBase()
    : QStyleOption(Version, SO_TabBarBase), documentMode(false), shape(QStyleOptionTab::RoundedNorth)
{
}

QStyleOptionTabBarBase::QStyleOptionTabBarBase(int versionIn)
    : QStyleOption(versionIn, SO_TabBarBase), documentMode(false)
{
}

/*!
    Constructs a QStyleOptionSizeGrip.
*/
QStyleOptionSizeGrip::QStyleOptionSizeGrip()
    : QStyleOptionComplex(Version, Type), corner(Qt::BottomRightCorner)
{
}

QStyleOptionSizeGrip::QStyleOptionSizeGrip(int versionIn)
    : QStyleOptionComplex(versionIn, Type), corner(Qt::BottomRightCorner)
{
}

/*!
    Constructs a QStyleOptionGraphicsItem.
*/
QStyleOptionGraphicsItem::QStyleOptionGraphicsItem()
    : QStyleOption(Version, Type), levelOfDetail(1)
{
}

QStyleOptionGraphicsItem::QStyleOptionGraphicsItem(int versionIn)
    : QStyleOption(versionIn, Type), levelOfDetail(1)
{
}

/*!
    \since 4.6

    Returns the level of detail from the \a worldTransform.

    Its value represents the maximum value of the height and
    width of a unity rectangle, mapped using the \a worldTransform
    of the painter used to draw the item. By default, if no
    transformations are applied, its value is 1. If zoomed out 1:2, the level
    of detail will be 0.5, and if zoomed in 2:1, its value is 2.

    \sa QGraphicsScene::minimumRenderSize()
*/
qreal QStyleOptionGraphicsItem::levelOfDetailFromTransform(const QTransform &worldTransform)
{
    if (worldTransform.type() <= QTransform::TxTranslate)
        return 1; // Translation only? The LOD is 1.

    // Two unit vectors.
    QLineF v1(0, 0, 1, 0);
    QLineF v2(0, 0, 0, 1);
    // LOD is the transformed area of a 1x1 rectangle.
    return qSqrt(worldTransform.map(v1).length() * worldTransform.map(v2).length());
}

/*!
    Constructs a QStyleHintReturn with version \a version and type \a
    type.

    The version has no special meaning for QStyleHintReturn; it can be
    used by subclasses to distinguish between different version of
    the same hint type.

    \sa QStyleOption::version, QStyleOption::type
*/
QStyleHintReturn::QStyleHintReturn(int versionIn, int type)
    : version(versionIn), type(type)
{
}

/*!
    \internal
*/

QStyleHintReturn::~QStyleHintReturn()
{
}

/*!
    Constructs a QStyleHintReturnMask. The member variables are
    initialized to default values.
*/
QStyleHintReturnMask::QStyleHintReturnMask() : QStyleHintReturn(Version, Type)
{
}

QStyleHintReturnMask::~QStyleHintReturnMask()
{
}

/*!
    Constructs a QStyleHintReturnVariant. The member variables are
    initialized to default values.
*/
QStyleHintReturnVariant::QStyleHintReturnVariant() : QStyleHintReturn(Version, Type)
{
}

QStyleHintReturnVariant::~QStyleHintReturnVariant()
{
}

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug debug, const QStyleOption::OptionType &optionType)
{
#if !defined(QT_NO_DEBUG)
    switch (optionType) {
    case QStyleOption::SO_Default:
        debug << "SO_Default"; break;
    case QStyleOption::SO_FocusRect:
        debug << "SO_FocusRect"; break;
    case QStyleOption::SO_Button:
        debug << "SO_Button"; break;
    case QStyleOption::SO_Tab:
        debug << "SO_Tab"; break;
    case QStyleOption::SO_MenuItem:
        debug << "SO_MenuItem"; break;
    case QStyleOption::SO_Frame:
        debug << "SO_Frame"; break;
    case QStyleOption::SO_ProgressBar:
        debug << "SO_ProgressBar"; break;
    case QStyleOption::SO_ToolBox:
        debug << "SO_ToolBox"; break;
    case QStyleOption::SO_Header:
        debug << "SO_Header"; break;
    case QStyleOption::SO_DockWidget:
        debug << "SO_DockWidget"; break;
    case QStyleOption::SO_ViewItem:
        debug << "SO_ViewItem"; break;
    case QStyleOption::SO_TabWidgetFrame:
        debug << "SO_TabWidgetFrame"; break;
    case QStyleOption::SO_TabBarBase:
        debug << "SO_TabBarBase"; break;
    case QStyleOption::SO_RubberBand:
        debug << "SO_RubberBand"; break;
    case QStyleOption::SO_Complex:
        debug << "SO_Complex"; break;
    case QStyleOption::SO_Slider:
        debug << "SO_Slider"; break;
    case QStyleOption::SO_SpinBox:
        debug << "SO_SpinBox"; break;
    case QStyleOption::SO_ToolButton:
        debug << "SO_ToolButton"; break;
    case QStyleOption::SO_ComboBox:
        debug << "SO_ComboBox"; break;
    case QStyleOption::SO_TitleBar:
        debug << "SO_TitleBar"; break;
    case QStyleOption::SO_CustomBase:
        debug << "SO_CustomBase"; break;
    case QStyleOption::SO_GroupBox:
        debug << "SO_GroupBox"; break;
    case QStyleOption::SO_ToolBar:
        debug << "SO_ToolBar"; break;
    case QStyleOption::SO_ComplexCustomBase:
        debug << "SO_ComplexCustomBase"; break;
    case QStyleOption::SO_SizeGrip:
        debug << "SO_SizeGrip"; break;
    case QStyleOption::SO_GraphicsItem:
        debug << "SO_GraphicsItem"; break;
    }
#else
    Q_UNUSED(optionType);
#endif
    return debug;
}

QDebug operator<<(QDebug debug, const QStyleOption &option)
{
#if !defined(QT_NO_DEBUG)
    debug << "QStyleOption(";
    debug << QStyleOption::OptionType(option.type);
    debug << ',' << (option.direction == Qt::RightToLeft ? "RightToLeft" : "LeftToRight");
    debug << ',' << option.state;
    debug << ',' << option.rect;
    debug << ',' << option.styleObject;
    debug << ')';
#else
    Q_UNUSED(option);
#endif
    return debug;
}
#endif

} // namespace QQC2

QT_END_NAMESPACE
