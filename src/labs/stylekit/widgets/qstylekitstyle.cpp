// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qstylekitstyle.h"
#include "qstylekitstyle_p.h"
#include <QtWidgets/qstyleoption.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qapplication.h>
#if QT_CONFIG(scrollarea)
#  include <QtWidgets/qabstractscrollarea.h>
#endif
#if QT_CONFIG(itemviews)
#  include <QtWidgets/qabstractitemview.h>
#endif
#if QT_CONFIG(lineedit)
#  include <QtWidgets/qlineedit.h>
#endif
#if QT_CONFIG(pushbutton)
#  include <QtWidgets/qpushbutton.h>
#endif
#if QT_CONFIG(checkbox)
#  include <QtWidgets/qcheckbox.h>
#endif
#if QT_CONFIG(radiobutton)
#  include <QtWidgets/qradiobutton.h>
#endif
#if QT_CONFIG(combobox)
#  include <QtWidgets/qcombobox.h>
#endif
#if QT_CONFIG(slider)
#  include <QtWidgets/qslider.h>
#endif
#if QT_CONFIG(scrollbar)
#  include <QtWidgets/qscrollbar.h>
#endif
#if QT_CONFIG(spinbox)
#  include <QtWidgets/qspinbox.h>
#endif
#if QT_CONFIG(progressbar)
#  include <QtWidgets/qprogressbar.h>
#endif
#if QT_CONFIG(textedit)
#  include <QtWidgets/qtextedit.h>
#  include <QtWidgets/qplaintextedit.h>
#endif
#if QT_CONFIG(tabbar)
#  include <QtWidgets/qtabbar.h>
#endif
#if QT_CONFIG(tabwidget)
#  include <QtWidgets/qtabwidget.h>
#endif
#if QT_CONFIG(toolbar)
#  include <QtWidgets/qtoolbar.h>
#endif
#if QT_CONFIG(toolbutton)
#  include <QtWidgets/qtoolbutton.h>
#endif
#if QT_CONFIG(groupbox)
#  include <QtWidgets/qgroupbox.h>
#endif
#if QT_CONFIG(menu)
#  include <QtWidgets/qmenu.h>
#  include <QtWidgets/qmenubar.h>
#endif
#if QT_CONFIG(label)
#  include <QtWidgets/qlabel.h>
#endif
#include <QtWidgets/private/qwidget_p.h>
#include <QtCore/private/qobject_p.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/qpainterstateguard.h>
#include <QtGui/qstylehints.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qnumeric.h>
#include <QtQml/private/qqmlcomponent_p.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <QtLabsStyleKit/private/qqstylekit_p.h>
#include <QtLabsStyleKit/private/qqstylekitcontrolproperties_p.h>
#include <QtLabsStyleKit/private/qqstylekitstyle_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcStyleKit, "qt.labs.stylekit")

/*!
    \class QStyleKitStyle
    \inmodule QtLabsStyleKit
    \ingroup appearance
    \since 6.12

    \brief The QStyleKitStyle class applies a \l {Qt Labs StyleKit} style
    to Qt Widgets.

    QStyleKitStyle is a QStyle implementation that uses a \l StyleKit \l Style
    to style Qt Widgets. The \l Style is a QML file that declaratively describes
    the visual design (colors, sizes, radii, borders, and other properties)
    for each widget type and state. Those property values drive the painting,
    which is done entirely with QPainter. Qt Quick and the scene graph play
    no part in the rendering.

    This separation means the same \l Style QML file can drive both
    Qt Quick Controls and Qt Widgets, sharing one design definition across
    both systems.

    \note StyleKit is a Qt Labs module, and its API may change between
    Qt releases.

    \section1 Loading a Style

    A style is a QML file whose root object is a \l Style. To load it,
    pass the file path to the constructor or to \l setStylePath():

    \if !defined(qtforpython)
    \code
    auto *style = new QStyleKitStyle(QStringLiteral(":/styles/MyStyle.qml"));
    QApplication::setStyle(style);
    \endcode
    \else
    \code
    style = QStyleKitStyle(":/styles/MyStyle.qml")
    QApplication.setStyle(style)
    \endcode
    \endif

    The Style is loaded with an internal QQmlEngine owned by the
    QStyleKitStyle instance. If the path is invalid or the root object is
    not a \l Style, a warning is emitted and the style uses a default
    fallback style until a valid \l stylePath is set.

    \section1 Themes

    A Style may define one or more named \l {Theme}{themes}. The active
    theme is selected with \l setThemeName(); the list of available
    themes is exposed through \l availableThemeNames. The special theme name
    \c System makes the style follow the platform color scheme: when the
    OS color scheme changes, the active theme is recreated automatically
    and all widgets are repolished.

    \section1 Widget to StyleKit Control Mapping

    Each Qt Widgets class is mapped to a StyleKit control type, which
    determines which control entry in the \l Style applies to it. Use the
    corresponding control entry to configure visual properties for that widget
    type, including individual parts of the widget such as its background,
    indicator, handle, etc. See \l ControlStyleProperties for the full list of
    stylable properties.
    Properties not set in a specific control entry fall back through the control
    type hierarchy: for example, \c button falls back to \c abstractButton, which
    falls back to \c control.

    \table
    \header
        \li Qt Widgets class
        \li StyleKit control
    \row
        \li QPushButton (flat)
        \li \l {AbstractStylableControls::flatButton}{flatButton}
    \row
        \li QPushButton
        \li \l {AbstractStylableControls::button}{button}
    \row
        \li QCheckBox
        \li \l {AbstractStylableControls::checkBox}{checkBox}
    \row
        \li QRadioButton
        \li \l {AbstractStylableControls::radioButton}{radioButton}
    \row
        \li QComboBox
        \li \l {AbstractStylableControls::comboBox}{comboBox}
    \row
        \li QSlider
        \li \l {AbstractStylableControls::slider}{slider}
    \row
        \li QScrollBar
        \li \l {AbstractStylableControls::scrollBar}{scrollBar}
    \row
        \li QSpinBox, QDoubleSpinBox
        \li \l {AbstractStylableControls::spinBox}{spinBox}
    \row
        \li QProgressBar
        \li \l {AbstractStylableControls::progressBar}{progressBar}
    \row
        \li QLineEdit
        \li \l {AbstractStylableControls::textField}{textField}
    \row
        \li QTextEdit, QPlainTextEdit
        \li \l {AbstractStylableControls::textArea}{textArea}
    \row
        \li QTabBar
        \li \l {AbstractStylableControls::tabBar}{tabBar}
    \row
        \li QTabWidget
        \li \l {AbstractStylableControls::page}{page}
    \row
        \li QToolBar
        \li \l {AbstractStylableControls::toolBar}{toolBar}
    \row
        \li QToolButton
        \li \l {AbstractStylableControls::toolButton}{toolButton}
    \row
        \li QGroupBox
        \li \l {AbstractStylableControls::groupBox}{groupBox}
    \row
        \li QFrame
        \li \l {AbstractStylableControls::frame}{frame}
    \row
        \li QLabel
        \li \l {AbstractStylableControls::label}{label}
    \row
        \li QMenu
        \li \l {AbstractStylableControls::menu}{menu}
    \row
        \li QMenuBar
        \li \l {AbstractStylableControls::menuBar}{menuBar}
    \row
        \li Everything else
        \li \l {AbstractStylableControls::control}{control}
    \endtable

    Widgets not listed above are not yet supported by QStyleKitStyle and will be
    painted by \l QCommonStyle. Support for remaining widgets is planned for future
    releases. Conversely, some control entries in \l AbstractStylableControls have no
    Qt Widgets equivalent and are not applied when styling widgets.

    \section2 Sub-controls within a widget

    Separate sub-controls within a widget can be styled individually, as each one maps
    to a separate control entry in the \l Style:

    \table
    \header
        \li Sub-element
        \li StyleKit control
    \row
        \li \l QStyledItemDelegate items - the default delegate for all Qt item views,
            including the \l QComboBox popup list
        \li \l {AbstractStylableControls::itemDelegate}{itemDelegate}
    \row
        \li The same items, when user-checkable (i.e. showing a check indicator)
        \li \l {AbstractStylableControls::checkDelegate}{checkDelegate};
            falls back to \c itemDelegate for anything not set explicitly
    \row
        \li Individual tabs in a \l QTabBar
        \li \l {AbstractStylableControls::tabButton}{tabButton}
    \row
        \li \l QMenu items
        \li \l {AbstractStylableControls::menuItem}{menuItem}
    \row
        \li Separators in a \l QMenu
        \li \l {AbstractStylableControls::menuSeparator}{menuSeparator}
    \row
        \li \l QMenuBar items
        \li \l {AbstractStylableControls::menuBarItem}{menuBarItem}
    \row
        \li Separators in a \l QToolBar
        \li \l {AbstractStylableControls::toolSeparator}{toolSeparator}
    \row
        \li The \l QComboBox popup list container
        \li \l {AbstractStylableControls::popup}{popup}
    \endtable

    \section1 Known Limitations

    QStyleKitStyle is in Tech Preview. The following StyleKit features are
    currently not supported when used with Qt Widgets:

    \list
        \li \b{Shadows} — shadows are not rendered.
        \li \b{Delegate scale above 1.0 on a control's background} — a widget cannot paint
            outside its own rect, so the scaled background is clipped at the widget edge.
            Use \l {DelegateStyle::}{margins} to inset the background and reserve room for
            it to grow. Scaling indicators, handles and foregrounds is unaffected.
        \li \b{Variations} — setting a \l StyleVariation on a widget instance is
            not yet supported.
        \li \b{Custom controls} — styling custom widgets using \l CustomControl
            is not yet supported.
        \li \b{Custom delegates} — the \l {DelegateStyle::}{delegate} property is
            not used; the built-in rendering is always applied.
    \endlist

    Support for these features is planned for a future release.

    \sa QStyle, QCommonStyle, {Qt Labs StyleKit}, Style, Theme
*/

/*!
    \property QStyleKitStyle::stylePath
    \brief the path to the QML \l Style file driving this style.

    The value is a path to a local file or a path to a file in the resource
    file system (for example, \c{:/styles/MyStyle.qml}). A relative path is
    resolved against the application's working directory. The file must
    contain a QML component whose root object is a \l Style. Setting this
    property reloads the style; if the new file cannot be loaded, the
    previously loaded style is kept and a warning is emitted.
*/

/*!
    \property QStyleKitStyle::themeName
    \brief the name of the active theme.

    The value must be one of the entries in \l availableThemeNames, or the
    special name \c System to follow the platform color scheme.
    Setting this property updates all widgets to repaint with the
    new theme.
*/

/*!
    \property QStyleKitStyle::availableThemeNames
    \brief the list of theme names exposed by the loaded \l Style.

    This list includes the built-in \c Light and \c Dark themes as well
    as any custom themes defined by the style.
*/

/*!
    \property QStyleKitStyle::customThemeNames
    \brief the list of custom theme names defined by the loaded \l Style.

    Unlike \l availableThemeNames, this list excludes the built-in \c Light and
    \c Dark themes and contains only the themes explicitly defined by the
    style author. Returns an empty list when no style is loaded.

    \sa availableThemeNames, themeName
*/

static QQStyleKitReader::ControlType controlTypeForWidget(const QWidget *widget)
{
    if (!widget)
        return QQStyleKitReader::Control;

#if QT_CONFIG(pushbutton)
    if (qobject_cast<const QPushButton *>(widget))
        return QQStyleKitReader::Button;
#endif
#if QT_CONFIG(checkbox)
    if (qobject_cast<const QCheckBox *>(widget))
        return QQStyleKitReader::CheckBox;
#endif
#if QT_CONFIG(radiobutton)
    if (qobject_cast<const QRadioButton *>(widget))
        return QQStyleKitReader::RadioButton;
#endif
#if QT_CONFIG(combobox)
    if (qobject_cast<const QComboBox *>(widget))
        return QQStyleKitReader::ComboBox;
#endif
#if QT_CONFIG(slider)
    if (qobject_cast<const QSlider *>(widget))
        return QQStyleKitReader::Slider;
#endif
#if QT_CONFIG(scrollbar)
    if (qobject_cast<const QScrollBar *>(widget))
        return QQStyleKitReader::ScrollBar;
#endif
#if QT_CONFIG(spinbox)
    if (qobject_cast<const QSpinBox *>(widget) || qobject_cast<const QDoubleSpinBox *>(widget))
        return QQStyleKitReader::SpinBox;
#endif
#if QT_CONFIG(itemviews)
    if (qobject_cast<const QAbstractItemView *>(widget))
        return QQStyleKitReader::ItemDelegate;
#endif
#if QT_CONFIG(progressbar)
    if (qobject_cast<const QProgressBar *>(widget))
        return QQStyleKitReader::ProgressBar;
#endif
#if QT_CONFIG(lineedit)
    if (qobject_cast<const QLineEdit *>(widget))
        return QQStyleKitReader::TextField;
#endif
#if QT_CONFIG(textedit)
    if (qobject_cast<const QTextEdit *>(widget) || qobject_cast<const QPlainTextEdit *>(widget))
        return QQStyleKitReader::TextArea;
#endif
#if QT_CONFIG(tabbar)
    if (qobject_cast<const QTabBar *>(widget))
        return QQStyleKitReader::TabBar;
#endif
#if QT_CONFIG(tabwidget)
    if (qobject_cast<const QTabWidget *>(widget))
        return QQStyleKitReader::Page;
#endif
#if QT_CONFIG(toolbar)
    if (qobject_cast<const QToolBar *>(widget))
        return QQStyleKitReader::ToolBar;
#endif
#if QT_CONFIG(toolbutton)
    if (qobject_cast<const QToolButton *>(widget))
        return QQStyleKitReader::ToolButton;
#endif
#if QT_CONFIG(groupbox)
    if (qobject_cast<const QGroupBox *>(widget))
        return QQStyleKitReader::GroupBox;
#endif
#if QT_CONFIG(menu)
    if (qobject_cast<const QMenuBar *>(widget))
        return QQStyleKitReader::MenuBar;
    if (qobject_cast<const QMenu *>(widget))
        return QQStyleKitReader::Menu;
#endif
    if (widget->windowType() & Qt::Popup)
        return QQStyleKitReader::Popup;
    if (widget->windowType() & Qt::Dialog)
        return QQStyleKitReader::Dialog;
    if (widget->windowType() & Qt::Window)
        return QQStyleKitReader::ApplicationWindow;
#ifndef QT_NO_FRAME
    if (qobject_cast<const QFrame *>(widget))
        return QQStyleKitReader::Frame;
#endif

    return QQStyleKitReader::Control;
}

#if QT_CONFIG(itemviews)
static QQStyleKitReader::ControlType itemViewControlType(const QStyleOption *opt)
{
    const auto *itemViewOption = qstyleoption_cast<const QStyleOptionViewItem *>(opt);
    return itemViewOption && (itemViewOption->features & QStyleOptionViewItem::HasCheckIndicator)
        ? QQStyleKitReader::ControlType::CheckDelegate
        : QQStyleKitReader::ControlType::ItemDelegate;
}
#endif

// Some widgets (like QAbstractScrollArea) paint on a child widget (the viewport)
// instead of on themselves. This function returns the widget that is actually painted on,
// which is the one that should be updated when the style values change.
static QWidget *paintTarget(const QWidget *widget)
{
#if QT_CONFIG(scrollarea)
    if (const auto *area = qobject_cast<const QAbstractScrollArea *>(widget))
        return area->viewport();
#endif
    return const_cast<QWidget *>(widget);
}

// Returns true for widgets that draw themselves using their widget font and
// palette directly, bypassing the style's drawControl path.
static bool isSelfPaintingWidget(const QWidget *widget)
{
    return false
#if QT_CONFIG(label)
        || qobject_cast<const QLabel *>(widget)
#endif
#if QT_CONFIG(textedit)
        || qobject_cast<const QTextEdit *>(widget)
        || qobject_cast<const QPlainTextEdit *>(widget)
#endif
#if QT_CONFIG(lineedit)
        || qobject_cast<const QLineEdit *>(widget)
#endif
        ;
}

static QWidget *managedViewport(QWidget *widget)
{
#if QT_CONFIG(textedit)
    if (auto *textEdit = qobject_cast<QTextEdit *>(widget))
        return textEdit->viewport();
    if (auto *plainTextEdit = qobject_cast<QPlainTextEdit *>(widget))
        return plainTextEdit->viewport();
#endif
#if QT_CONFIG(itemviews) && QT_CONFIG(combobox)
    if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
        if (auto *p = view->parentWidget(); p && p->inherits("QComboBoxPrivateContainer"))
            return view->viewport();
    }
#endif
    return nullptr;
}

static uint resolvedAlignment(uint raw, Qt::Alignment hDefault, Qt::Alignment vDefault)
{
    uint result = 0;
    const uint h = raw & Qt::AlignHorizontal_Mask;
    const uint v = raw & Qt::AlignVertical_Mask;
    result |= h ? h : uint(hDefault);
    result |= v ? v : uint(vDefault);
    return result;
}

static qreal resolvedWidth(const QQStyleKitDelegateProperties *element, qreal availableW)
{
    return qMax(0.0, element->fillWidth() ? availableW : element->width());
}

static qreal resolvedHeight(const QQStyleKitDelegateProperties *element, qreal availableH)
{
    return qMax(0.0, element->fillHeight() ? availableH : element->height());
}

static QMargins elementMargins(const QQStyleKitDelegateProperties *element)
{
    using QtPrivate::qSaturateRound;
    return QMargins(qSaturateRound(element->leftMargin()),
                    qSaturateRound(element->topMargin()),
                    qSaturateRound(element->rightMargin()),
                    qSaturateRound(element->bottomMargin()));
}

/*! \internal
    Applies \a element's rotation and scale to \a painter about the centre of \a box.
    Returns \c false if the scale is 0 or if \a element is null, otherwise returns \c true.
*/
static bool applyDelegateTransform(QPainter *painter,
                                   const QQStyleKitDelegateProperties *element,
                                   const QRectF &box)
{
    if (!element)
        return false;

    const qreal scale = element->scale();
    if (qFuzzyIsNull(scale))
        return false;

    const qreal rotation = element->rotation();
    if (qFuzzyIsNull(rotation) && qFuzzyCompare(scale, 1.0))
        return true;

    const QPointF center = box.center();
    painter->translate(center);
    if (!qFuzzyIsNull(rotation))
        painter->rotate(rotation);
    if (!qFuzzyCompare(scale, 1.0))
        painter->scale(scale, scale);
    painter->translate(-center);
    return true;
}

// Copied from qstylesheetstyle.cpp
static const QWidget *containerWidget(const QWidget *w)
{
#if QT_CONFIG(lineedit)
    if (qobject_cast<const QLineEdit *>(w)) {
        // if the QLineEdit is an embeddedWidget, we need the real widget
        QWidget *parent = w->parentWidget();
        if (false
#  if QT_CONFIG(combobox)
            || qobject_cast<const QComboBox *>(parent)
#  endif
#  if QT_CONFIG(spinbox)
            || qobject_cast<const QAbstractSpinBox *>(parent)
#  endif
        ) {
            return parent;
        }
    }
#endif

#if QT_CONFIG(scrollarea)
    if (const QAbstractScrollArea *sa = qobject_cast<const QAbstractScrollArea *>(w->parentWidget())) {
        if (sa->viewport() == w)
            return w->parentWidget();
    }
#endif

    return w;
}

QStyleKitStylePrivate::QStyleKitStylePrivate()
    : QCommonStylePrivate()
{
}

static QUrl urlFromStylePath(const QString &filePath)
{
    return filePath.startsWith(QLatin1Char(':'))
        ? QUrl(QLatin1String("qrc") + filePath)
        : QUrl::fromLocalFile(filePath);
}

bool QStyleKitStylePrivate::loadStyle()
{
    Q_Q(QStyleKitStyle);
    if (!qmlEngine)
        qmlEngine = new QQmlEngine(q);

    if (!qmlEngine) {
        qWarning("QStyleKitStyle: No QML engine available to load style.");
        return false;
    }
    const QUrl url = stylePath.isEmpty() ? QUrl() : urlFromStylePath(stylePath);
    if (stylePath.isEmpty() || !url.isValid()) {
        qWarning("QStyleKitStyle: No valid style path provided: %s", qPrintable(stylePath));
        return false;
    }
    QQmlComponent component(qmlEngine, url);
    if (component.isError()) {
        qWarning("QStyleKitStyle: Failed to load style from %s: %s",
                 qPrintable(stylePath), qPrintable(component.errorString()));
        return false;
    }
    // Avoid creating anything other than StyleKit Style objects
    // by checking the metaobject of the root type before creating the object
    QQmlComponentPrivate *componentPrivate = QQmlComponentPrivate::get(&component);
    const auto compilationUnit = componentPrivate->compilationUnit();
    const auto propertyCache = compilationUnit ? compilationUnit->rootPropertyCache() : nullptr;
    const auto firstMetaObject = propertyCache ? propertyCache->firstCppMetaObject() : nullptr;
    if (!firstMetaObject || !firstMetaObject->inherits(&QQStyleKitStyle::staticMetaObject)) {
        qWarning("QStyleKitStyle: Failed to load style from %s: component is not a StyleKit Style.",
                 qPrintable(stylePath));
        return false;
    }
    QQStyleKitStyle *styleObject = qobject_cast<QQStyleKitStyle *>(component.create());
    if (!styleObject) {
        qWarning("QStyleKitStyle: Failed to create style object from %s: component is not a StyleKit style.",
                 qPrintable(stylePath));
        return false;
    }
    const bool isReload = style != nullptr;
    delete style;
    style = styleObject;
    style->setParent(q);

    if (!isReload) {
        QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, q, [this]() {
            if (style && style->themeName() == QLatin1String("System")) {
                style->recreateTheme();
                updateStyle();
            }
        });
    }

    if (style->loaded())
        QQStyleKitReader::resetReadersForStyle(style);

    return true;
}

/*! \internal
    Creates and returns the empty fallback style used when no user
    style is loaded.
*/
QQStyleKitStyle *QStyleKitStylePrivate::ensureDefaultStyle()
{
    if (defaultStyle)
        return defaultStyle;

    Q_Q(QStyleKitStyle);
    if (!qmlEngine)
        qmlEngine = new QQmlEngine(q);

    QQmlComponent component(qmlEngine);
    component.loadFromModule("Qt.labs.StyleKit", "Style");
    if (component.isError()) {
        qWarning("QStyleKitStyle: Failed to create default fallback style: %s",
                 qPrintable(component.errorString()));
        return nullptr;
    }
    defaultStyle = qobject_cast<QQStyleKitStyle *>(component.create());
    if (defaultStyle) {
        defaultStyle->setParent(q);
        qCDebug(lcStyleKit, "No style set; using a default fallback style. "
                            "Set stylePath to load a StyleKit Style.");
    }
    return defaultStyle;
}

/*! \internal
    Returns the user style if one is loaded, otherwise the default fallback
    style
*/
QQStyleKitStyle *QStyleKitStylePrivate::effectiveStyle() const
{
    return style ? style : defaultStyle;
}

void QStyleKitStylePrivate::updateStyle()
{
    clearMetricsCache();

    QQStyleKitStyle *effective = effectiveStyle();

    if (sharedReader && sharedReader->style() != effective)
        sharedReader->setExplicitStyle(effective);

    if (subElementReader && subElementReader->style() != effective)
        subElementReader->setExplicitStyle(effective);

    clearAllSubElements();

    for (auto *wr : std::as_const(widgetReaders)) {
        if (wr->style() != effective)
            wr->setExplicitStyle(effective);
    }

    if (!QWidgetPrivate::allWidgets)
        return;

    for (auto *widget : std::as_const(*QWidgetPrivate::allWidgets)) {
        if (customFontWidgets.contains(widget))
            unsetStyleFont(widget);
        refreshStyleFont(widget);
        if (customPaletteWidgets.contains(widget))
            unsetStylePalette(widget);
        refreshStylePalette(widget);
    }

    QEvent styleChange(QEvent::StyleChange);
    for (auto *widget : std::as_const(*QWidgetPrivate::allWidgets)) {
        QApplication::sendEvent(widget, &styleChange);
        widget->update();
    }
}

// Follow the approach in qstylesheetstyle.cpp
void QStyleKitStylePrivate::unsetStyleFont(QWidget *widget)
{
    auto it = customFontWidgets.find(widget);
    if (it == customFontWidgets.end())
        return;

    auto customFont = std::move(*it);
    customFontWidgets.erase(it);
    widget->setFont(std::move(customFont).reverted(widget->font()));
}

void QStyleKitStylePrivate::setStyleFont(QWidget *widget, const QFont &styleFont)
{
    if (!effectiveStyle() || !widget)
        return;

    const auto styleMask = styleFont.resolveMask();
    auto it = customFontWidgets.find(widget);

    if (it == customFontWidgets.end()) {
        if (styleMask == 0)
            return;
        it = customFontWidgets.insert(widget, { QWidgetPrivate::get(widget)->localFont(), 0 });
        QObject::connect(widget, &QObject::destroyed, widget, [this, widget]() {
            customFontWidgets.remove(widget);
        });
    }

    it->resolveMask = styleMask;

    const QFont &baseline = it->oldWidgetValue;
    QFont merged = styleFont.resolve(baseline);
    merged.setResolveMask(baseline.resolveMask() | styleMask);
    if (widget->font() != merged)
        widget->setFont(merged);
}

void QStyleKitStylePrivate::refreshStyleFont(QWidget *widget)
{
    if (!effectiveStyle() || !widget)
        return;

    const QWidget *targetWidget = containerWidget(widget);
    QQStyleKitReader::ControlType controlType = controlTypeForWidget(targetWidget);

    auto *shared = ensureSharedReader();
    if (!shared)
        return;

    QStyleOption opt;
    opt.initFrom(targetWidget);
    const QQSK::State currentState = resolvedStateFor(controlType, opt.state, targetWidget);

    shared->setControlTypeAndState(controlType, currentState);
    setStyleFont(widget, shared->font());
}

void QStyleKitStylePrivate::unsetStylePalette(QWidget *widget)
{
    auto it = customPaletteWidgets.find(widget);
    if (it == customPaletteWidgets.end())
        return;

    auto customPalette = std::move(*it);
    customPaletteWidgets.erase(it);
    widget->setPalette(std::move(customPalette).reverted(widget->palette()));
}

void QStyleKitStylePrivate::setStylePalette(QWidget *widget, const QPalette &stylePalette) const
{
    if (!effectiveStyle() || !widget)
        return;

    const quint64 styleMask = stylePalette.resolveMask();
    if (styleMask == 0)
        return;

    if (!customPaletteWidgets.contains(widget)) {
        customPaletteWidgets.insert(widget, { widget->palette(), 0 });
        QObject::connect(widget, &QObject::destroyed, widget, [this, widget]() {
            customPaletteWidgets.remove(widget);
        });
    }

    customPaletteWidgets[widget].resolveMask |= styleMask;
    QPalette merged = stylePalette.resolve(widget->palette());
    merged.setResolveMask(widget->palette().resolveMask() | styleMask);
    widget->setPalette(merged);
}

// For widgets that partially or fully draw themselves (instead of delegating to the style),
// using palette roles, we need to push the style colors to those roles
void QStyleKitStylePrivate::refreshStylePalette(QWidget *widget)
{
    if (!effectiveStyle() || !widget)
        return;

    const bool isWindow = widget->windowType() & (Qt::Popup | Qt::Window | Qt::Dialog);
    const bool isPaletteManaged = isSelfPaintingWidget(widget) || isWindow;
    if (!isPaletteManaged)
        return;

    const QWidget *targetWidget = containerWidget(widget);
    QQStyleKitReader::ControlType controlType = controlTypeForWidget(targetWidget);

    auto *shared = ensureSharedReader();
    if (!shared)
        return;

    QStyleOption opt;
    opt.initFrom(targetWidget);
    const QQSK::State currentState = resolvedStateFor(controlType, opt.state, targetWidget);

    QPalette stylePalette;
    if (isWindow) {
        // Windows draw their own background using the QPalette::Window role
        shared->setControlTypeAndState(controlType, currentState);
        if (const auto *bg = shared->global()->background(); bg && bg->isDefined(QQSK::Property::Color))
            stylePalette.setColor(QPalette::Window, bg->color());
        shared->setControlTypeAndState(controlType, QQSK::StateFlag::Disabled);
        if (const auto *dbg = shared->global()->background(); dbg && dbg->isDefined(QQSK::Property::Color))
            stylePalette.setColor(QPalette::Disabled, QPalette::Window, dbg->color());
    } else {
        // The remaining text-based widgets use the QPalette::Text/WindowText roles for their foreground color
        shared->setControlTypeAndState(controlType, currentState);
        if (const auto *text = shared->global()->text(); text && text->isDefined(QQSK::Property::Color)) {
            stylePalette.setColor(QPalette::Text, text->color());
            stylePalette.setColor(QPalette::WindowText, text->color());
        }
        shared->setControlTypeAndState(controlType, QQSK::StateFlag::Disabled);
        if (const auto *dt = shared->global()->text(); dt && dt->isDefined(QQSK::Property::Color)) {
            stylePalette.setColor(QPalette::Disabled, QPalette::Text, dt->color());
            stylePalette.setColor(QPalette::Disabled, QPalette::WindowText, dt->color());
        }
    }

    setStylePalette(widget, stylePalette);
}

/*! \internal
    Returns a reader for the given widget, creating and caching it if needed.
*/
QQStyleKitReader *QStyleKitStylePrivate::readerForWidget(const QWidget *widget) const
{
    Q_Q(const QStyleKitStyle);
    QQStyleKitStyle *effective = effectiveStyle();
    if (!effective || !widget)
        return nullptr;

    if (auto it = widgetReaders.find(widget); it != widgetReaders.end())
        return *it;

    auto *widgetReader = new QQStyleKitReader(const_cast<QStyleKitStyle *>(q));
    widgetReader->setExplicitStyle(effective);
    widgetReader->setTarget(paintTarget(widget));
    widgetReader->setCompleted(true);
    widgetReaders.insert(widget, widgetReader);
    QObjectPrivate::connect(widget, &QObject::destroyed, this,
                            &QStyleKitStylePrivate::onWidgetDestroyed, Qt::UniqueConnection);
    return widgetReader;
}

void QStyleKitStylePrivate::cleanupWidgetReader(const QWidget *widget) const
{
    if (auto *reader = widgetReaders.take(widget))
        reader->deleteLater();
    cleanupSubElements(widget);
}

void QStyleKitStylePrivate::onWidgetDestroyed(QObject *w) const
{
    cleanupWidgetReader(static_cast<QWidget *>(w));
}

/*! \internal
    Returns the key identifying the sub-element (item-view cell, menu item or
    menubar item) that \a opt refers to, or an invalid key if the option does
    not identify one.
*/
QStyleKitStylePrivate::SubElementKey QStyleKitStylePrivate::subElementKeyForOption(
    const QStyleOption *opt, const QWidget *widget)
{
    SubElementKey key;
    if (!widget)
        return key;

#if QT_CONFIG(itemviews)
    if (const auto *viewOpt = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
        const QModelIndex &idx = viewOpt->index;
        if (!idx.isValid())
            return key;
        key.widget = widget;
        key.id = SubElementKey::ItemViewCell{idx.model(), idx.internalId(), idx.row(), idx.column()};
        return key;
    }
#endif
#if QT_CONFIG(menu)
    if (const auto *menuOpt = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
        if (menuOpt->menuItemType == QStyleOptionMenuItem::Separator)
            return key;
        QAction *action = nullptr;
        if (const auto *menu = qobject_cast<const QMenu *>(widget))
            action = menu->actionAt(menuOpt->rect.center());
        else if (const auto *menuBar = qobject_cast<const QMenuBar *>(widget))
            action = menuBar->actionAt(menuOpt->rect.center());
        if (action) {
            key.widget = widget;
            key.id = SubElementKey::Action{ action };
        }
        return key;
    }
#endif
#if QT_CONFIG(tabbar)
    if (const auto *tabOpt = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
        if (tabOpt->tabIndex < 0)
            return key;
        key.widget = widget;
        key.id = SubElementKey::Tab{ tabOpt->tabIndex };
        return key;
    }
#endif
    return key;
}

/*! \internal
    Creates and returns a transitions-enabled reader for the given sub-element
    that animates from \a fromState to \a toState

*/
QQStyleKitReader *QStyleKitStylePrivate::startSubElementTransition(
    const SubElementKey &key, QQStyleKitReader::ControlType type,
    QQSK::State fromState, QQSK::State toState, QQuickTransition *transition) const
{
    Q_Q(const QStyleKitStyle);
    QQStyleKitStyle *effective = effectiveStyle();
    if (!effective)
        return nullptr;

    // Reclaim finished readers. running() is per (shared) transition object, so
    // it turns false only once no instance of that transition animates anywhere
    for (auto it = subElementAnimationReaders.begin();
         it != subElementAnimationReaders.end();) {
        const QQuickTransition *t = it.value()->transition();
        if (!t || !t->running()) {
            it.value()->deleteLater();
            it = subElementAnimationReaders.erase(it);
        } else {
            ++it;
        }
    }


    if (subElementAnimationReaders.size() >= kMaxSubElementAnimationReaders) {
        const auto it = subElementAnimationReaders.begin();
        if (const QWidget *widget = it.key().widget)
            paintTarget(widget)->update(); // snap to end state
        it.value()->deleteLater();
        subElementAnimationReaders.erase(it);
    }

    auto *reader = new QQStyleKitReader(const_cast<QStyleKitStyle *>(q));
    reader->setExplicitStyle(effective);
    reader->setTarget(paintTarget(key.widget));
    // just stores the fromState, doesn't start the animation yet
    reader->setControlTypeAndState(type, fromState);
    reader->setCompleted(true);
    // As delegates are created lazily, a fresh reader has none,
    // so we need to create them now so delegates can already start animating changes
    // Only create delegates actually used by subelements, ie: background, indicators
    reader->background();
    auto *indicator = reader->indicator();
    indicator->first()->foreground();
    indicator->second()->foreground();
    // Start the transition to the new state
    reader->setControlTypeAndState(type, toState);
    if (!transition || !transition->running()) {
        reader->deleteLater();
        return nullptr;
    }
    subElementAnimationReaders.insert(key, reader);
    return reader;
}

/*! \internal
    Drops all sub-element state tracking and animation readers for \a widget.
*/
void QStyleKitStylePrivate::cleanupSubElements(const QWidget *widget) const
{
    for (auto it = subElementStates.begin(); it != subElementStates.end();) {
        if (it.key().widget == widget)
            it = subElementStates.erase(it);
        else
            ++it;
    }
    for (auto it = subElementAnimationReaders.begin();
         it != subElementAnimationReaders.end();) {
        if (it.key().widget == widget) {
            it.value()->deleteLater();
            it = subElementAnimationReaders.erase(it);
        } else {
            ++it;
        }
    }
}

/*! \internal
    Drops all sub-element state tracking and animation readers.
*/
void QStyleKitStylePrivate::clearAllSubElements() const
{
    subElementStates.clear();
    for (QQStyleKitReader *reader : std::as_const(subElementAnimationReaders))
        reader->deleteLater();
    subElementAnimationReaders.clear();
}

/*! \internal
    Resolves the given QStyle::State into a QQSK::State based on the mapping of
    state flags for the given control type.
*/
QQSK::State QStyleKitStylePrivate::resolvedStateFor(
    QQStyleKitReader::ControlType type, QStyle::State state, const QWidget *widget) const
{
    QQSK::State flags;
    flags.setFlag(QQSK::StateFlag::Hovered, (state & QStyle::State_MouseOver));
    flags.setFlag(QQSK::StateFlag::Pressed, state & QStyle::State_Sunken);
    flags.setFlag(QQSK::StateFlag::Checked, state & QStyle::State_On);
    flags.setFlag(QQSK::StateFlag::Focused, state & QStyle::State_HasFocus);
    flags.setFlag(QQSK::StateFlag::Highlighted, state & QStyle::State_Selected);
    flags.setFlag(QQSK::StateFlag::Vertical, !(state & QStyle::State_Horizontal));
    // Some widgets don't set State_Enabled in their
    // style option even when the widget is enabled.
    // Fall back to widget->isEnabled() in that case
    const bool disabled = (state & QStyle::State_Enabled) ? false
                        : (widget ? !widget->isEnabled() : true);
    flags.setFlag(QQSK::StateFlag::Disabled, disabled);

    // ComboBox uses QStyle::State_On to indicate the popup is open, which is
    // not a "checked" semantic.
    if (type == QQStyleKitReader::ControlType::ComboBox)
        flags.setFlag(QQSK::StateFlag::Checked, false);

    // Popup has no hover state in the Controls style
    if (type == QQStyleKitReader::ControlType::Popup)
        flags.setFlag(QQSK::StateFlag::Hovered, false);

    // QTabBar sets State_Selected (not State_On) on the current tab
    if (type == QQStyleKitReader::ControlType::TabButton) {
        flags.setFlag(QQSK::StateFlag::Checked, state & QStyle::State_Selected);
        flags.setFlag(QQSK::StateFlag::Highlighted, false);
    }

    return flags;
}

/*! \internal
    Returns the ControlMetrics for the given control type and state,
    reading from the style if needed and caching the result.
*/
const QStyleKitStylePrivate::ControlMetrics &QStyleKitStylePrivate::metricsFor(
    QQStyleKitReader::ControlType type, QQSK::State state) const
{
    if (auto it = metricsCache.find({ type, state }); it != metricsCache.end())
        return *it;

    auto *reader = ensureSharedReader();
    Q_ASSERT(reader);
    reader->setControlTypeAndState(type, state);
    return *metricsCache.insert({ type, state }, metricsForReader(reader));
}

/*! \internal
    Resolves the properties for the given widget, control type and state,
    and returns them as a QQStyleKitResolved.
*/
QStyleKitStylePrivate::QQStyleKitResolved QStyleKitStylePrivate::resolve(
    const QWidget *w, QQStyleKitReader::ControlType type, QStyle::State state) const
{
    QQStyleKitResolved out;
    out.widget = w;

    auto *reader = readerForWidget(w);
    if (!reader)
        return out;

    const QQSK::State resolvedState = resolvedStateFor(type, state, w);
    reader->setControlTypeAndState(type, resolvedState);

    out.reader = reader;
    out.metrics = &metricsFor(type, resolvedState);
    return out;
}

/*! \internal
    Resolves the properties for a sub-element of \a w (an item-view item, menu/
    menuBar item), and returns them as a QQStyleKitResolved.

    Sub-elements in a steady-state use the shared, transitions-disabled
    subElementReader. When a sub-element is observed to change state and
    the style defines a transition for the new state, a transient per-item
    reader is created to run that transition

    Pass \a track = false for synthetic resolves that should not register state changes
    or retarget a running animation.
*/
QStyleKitStylePrivate::QQStyleKitResolved QStyleKitStylePrivate::resolveSubElement(
    const QWidget *w, const QStyleOption *opt,
    QQStyleKitReader::ControlType type, QStyle::State state, bool track) const
{
    QQStyleKitResolved out;
    out.widget = w;

    auto *steadyReader = ensureSubElementReader();
    if (!steadyReader)
        return out;

    const QQSK::State newState = resolvedStateFor(type, state, w);
    const SubElementKey key = subElementKeyForOption(opt, w);

    QQStyleKitReader *reader = nullptr;
    if (track && key.isValid()) {
        QQStyleKitReader *animReader = subElementAnimationReaders.value(key);
        const auto it = subElementStates.constFind(key);
        const bool seen = it != subElementStates.cend();

        // Fallback to a reasonable base state to transition from by stripping the
        // interactive/pointer-driven flags from the new state.
        const auto baseState = newState & ~(QQSK::State(QQSK::StateFlag::Hovered)
            | QQSK::StateFlag::Pressed
            | QQSK::StateFlag::Checked
            | QQSK::StateFlag::Focused
            | QQSK::StateFlag::Highlighted);
        const QQSK::State prevState = seen ? *it : baseState;
        if (prevState != newState) {
            if (animReader) {
                // State changed before transition finished
                // Retarget to new state, the transition continues from the current interpolated values
                animReader->setControlTypeAndState(type, newState);
            } else {
                // Only pointer-driven states should animate
                // If an item first comes into view checked/highlighted, etc.,
                // it should not animate
                const bool shouldAnimate = seen
                    || newState.testFlag(QQSK::StateFlag::Hovered)
                    || newState.testFlag(QQSK::StateFlag::Pressed);
                if (shouldAnimate) {
                    steadyReader->setControlTypeAndState(type, newState);
                    if (QQuickTransition *transition = steadyReader->transition())
                        animReader = startSubElementTransition(key, type, prevState,
                                                               newState, transition);
                }
            }
        }
        if (!seen || prevState != newState) {
            QObjectPrivate::connect(w, &QObject::destroyed, this,
                                    &QStyleKitStylePrivate::onWidgetDestroyed, Qt::UniqueConnection);
            if (subElementStates.size() >= kMaxSubElementStates)
                subElementStates.clear();
            subElementStates.insert(key, newState);
        }
        reader = animReader;
    }

    if (!reader)
        reader = steadyReader;
    reader->setControlTypeAndState(type, newState);

    out.reader = reader;
    out.metrics = &metricsFor(type, newState);
    return out;
}

const QQStyleKitTextProperties *QStyleKitStylePrivate::QQStyleKitResolved::text() const
{
    return reader ? reader->text() : nullptr;
}

const QQStyleKitDelegateProperties *QStyleKitStylePrivate::QQStyleKitResolved::background() const
{
    return reader ? reader->background() : nullptr;
}

const QQStyleKitHandleProperties *QStyleKitStylePrivate::QQStyleKitResolved::handle() const
{
    return reader ? reader->handle() : nullptr;
}

const QQStyleKitIndicatorWithSubTypes *QStyleKitStylePrivate::QQStyleKitResolved::indicator() const
{
    return reader ? reader->indicator() : nullptr;
}

QFont QStyleKitStylePrivate::QQStyleKitResolved::font() const
{
    return reader ? reader->font() : QFont();
}

QQStyleKitReader *QStyleKitStylePrivate::ensureSharedReader() const
{
    if (sharedReader)
        return sharedReader;

    Q_Q(const QStyleKitStyle);
    QQStyleKitStyle *effective = effectiveStyle();
    if (!effective)
        return nullptr;
    sharedReader = new QQStyleKitReader(const_cast<QStyleKitStyle *>(q));
    // Disable transitions since this reader is used for one-off metric reads in layout queries
    sharedReader->setTransitionsEnabled(false);
    sharedReader->setExplicitStyle(effective);
    sharedReader->setCompleted(true);
    return sharedReader;
}

QQStyleKitReader *QStyleKitStylePrivate::ensureSubElementReader() const
{
    if (subElementReader)
        return subElementReader;

    Q_Q(const QStyleKitStyle);
    QQStyleKitStyle *effective = effectiveStyle();
    if (!effective)
        return nullptr;
    subElementReader = new QQStyleKitReader(const_cast<QStyleKitStyle *>(q));
    // Disable transitions: this reader is retargeted across many sub-elements
    // per paint pass and must never animate between their states
    subElementReader->setTransitionsEnabled(false);
    subElementReader->setExplicitStyle(effective);
    subElementReader->setCompleted(true);
    return subElementReader;
}

/*! \internal
    Resolves the properties for the given control type and state using the shared reader,
    which disables transitions and is used for layout queries, so that layout never depends
    on animation state.
*/
QStyleKitStylePrivate::QQStyleKitLayoutResolved QStyleKitStylePrivate::resolveLayout(
    QQStyleKitReader::ControlType type, QStyle::State state) const
{
    QQStyleKitLayoutResolved out;
    auto *reader = ensureSharedReader();
    if (!reader)
        return out;

    const QQSK::State resolvedState = resolvedStateFor(type, state);
    out.metrics = &metricsFor(type, resolvedState);
    // metricsFor() sets the sharedReader to (type, resolvedState) on cache miss.
    // On hit it didn't, so make sure staticProps / staticFont reflect the state
    // and type of the caller
    reader->setControlTypeAndState(type, resolvedState);
    out.staticProps = reader->global();
    out.staticFont = reader->font();
    return out;
}

const QQStyleKitTextProperties *QStyleKitStylePrivate::QQStyleKitLayoutResolved::text() const
{
    return staticProps ? staticProps->text() : nullptr;
}

const QQStyleKitDelegateProperties *QStyleKitStylePrivate::QQStyleKitLayoutResolved::background() const
{
    return staticProps ? staticProps->background() : nullptr;
}

const QQStyleKitHandleProperties *QStyleKitStylePrivate::QQStyleKitLayoutResolved::handle() const
{
    return staticProps ? staticProps->handle() : nullptr;
}

const QQStyleKitIndicatorWithSubTypes *QStyleKitStylePrivate::QQStyleKitLayoutResolved::indicator() const
{
    return staticProps ? staticProps->indicator() : nullptr;
}

void QStyleKitStylePrivate::clearMetricsCache()
{
    metricsCache.clear();
}

void QStyleKitStylePrivate::drawControlIndicator(const QQStyleKitDelegateProperties *indicator, const QRectF &rect, QPainter *painter) const
{
    if (!indicator || !indicator->visible() || indicator->opacity() <= 0 || !rect.isValid())
        return;

    // The foreground is a "child" of the indicator container so it
    // inherits the indicator's transform. Apply it here and let the foreground stack its
    // own on top.
    QPainterStateGuard stateGuard(painter);
    if (!applyDelegateTransform(painter, indicator, rect))
        return;

    // indicator (background)
    QRectF indicatorRect = rect;
    drawStyledItemContents(indicator, indicatorRect, painter);

    const QQStyleKitDelegateProperties *foreground = nullptr;
    if (auto *indicatorWithSubTypes = qobject_cast<const QQStyleKitIndicatorWithSubTypes *>(indicator)) {
        foreground = indicatorWithSubTypes->foreground();
    } else if (auto *indicatorProps = qobject_cast<const QQStyleKitIndicatorProperties *>(indicator)) {
        foreground = indicatorProps->foreground();
    } else {
        return;
    }
    if (!foreground || !foreground->visible() || foreground->opacity() <= 0)
        return;

    // foreground
    QRectF foregroundRect;
    const uint foregroundAlign = resolvedAlignment(foreground->alignment(), Qt::AlignHCenter, Qt::AlignVCenter);
    const auto foregroundW = resolvedWidth(foreground,
        indicatorRect.width() - foreground->leftMargin() - foreground->rightMargin());
    const auto foregroundH = resolvedHeight(foreground,
        indicatorRect.height() - foreground->topMargin() - foreground->bottomMargin());
    foregroundRect.setSize(QSizeF(foregroundW, foregroundH));
    if (foregroundAlign & Qt::AlignLeft)
        foregroundRect.moveLeft(indicatorRect.left() + foreground->leftMargin());
    else if (foregroundAlign & Qt::AlignHCenter)
        foregroundRect.moveLeft(indicatorRect.left() + (indicatorRect.width() - foregroundW) / 2.0);
    else if (foregroundAlign & Qt::AlignRight)
        foregroundRect.moveLeft(indicatorRect.right() - foreground->rightMargin() - foregroundW);
    if (foregroundAlign & Qt::AlignTop)
        foregroundRect.moveTop(indicatorRect.top() + foreground->topMargin());
    else if (foregroundAlign & Qt::AlignVCenter)
        foregroundRect.moveTop(indicatorRect.top() + (indicatorRect.height() - foregroundH) / 2.0);
    else if (foregroundAlign & Qt::AlignBottom)
        foregroundRect.moveTop(indicatorRect.bottom() - foreground->bottomMargin() - foregroundH);
    drawStyledItemRect(foreground, foregroundRect, painter);
}

void QStyleKitStylePrivate::drawControlText(const QQStyleKitTextProperties *textProps,
                                             const QFont &font, const QRect &rect,
                                             const QString &text, uint textFlags,
                                             QPainter *p, Qt::Alignment defaultAlignment) const
{
    uint flags = textFlags;
    flags |= textProps
        ? resolvedAlignment(textProps->alignment(),
                            defaultAlignment & Qt::AlignHorizontal_Mask,
                            defaultAlignment & Qt::AlignVertical_Mask)
        : uint(defaultAlignment);

    const QFont oldFont = p->font();
    p->setFont(font.resolve(oldFont));
    p->setBrush(Qt::NoBrush);
    p->setPen(textProps ? textProps->color() : QColor());
    p->drawText(rect, flags, text, nullptr);
}

void QStyleKitStylePrivate::drawStyledItemRect(const QQStyleKitDelegateProperties *props, const QRectF &rect, QPainter *painter) const
{
    if (!props || !props->visible() || props->opacity() <= 0 || !rect.isValid())
        return;

    QPainterStateGuard stateGuard(painter);
    if (!applyDelegateTransform(painter, props, rect))
        return;
    drawStyledItemContents(props, rect, painter);
}

void QStyleKitStylePrivate::drawStyledItemContents(const QQStyleKitDelegateProperties *props,
                                                   const QRectF &rect, QPainter *painter) const
{
    if (!props || !props->visible() || props->opacity() <= 0 || !rect.isValid())
        return;

    QPainterStateGuard stateGuard(painter);
    painter->setRenderHint(QPainter::Antialiasing, true);
    if (props->clip())
        painter->setClipping(true);

    // opacity
    const auto opacity = props->opacity();

    // border
    const auto *border = props->border();
    auto borderWidth = 0;
    auto inset = 0.0;
    QColor borderColor;
    if (border) {
        borderWidth = border->width();
        borderColor = border->color();
        inset = borderWidth / 2.0;
    }
    if (borderWidth > 0 && borderColor.isValid() && borderColor.alpha() > 0) {
        QPen pen(borderColor, borderWidth);
        painter->setPen(pen);
    } else {
        painter->setPen(Qt::NoPen);
    }

    // radius
    // TODO: support different radius for each corner
    const qreal minDimension = qMin(rect.width(), rect.height());
    const qreal xRadius = qMax(0.0, qMin(props->topLeftRadius() - inset, minDimension / 2.0));
    const qreal yRadius = qMax(0.0, qMin(props->bottomLeftRadius() - inset, minDimension / 2.0));

    QRectF adjustedRect = rect.adjusted(inset, inset, -inset, -inset);

    // gradients/color
    // TODO: support palette roles for colors and gradients
    QColor color = props->color();
    QBrush colorBrush(color);
    painter->setBrush(colorBrush);
    if (color.isValid()) {
        painter->setOpacity(opacity);
        painter->drawRoundedRect(adjustedRect, xRadius, yRadius, Qt::AbsoluteSize);
    }
    if (QQuickGradient *gradient = props->gradient()) {
        QLinearGradient linearGradient(rect.topLeft(), rect.bottomLeft());
        QQmlListProperty<QQuickGradientStop> stops = gradient->stops();
        const int stopCount = stops.count(&stops);
        for (int i = 0; i < stopCount; i++) {
            QQuickGradientStop *stop = static_cast<QQuickGradientStop *>(stops.at(&stops, i));
            linearGradient.setColorAt(stop->position(), stop->color());
        }
        QBrush gradientBrush(linearGradient);
        painter->setBrush(gradientBrush);
        painter->setOpacity(opacity);
        painter->drawRoundedRect(adjustedRect, xRadius, yRadius, Qt::AbsoluteSize);
    }

    // image
    drawStyledItemImage(props->image(), adjustedRect, opacity, painter);
}

void QStyleKitStylePrivate::drawStyledItemImage(const QQStyleKitImageProperties *image, const QRectF &rect,
                                                qreal opacity, QPainter *painter) const
{
    if (!image || image->source().isEmpty() || image->color().alpha() <= 0)
        return;

    QString imageSource = image->source().toString();
    if (imageSource.startsWith(QLatin1String("qrc:/")))
        imageSource = imageSource.mid(3);
    QUrl imageUrl(imageSource);
    QString imagePath = imageUrl.isLocalFile() ? imageUrl.toLocalFile() : imageSource;
    QPixmap pixmap(imagePath);
    if (pixmap.isNull())
        return;

    if (image->color().isValid()) {
        QImage coloredImage = pixmap.toImage();
        QPainter imagePainter(&coloredImage);
        imagePainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        imagePainter.fillRect(coloredImage.rect(), image->color());
        imagePainter.end();
        pixmap = QPixmap::fromImage(coloredImage);
    }
    painter->setOpacity(opacity);
    const QSizeF pixmapSize = pixmap.deviceIndependentSize();

    switch (image->fillMode()) {
    case QQuickImage::PreserveAspectFit: {
        const QSize scaled = pixmapSize.scaled(rect.size(), Qt::KeepAspectRatio).toSize();
        const QPointF topLeft(
            rect.x() + (rect.width() - scaled.width()) / 2.0,
            rect.y() + (rect.height() - scaled.height()) / 2.0
        );
        painter->drawPixmap(QRectF(topLeft, scaled), pixmap, QRectF(QPointF(0, 0), pixmapSize));
        break;
    }
    case QQuickImage::PreserveAspectCrop: {
        const QSize scaled = pixmapSize.scaled(rect.size(), Qt::KeepAspectRatioByExpanding).toSize();
        const QPointF topLeft(
            rect.x() + (rect.width() - scaled.width()) / 2.0,
            rect.y() + (rect.height() - scaled.height()) / 2.0
        );
        QPainterStateGuard stateGuard(painter);
        painter->setClipRect(rect);
        painter->drawPixmap(QRectF(topLeft, scaled), pixmap, QRectF(QPointF(0, 0), pixmapSize));
        break;
    }
    // TODO: Support remaining fill modes
    case QQuickImage::Stretch:
    default:
        painter->drawPixmap(rect, pixmap, QRectF(QPointF(0, 0), pixmapSize));
        break;
    }
}

QRect QStyleKitStylePrivate::getAlignedRectInContainer(const QRect &container, const QSize &size,
                                                       uint alignment, const QMargins &padding,
                                                       const QMargins &margins) const
{
    QRect r(QPoint(0, 0), size);

    if (alignment & Qt::AlignLeft)
        r.moveLeft(container.x() + padding.left() + margins.left());
    else if (alignment & Qt::AlignHCenter)
        r.moveLeft(container.x() + (container.width() - size.width()) / 2);
    else // AlignRight
        r.moveLeft(container.x() + container.width() - padding.right() - margins.right() - size.width());

    if (alignment & Qt::AlignTop)
        r.moveTop(container.y() + padding.top() + margins.top());
    else if (alignment & Qt::AlignVCenter)
        r.moveTop(container.y() + (container.height() - size.height()) / 2);
    else // AlignBottom
        r.moveTop(container.y() + container.height() - padding.bottom() - margins.bottom() - size.height());

    return r;
}

QStyleKitStylePrivate::ControlMetrics QStyleKitStylePrivate::metricsForReader(QQStyleKitReader *reader) const
{
    Q_ASSERT(reader);
    using QtPrivate::qSaturateRound;
    const QQStyleKitControlProperties *props = reader->global();
    ControlMetrics metrics;
    metrics.bgImplicitSize = QSize(0, 0);
    metrics.textPadding = QMargins(0, 0, 0, 0);
    metrics.padding = QMargins(qSaturateRound(props->leftPadding()),
                               qSaturateRound(props->topPadding()),
                               qSaturateRound(props->rightPadding()),
                               qSaturateRound(props->bottomPadding()));
    metrics.spacing = qSaturateRound(props->spacing());
    metrics.margins = QMargins(0, 0, 0, 0);
    metrics.indicatorImplicitSize = QSize(0, 0);
    metrics.indicatorMargins = QMargins(0, 0, 0, 0);
    metrics.foregroundImplicitSize = QSize(0, 0);
    metrics.foregroundMargins = QMargins(0, 0, 0, 0);
    const auto *background = props->background();

    // Note: scale is deliberately absent here as it should not affect layout
    const auto elementSize = [](qreal w, qreal h) {
        constexpr qreal zero(.0);
        return QSizeF(std::max(zero, w), std::max(zero, h)).toSize();
    };

    if (background) {
        metrics.bgImplicitSize = elementSize(background->width(), background->height());
        metrics.margins = elementMargins(background);
    }
    const auto *textProps = props->text();
    if (textProps)
        metrics.textPadding = QMargins(qSaturateRound(textProps->leftPadding()),
                                       qSaturateRound(textProps->topPadding()),
                                       qSaturateRound(textProps->rightPadding()),
                                       qSaturateRound(textProps->bottomPadding()));
    const auto *indicator = props->indicator();
    if (indicator) {
        metrics.indicatorMargins = elementMargins(indicator);
        metrics.indicatorImplicitSize = elementSize(indicator->width(), indicator->height());

        const auto *foreground = indicator->foreground();
        if (foreground) {
            metrics.foregroundMargins = elementMargins(foreground);
            const auto foregroundW = resolvedWidth(foreground,
                std::max(.0, qreal(metrics.indicatorImplicitSize.width()
                                    - metrics.foregroundMargins.left()
                                    - metrics.foregroundMargins.right())));
            const auto foregroundH = resolvedHeight(foreground,
                std::max(.0, qreal(metrics.indicatorImplicitSize.height()
                                    - metrics.foregroundMargins.top()
                                    - metrics.foregroundMargins.bottom())));
            metrics.foregroundImplicitSize = elementSize(foregroundW, foregroundH);
        }
    }
    const auto *handle = props->handle();
    if (handle) {
        metrics.handleImplicitSize = elementSize(handle->width(), handle->height());
        metrics.handleMargins = elementMargins(handle);
    }
    return metrics;
}

/*!
    Constructs a QStyleKitStyle with no style loaded.

    Use \l setStylePath() to load a QML \l Style after construction.
    Until a style is loaded, the style uses a default fallback style.
*/
QStyleKitStyle::QStyleKitStyle()
    : QCommonStyle(*new QStyleKitStylePrivate())
{
}

/*!
    Constructs a QStyleKitStyle and loads the QML \l Style at \a filePath.

    \a filePath is a path to a local file or a path to a file in the resource
    file system; a relative path is resolved against the application's working
    directory. If the path is invalid or the root object of the loaded component
    is not a \l Style, a warning is emitted and the constructed style uses a
    default fallback style until a valid \l stylePath is set.
*/
QStyleKitStyle::QStyleKitStyle(const QString &filePath)
    : QCommonStyle(*new QStyleKitStylePrivate())
{
    Q_D(QStyleKitStyle);
    d->stylePath = filePath;
    if (!d->loadStyle()) {
        qWarning("QStyleKitStyle: Failed to load style from %s", qPrintable(filePath));
    }
}

/*!
    Destroys the QStyleKitStyle.
*/
QStyleKitStyle::~QStyleKitStyle()
{
}

/*!
    Returns the path of the currently loaded \l Style file.

    \sa setStylePath()
*/
QString QStyleKitStyle::stylePath() const
{
    Q_D(const QStyleKitStyle);
    return d->stylePath;
}

/*!
    Loads the QML \l Style at \a filePath and applies it to all widgets.

    \a filePath is a path to a local file or a path to a file in the resource
    file system; see the \l stylePath property for the accepted forms. If it is
    the same as the current \l stylePath, this function does nothing. If the new
    style cannot be loaded, the previously loaded style remains active and a
    warning is emitted; \l stylePathChanged() is still emitted to reflect
    the changed property value.

    \sa stylePath()
*/
void QStyleKitStyle::setStylePath(const QString &filePath)
{
    Q_D(QStyleKitStyle);
    if (d->stylePath == filePath)
        return;
    d->stylePath = filePath;
    if (d->loadStyle())
        d->updateStyle();
    emit stylePathChanged();
}

/*!
    Returns the name of the currently active theme, or an empty string
    if no \l Style has been loaded.

    \sa setThemeName(), availableThemeNames()
*/
QString QStyleKitStyle::themeName() const
{
    Q_D(const QStyleKitStyle);
    return d->style ? d->style->themeName() : QString();
}

/*!
    Activates the theme named \a themeName.

    \a themeName must be one of the entries in \l availableThemeNames(), or the
    special name \c System to follow the platform color scheme. If no
    \l Style has been loaded, this function emits a warning and returns
    without changing the active theme.

    \sa themeName(), availableThemeNames()
*/
void QStyleKitStyle::setThemeName(const QString &themeName)
{
    Q_D(QStyleKitStyle);
    if (!d->style) {
        qWarning("QStyleKitStyle: No style loaded, cannot set theme name.");
        return;
    }
    if (d->style->themeName() == themeName)
        return;
    d->style->setThemeName(themeName);
    d->updateStyle();
    emit themeNameChanged();
}

/*!
    Returns the names of all themes exposed by the loaded \l Style,
    including the built-in \c Light and \c Dark themes and any custom
    themes defined by the style. Returns an empty list when no style
    is loaded.

    \sa customThemeNames(), themeName()
*/
QStringList QStyleKitStyle::availableThemeNames() const
{
    Q_D(const QStyleKitStyle);
    return d->style ? d->style->availableThemeNames() : QStringList();
}

/*!
    Returns the names of the custom themes defined by the loaded
    \l Style, excluding the built-in \c Light and \c Dark themes.
    Returns an empty list when no style is loaded.

    \sa availableThemeNames()
*/
QStringList QStyleKitStyle::customThemeNames() const
{
    Q_D(const QStyleKitStyle);
    return d->style ? d->style->customThemeNames() : QStringList();
}

/*! \reimp */
void QStyleKitStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                                   const QWidget *w) const
{
    Q_D(const QStyleKitStyle);

    switch (pe) {
#if QT_CONFIG(menu)
    case PE_PanelMenu: {
        const auto controlType = w && w->inherits("QComboBoxPrivateContainer")
            ? QQStyleKitReader::ControlType::Popup
            : QQStyleKitReader::ControlType::Menu;
        const auto r = d->resolve(w, controlType, opt->state);
        if (!r.isValid())
            break;
        d->drawStyledItemRect(r.background(), opt->rect.marginsRemoved(r.metrics->margins), p);
        return;
    }
    case PE_FrameMenu:
        // already drawn in PE_PanelMenu
        return;
#endif // QT_CONFIG(menu)
#if QT_CONFIG(menubar)
    case PE_PanelMenuBar: {
        const auto r = d->resolve(w, QQStyleKitReader::ControlType::MenuBar, opt->state);
        if (!r.isValid())
            break;
        d->drawStyledItemRect(r.background(), opt->rect.marginsRemoved(r.metrics->margins), p);
        return;
    }
#endif // QT_CONFIG(menubar)
    case PE_FrameButtonBevel:
        if (const auto *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            const auto controlType = btn->features & QStyleOptionButton::Flat
                ? QQStyleKitReader::ControlType::FlatButton
                : QQStyleKitReader::ControlType::Button;
            const auto r = d->resolve(w, controlType, btn->state);
            if (!r.isValid())
                break;
            d->drawStyledItemRect(r.background(), opt->rect, p);
            return;
        }
        break;
    case PE_Frame: {
        QQStyleKitReader::ControlType controlType;
        const bool isPopup = w && (false
#if QT_CONFIG(combobox)
                || w->inherits("QComboBoxPrivateContainer")
#endif
            );
        if (isPopup)
            controlType = QQStyleKitReader::ControlType::Popup;
#if QT_CONFIG(lineedit)
        else if (qobject_cast<const QLineEdit *>(w))
            controlType = QQStyleKitReader::ControlType::TextField;
#endif
        else
            controlType = QQStyleKitReader::ControlType::Frame;
        const auto r = d->resolve(w, controlType, opt->state);
        if (!r.isValid())
            break;
        d->drawStyledItemRect(r.background(), opt->rect, p);
        return;
    }
#if QT_CONFIG(lineedit)
    case PE_PanelLineEdit:
        if (const auto *lineEdit = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            // LineEdit sets Sunken flag to indicate Sunken frame,
            // but the style uses it to indicate pressed state, so ignore it
            QStyleOption lineEditOpt(*lineEdit);
            lineEditOpt.state &= ~QStyle::State_Sunken;
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::TextField, lineEditOpt.state);
            if (!r.isValid())
                break;

            // LineEdit draws its own text using the Text role so update that role in the palette
            if (auto *le = qobject_cast<const QLineEdit *>(w)) {
                if (const auto *txt = r.text(); txt && txt->isDefined(QQSK::Property::Color)) {
                    QPalette stylePalette;
                    stylePalette.setColor(QPalette::Text, txt->color());
                    d->setStylePalette(const_cast<QLineEdit *>(le), stylePalette);
                }
                const_cast<QStyleKitStylePrivate *>(d)->setStyleFont(const_cast<QLineEdit *>(le), r.font());
            }

            const QObject *parent = w ? w->parent() : nullptr;
#  if QT_CONFIG(spinbox)
            const bool isInSpinBox = qobject_cast<const QAbstractSpinBox *>(parent);
#  else
            const bool isInSpinBox = false;
#  endif
#  if QT_CONFIG(combobox)
            const bool isInComboBox = qobject_cast<const QComboBox *>(parent);
#  else
            const bool isInComboBox = false;
#  endif
            // For spinbox and combobox, the line edit doesn't have its own background in the Controls style
            if (isInSpinBox || isInComboBox)
                return;

            d->drawStyledItemRect(r.background(), opt->rect, p);
            return;
        }
        break;
#endif // QT_CONFIG(lineedit)
#if QT_CONFIG(itemviews)
    case PE_PanelItemViewItem: {
        const auto r = d->resolveSubElement(w, opt, itemViewControlType(opt), opt->state);
        if (!r.isValid())
            break;
        d->drawStyledItemRect(r.background(), opt->rect, p);
        return;
    }
#endif // QT_CONFIG(itemviews)
    case PE_IndicatorCheckBox:
    case PE_IndicatorRadioButton: {
        const auto controlType = pe == PE_IndicatorCheckBox
            ? QQStyleKitReader::ControlType::CheckBox
            : QQStyleKitReader::ControlType::RadioButton;
        const auto r = d->resolve(w, controlType, opt->state);
        if (!r.isValid())
            break;
        d->drawControlIndicator(r.indicator(), opt->rect, p);
        return;
    }
    case PE_IndicatorArrowDown: {
        const auto r = d->resolve(w, QQStyleKitReader::ControlType::ComboBox, opt->state);
        if (!r.isValid())
            break;
        d->drawControlIndicator(r.indicator(), opt->rect, p);
        return;
    }
    case PE_IndicatorItemViewItemCheck: {
        // track = false: Checked state is synthetic
        const auto r = d->resolveSubElement(w, opt, itemViewControlType(opt), opt->state, false);
        if (!r.isValid())
            break;
        d->drawControlIndicator(r.indicator(), opt->rect, p);
        return;
    }
#if QT_CONFIG(spinbox)
    case PE_IndicatorSpinUp:
    case PE_IndicatorSpinDown: {
        const auto r = d->resolve(w, QQStyleKitReader::ControlType::SpinBox, opt->state);
        if (!r.isValid())
            break;
        const auto *indicator = r.indicator();
        const auto *upDownIndicator = indicator ? (pe == PE_IndicatorSpinUp ? indicator->first() : indicator->second()) : nullptr;
        d->drawControlIndicator(upDownIndicator, opt->rect, p);
        return;
    }
#endif // QT_CONFIG(spinbox)
#if QT_CONFIG(groupbox)
    case PE_FrameGroupBox: {
        const auto r = d->resolve(w, QQStyleKitReader::ControlType::GroupBox, opt->state);
        if (!r.isValid())
            break;
        d->drawStyledItemRect(r.background(), opt->rect, p);
        return;
    }
#endif // QT_CONFIG(groupbox)
#if QT_CONFIG(tabwidget)
    case PE_FrameTabWidget: {
        const auto r = d->resolve(w, QQStyleKitReader::ControlType::Page, opt->state);
        if (!r.isValid())
            break;
        d->drawStyledItemRect(r.background(), opt->rect, p);
        return;
    }
#endif // QT_CONFIG(tabwidget)
#if QT_CONFIG(tabbar)
    // Not (yet) supported in StyleKit
    case PE_IndicatorTabClose:
    case PE_IndicatorTabTearRight:
    case PE_IndicatorTabTearLeft:
        return;
#endif // QT_CONFIG(tabbar)
#if QT_CONFIG(toolbutton)
    case PE_FrameButtonTool:
    case PE_PanelButtonTool: {
        const auto r = d->resolve(w, QQStyleKitReader::ControlType::ToolButton, opt->state);
        if (!r.isValid())
            break;
        d->drawStyledItemRect(r.background(), opt->rect, p);
        return;
    }
#endif // QT_CONFIG(toolbutton)
#if QT_CONFIG(toolbar)
    case PE_IndicatorToolBarHandle:
        return; // no handle in Controls style
    case PE_IndicatorToolBarSeparator: {
        const auto r = d->resolve(w, QQStyleKitReader::ControlType::ToolSeparator, opt->state);
        if (!r.isValid())
            break;
        const auto &metrics = *r.metrics;
        // background
        QRect frameRect = opt->rect.marginsRemoved(metrics.margins);
        d->drawStyledItemRect(r.background(), frameRect, p);
        // indicator
        const auto *indicator = r.indicator();
        if (!indicator || !indicator->visible() || indicator->opacity() == 0)
            return;
        QRect contentRect = frameRect.marginsRemoved(metrics.padding);
        const int iw = QtPrivate::qSaturateRound(resolvedWidth(indicator,
            contentRect.width() - metrics.indicatorMargins.left() - metrics.indicatorMargins.right()));
        const int ih = QtPrivate::qSaturateRound(resolvedHeight(indicator,
            contentRect.height() - metrics.indicatorMargins.top() - metrics.indicatorMargins.bottom()));
        const uint alignment = resolvedAlignment(indicator->alignment(), Qt::AlignHCenter, Qt::AlignVCenter);
        const QRect indicatorRect = d->getAlignedRectInContainer(
            contentRect, QSize(iw, ih), alignment, QMargins(), metrics.indicatorMargins);
        d->drawControlIndicator(indicator, visualRect(opt->direction, contentRect, indicatorRect), p);
        return;
    }
#endif
    default:
        break;
    }
    QCommonStyle::drawPrimitive(pe, opt, p, w);
}

/*! \reimp */
void QStyleKitStyle::drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                                 const QWidget *w) const
{
    Q_D(const QStyleKitStyle);

    switch (element) {
    case CE_PushButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            proxy()->drawControl(CE_PushButtonBevel, btn, p, w);
            QStyleOptionButton btnContent(*btn);
            btnContent.rect = subElementRect(SE_PushButtonContents, btn, w);
            proxy()->drawControl(CE_PushButtonLabel, &btnContent, p, w);
        }
        return;
    case CE_PushButtonBevel:
        if (const auto *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            QStyleOptionButton btnBg(*btn);
            btnBg.rect = subElementRect(SE_PushButtonBevel, btn, w);
            proxy()->drawPrimitive(PE_FrameButtonBevel, &btnBg, p, w);
        }
        return;
    case CE_PushButtonLabel:
        if (const auto *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            const auto controlType = btn->features & QStyleOptionButton::Flat ? QQStyleKitReader::ControlType::FlatButton : QQStyleKitReader::ControlType::Button;
            const auto r = d->resolve(w, controlType, btn->state);
            if (!r.isValid())
                break;
            const auto metrics = r.metrics;

            QRect textRect = opt->rect;
            uint textFlags = Qt::TextShowMnemonic;
            if (!styleHint(SH_UnderlineShortcut, opt, w))
                textFlags |= Qt::TextHideMnemonic;

            // icon
            const QIcon icon = btn->icon;
            if (!icon.isNull()) {
                const auto *textProps = r.text();
                uint iconTextFlags = textFlags;
                iconTextFlags |= textProps
                    ? resolvedAlignment(textProps->alignment(), Qt::AlignHCenter, Qt::AlignVCenter)
                    : uint(Qt::AlignHCenter | Qt::AlignVCenter);
                QIcon::Mode mode = btn->state & State_Enabled ? QIcon::Normal : QIcon::Disabled;
                if (mode == QIcon::Normal && btn->state & State_HasFocus)
                    mode = QIcon::Active;
                QIcon::State iconState = btn->state & State_On ? QIcon::On : QIcon::Off;
                const auto paintDeviceDpr = p->device()->devicePixelRatio();
                QPixmap pixmap = icon.pixmap(btn->iconSize, paintDeviceDpr, mode, iconState);
                int pixmapWidth = pixmap.width() / pixmap.devicePixelRatio();
                int pixmapHeight = pixmap.height() / pixmap.devicePixelRatio();
                int labelWidth = pixmapWidth;
                int iconSpacing = metrics->spacing;
                int textWidth = btn->fontMetrics.boundingRect(opt->rect, iconTextFlags, btn->text).width();
                if (!btn->text.isEmpty())
                    labelWidth += (textWidth + iconSpacing + textProps->leftPadding() + textProps->rightPadding());

                QRect iconRect;
                if (iconTextFlags & Qt::AlignLeft) {
                    iconRect = QRect(textRect.x(), textRect.y() + (textRect.height() - pixmapHeight) / 2,
                                     pixmapWidth, pixmapHeight);
                } else if (iconTextFlags & Qt::AlignHCenter) {
                    iconRect = QRect(textRect.x() + (textRect.width() - labelWidth) / 2,
                                     textRect.y() + (textRect.height() - pixmapHeight) / 2,
                                     pixmapWidth, pixmapHeight);
                } else {
                    iconRect = QRect(textRect.x() + textRect.width() - labelWidth,
                                     textRect.y() + (textRect.height() - pixmapHeight) / 2,
                                     pixmapWidth, pixmapHeight);
                }
                iconRect = visualRect(btn->direction, textRect, iconRect);

                // After placing the icon, left-align the text relative to it
                textFlags &= ~Qt::AlignHorizontal_Mask;
                textFlags |= Qt::AlignLeft;
                if (btn->direction == Qt::RightToLeft)
                    textRect.setRight(iconRect.left() - iconSpacing);
                else
                    textRect.setLeft(iconRect.left() + iconRect.width() + iconSpacing);

                p->drawPixmap(iconRect, pixmap);
            }

            if (btn->features & QStyleOptionButton::HasMenu) {
                int indicatorSize = pixelMetric(PM_MenuButtonIndicator, btn, w);
                if (btn->direction == Qt::LeftToRight)
                    textRect = textRect.adjusted(0, 0, -indicatorSize, 0);
                else
                    textRect = textRect.adjusted(indicatorSize, 0, 0, 0);
            }
            d->drawControlText(r.text(), r.font(), textRect, btn->text, textFlags, p);
            return;
        }
        break;
    case CE_CheckBox:
    case CE_RadioButton:
        if (const auto *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            const auto controlType = element == CE_CheckBox ? QQStyleKitReader::ControlType::CheckBox
                                                            : QQStyleKitReader::ControlType::RadioButton;
            const auto r = d->resolve(w, controlType, btn->state);
            if (!r.isValid())
                break;
            QRect backgroundRect = btn->rect.marginsRemoved(r.metrics->margins);
            // background
            d->drawStyledItemRect(r.background(), backgroundRect, p);
            // label
            QStyleOptionButton btnContent(*btn);
            const auto contentElement = element == CE_CheckBox ? SE_CheckBoxContents : SE_RadioButtonContents;
            btnContent.rect = subElementRect(contentElement, btn, w);
            const auto labelElement = element == CE_CheckBox ? CE_CheckBoxLabel : CE_RadioButtonLabel;
            proxy()->drawControl(labelElement, &btnContent, p, w);
            // indicator
            QStyleOptionButton indicator(*btn);
            const auto indicatorElement = element == CE_CheckBox ? SE_CheckBoxIndicator : SE_RadioButtonIndicator;
            indicator.rect = subElementRect(indicatorElement, btn, w);
            const auto primitiveElement = element == CE_CheckBox ? PE_IndicatorCheckBox : PE_IndicatorRadioButton;
            proxy()->drawPrimitive(primitiveElement, &indicator, p, w);
            return;
        }
        break;
    case CE_CheckBoxLabel:
    case CE_RadioButtonLabel:
        if (const auto *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            const auto controlType = element == CE_CheckBoxLabel ? QQStyleKitReader::ControlType::CheckBox
                                                                 : QQStyleKitReader::ControlType::RadioButton;
            const auto r = d->resolve(w, controlType, btn->state);
            if (!r.isValid())
                break;
            uint textFlags = Qt::TextShowMnemonic;
            if (!styleHint(SH_UnderlineShortcut, opt, w))
                textFlags |= Qt::TextHideMnemonic;
            d->drawControlText(r.text(), r.font(), opt->rect, btn->text, textFlags, p);
            return;
        }
        break;
#if QT_CONFIG(combobox)
    case CE_ComboBoxLabel:
        if (const auto *comboBox = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            if (comboBox->editable)
                return;
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::ComboBox, comboBox->state);
            if (!r.isValid())
                break;
            const QRect textRect = subControlRect(CC_ComboBox, comboBox, SC_ComboBoxEditField, w)
                .marginsRemoved(r.metrics->textPadding);
            uint textFlags = Qt::TextShowMnemonic;
            if (!styleHint(SH_UnderlineShortcut, opt, w))
                textFlags |= Qt::TextHideMnemonic;
            d->drawControlText(r.text(), r.font(), textRect, comboBox->currentText, textFlags, p);
            return;
        }
        break;
#endif // QT_CONFIG(combobox)
#if QT_CONFIG(progressbar)
    case CE_ProgressBar:
        if (const QStyleOptionProgressBar *progressBar = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::ProgressBar, progressBar->state);
            if (!r.isValid())
                break;
            QRect backgroundRect = progressBar->rect.marginsRemoved(r.metrics->margins);
            // background
            d->drawStyledItemRect(r.background(), backgroundRect, p);
            // groove
            QStyleOptionProgressBar contents(*progressBar);
            contents.rect = subElementRect(SE_ProgressBarGroove, progressBar, w);
            const QRect grooveRect = contents.rect;
            proxy()->drawControl(CE_ProgressBarGroove, &contents, p, w);
            // track
            contents.rect = subElementRect(SE_ProgressBarContents, progressBar, w);
            // The track is the groove's foreground child, so it inherits the indicator's
            // transform
            QPainterStateGuard stateGuard(p);
            if (applyDelegateTransform(p, r.indicator(), grooveRect))
                proxy()->drawControl(CE_ProgressBarContents, &contents, p, w);
            // We intentionally don't draw the label as it is not drawn on the Controls Style
            return;
        }
        break;
    case CE_ProgressBarGroove: {
        // groove = indicator background
        const auto r = d->resolve(w, QQStyleKitReader::ControlType::ProgressBar, opt->state);
        if (!r.isValid())
            break;
        d->drawStyledItemRect(r.indicator(), opt->rect, p);
        return;
    }
    case CE_ProgressBarContents:
        // contents = indicator foreground
        if (const auto *progressBar = qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::ProgressBar, progressBar->state);
            if (!r.isValid())
                break;
            const auto progress = progressBar->progress;
            const auto ratio = progressBar->maximum > progressBar->minimum ? (progress - progressBar->minimum) / static_cast<qreal>(progressBar->maximum - progressBar->minimum) : 0;
            const auto width = static_cast<int>(progressBar->rect.width() * ratio);
            const auto x = progressBar->invertedAppearance ? progressBar->rect.right() - width : progressBar->rect.left();
            const auto contentsRect = QRect(x, progressBar->rect.y(), width, progressBar->rect.height());
            const auto *foreground = r.indicator() ? r.indicator()->foreground() : nullptr;
            if (foreground && foreground->visible() && foreground->opacity() > 0) {
                QPainterStateGuard stateGuard(p);
                if (applyDelegateTransform(p, foreground, progressBar->rect))
                    d->drawStyledItemContents(foreground, contentsRect, p);
            }
            return;
        }
        break;
#endif // QT_CONFIG(progressbar)
#if QT_CONFIG(itemviews)
    case CE_ItemViewItem:
        if (const auto *itemViewOption = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            const auto r = d->resolveSubElement(w, opt, itemViewControlType(opt), opt->state);
            if (!r.isValid())
                break;
            QStyleOptionViewItem optBg(*itemViewOption);
            optBg.rect = optBg.rect.marginsRemoved(r.metrics->margins);
            QRect indicatorRect = subElementRect(SE_ItemViewItemCheckIndicator, itemViewOption, w);
            QRect iconRect = subElementRect(SE_ItemViewItemDecoration, itemViewOption, w);
            QRect textRect = subElementRect(SE_ItemViewItemText, itemViewOption, w);
            // Capture text properties + font now, before the indicator
            // drawPrimitive below potentially mutates the reader's state
            const auto *itemTextProps = r.text();
            const QFont itemFont = r.font();

            // background
            proxy()->drawPrimitive(PE_PanelItemViewItem, &optBg, p, w);

            // indicator
            if (itemViewOption->features & QStyleOptionViewItem::HasCheckIndicator) {
                QStyleOptionViewItem option(*itemViewOption);
                option.rect = indicatorRect;
                option.state = option.state & ~QStyle::State_HasFocus;

                switch (itemViewOption->checkState) {
                case Qt::Unchecked:
                    option.state |= QStyle::State_Off;
                    break;
                case Qt::PartiallyChecked:
                    option.state |= QStyle::State_NoChange;
                    break;
                case Qt::Checked:
                    option.state |= QStyle::State_On;
                    break;
                }
                proxy()->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &option, p, w);
            }

            // icon
            QIcon::Mode mode = QIcon::Normal;
            if (!(itemViewOption->state & QStyle::State_Enabled))
                mode = QIcon::Disabled;
            else if (itemViewOption->state & QStyle::State_Selected)
                mode = QIcon::Selected;
            QIcon::State state = itemViewOption->state & QStyle::State_Open ? QIcon::On : QIcon::Off;
            itemViewOption->icon.paint(p, iconRect, itemViewOption->decorationAlignment, mode, state);

            // draw text
            uint textFlags = Qt::TextShowMnemonic;
            if (!styleHint(SH_UnderlineShortcut, opt, w))
                textFlags |= Qt::TextHideMnemonic;
            d->drawControlText(itemTextProps, itemFont, textRect, itemViewOption->text, textFlags, p,
                               Qt::AlignLeft | Qt::AlignVCenter);
            return;
        }
        break;
#endif // QT_CONFIG(itemviews)
#ifndef QT_NO_FRAME
    case CE_ShapedFrame: {
#  if QT_CONFIG(scrollarea)
        if (qobject_cast<const QAbstractScrollArea *>(w))
            // ScrollView has no styling in Controls, keep consistent
            return;
#  endif
        auto controlType = QQStyleKitReader::ControlType::Frame;
#  if QT_CONFIG(combobox)
        if (w && w->inherits("QComboBoxPrivateContainer"))
            controlType = QQStyleKitReader::ControlType::Popup;
#  endif
#  if QT_CONFIG(textedit)
        else if (w && qobject_cast<const QTextEdit *>(w))
            controlType = QQStyleKitReader::ControlType::TextArea;
#  endif
#  if QT_CONFIG(label)
        else if (w && qobject_cast<const QLabel *>(w))
            controlType = QQStyleKitReader::ControlType::Label;
#  endif
        // Fallback to Frame
        if (qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            const auto r = d->resolve(w, controlType, opt->state);
            if (!r.isValid())
                break;
            QRect backgroundRect = opt->rect.marginsRemoved(r.metrics->margins);
            d->drawStyledItemRect(r.background(), backgroundRect, p);
            return;
        }
        break;
    }
#endif // QT_NO_FRAME
#if QT_CONFIG(scrollbar)
    case CE_ScrollBarSlider: {
        const auto r = d->resolve(w, QQStyleKitReader::ControlType::ScrollBar, opt->state);
        if (!r.isValid())
            break;
        d->drawControlIndicator(r.indicator(), opt->rect, p);
        return;
    }
#endif // QT_CONFIG(scrollbar)
#if QT_CONFIG(toolbutton)
    case CE_ToolButtonLabel:
        if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::ToolButton, tool->state);
            if (!r.isValid())
                break;
            const auto *metrics = r.metrics;
            QRect rect = tool->rect.marginsRemoved(metrics->textPadding);

            uint textFlags = Qt::TextShowMnemonic;
            if (!styleHint(SH_UnderlineShortcut, opt, w))
                textFlags |= Qt::TextHideMnemonic;

            const bool hasArrow = tool->features & QStyleOptionToolButton::Arrow;
            if ((!hasArrow && tool->icon.isNull() && !tool->text.isEmpty())
                || tool->toolButtonStyle == Qt::ToolButtonTextOnly) {
                d->drawControlText(r.text(), r.font(), rect, tool->text, textFlags, p);
                return;
            }

            QPixmap pm;
            QSize pmSize;
            if (!hasArrow && !tool->icon.isNull()) {
                const QIcon::State iconState = tool->state & State_On ? QIcon::On : QIcon::Off;
                QIcon::Mode mode;
                if (!(tool->state & State_Enabled))
                    mode = QIcon::Disabled;
                else if ((tool->state & State_MouseOver) && (tool->state & State_AutoRaise))
                    mode = QIcon::Active;
                else
                    mode = QIcon::Normal;
                pm = tool->icon.pixmap(tool->iconSize, p->device()->devicePixelRatio(), mode, iconState);
                pmSize = pm.size() / pm.devicePixelRatio();
            }

            if (tool->toolButtonStyle == Qt::ToolButtonIconOnly || tool->text.isEmpty()) {
                if (!pm.isNull()) {
                    p->drawPixmap(visualRect(tool->direction, rect,
                        QRect(rect.x() + (rect.width() - pmSize.width()) / 2,
                              rect.y() + (rect.height() - pmSize.height()) / 2,
                              pmSize.width(), pmSize.height())), pm);
                }
                return;
            }

            if (tool->toolButtonStyle == Qt::ToolButtonTextUnderIcon) {
                if (!pm.isNull()) {
                    const QRect pr(rect.x() + (rect.width() - pmSize.width()) / 2,
                                   rect.y(), pmSize.width(), pmSize.height());
                    p->drawPixmap(visualRect(tool->direction, rect, pr), pm);
                    QRect tr = rect;
                    tr.setTop(rect.y() + pmSize.height() + metrics->spacing);
                    d->drawControlText(r.text(), r.font(), visualRect(tool->direction, rect, tr),
                                       tool->text, textFlags, p, Qt::AlignCenter);
                } else {
                    d->drawControlText(r.text(), r.font(), rect, tool->text, textFlags, p, Qt::AlignCenter);
                }
            } else {
                if (!pm.isNull()) {
                    const QRect pr(rect.x(), rect.y() + (rect.height() - pmSize.height()) / 2,
                                   pmSize.width(), pmSize.height());
                    p->drawPixmap(visualRect(tool->direction, rect, pr), pm);
                    QRect tr = rect;
                    tr.setLeft(rect.x() + pmSize.width() + metrics->spacing);
                    d->drawControlText(r.text(), r.font(), visualRect(tool->direction, rect, tr),
                                       tool->text, textFlags, p, Qt::AlignLeft | Qt::AlignVCenter);
                } else {
                    d->drawControlText(r.text(), r.font(), rect, tool->text, textFlags, p, Qt::AlignLeft | Qt::AlignVCenter);
                }
            }
            return;
        }
        break;
#endif // QT_CONFIG(toolbutton)
#if QT_CONFIG(menu)
    case CE_MenuItem:
        if (const auto *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            using QtPrivate::qSaturateRound;
            // MenuItemSeperator
            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                const auto r = d->resolve(w, QQStyleKitReader::ControlType::MenuSeparator, menuItem->state);
                if (!r.isValid())
                    break;
                const auto metrics = r.metrics;
                QRect contentRect = opt->rect.marginsRemoved(metrics->margins).marginsRemoved(metrics->padding);
                const auto *indicator = r.indicator();
                if (indicator && indicator->visible() && indicator->opacity() > 0) {
                    const int w = qSaturateRound(resolvedWidth(indicator,
                        contentRect.width() - indicator->leftMargin() - indicator->rightMargin()));
                    const int h = qSaturateRound(resolvedHeight(indicator,
                        contentRect.height() - indicator->topMargin() - indicator->bottomMargin()));
                    const uint align = resolvedAlignment(indicator->alignment(), Qt::AlignHCenter, Qt::AlignVCenter);
                    const QRect indicatorRect = d->getAlignedRectInContainer(
                        contentRect, QSize(w, h), align, QMargins(), elementMargins(indicator));
                    d->drawControlIndicator(indicator, visualRect(opt->direction, contentRect, indicatorRect), p);
                }
                return;
            }
            // MenuItem
            QStyle::State adjustedState = menuItem->state;
            if (adjustedState & State_Selected)
                adjustedState |= State_MouseOver;

            const auto r = d->resolveSubElement(w, opt, QQStyleKitReader::ControlType::MenuItem, adjustedState);
            if (!r.isValid())
                break;

            // background
            d->drawStyledItemRect(r.background(), opt->rect.marginsRemoved(r.metrics->margins), p);

            // indicators
            const QRect contentRect = opt->rect.marginsRemoved(r.metrics->padding);
            const auto *indicator = r.indicator();
            const auto *first = indicator ? indicator->first() : nullptr;
            const auto *second = indicator ? indicator->second() : nullptr;
            int leftOffset = 0, rightOffset = 0;
            const int spacing = r.metrics->spacing;
            QRect firstDrawRect, secondDrawRect;
            auto placeSubIndicator = [&](const QQStyleKitIndicatorProperties *sub, QRect &drawRect) {
                if (!sub)
                    return;
                const QSize sz(qSaturateRound(sub->width()),
                               qSaturateRound(sub->height()));
                const QMargins subMargins = elementMargins(sub);
                const int slotW = qSaturateRound(sub->leftMargin() + sub->width() + sub->rightMargin());
                if (sub->alignment() & Qt::AlignRight) {
                    const QRect slot(contentRect.right() - rightOffset - slotW, contentRect.top(),
                                     slotW, contentRect.height());
                    drawRect = d->getAlignedRectInContainer(slot, sz, Qt::AlignRight | Qt::AlignVCenter, {}, subMargins);
                    rightOffset += slotW + spacing;
                } else {
                    const QRect slot(contentRect.left() + leftOffset, contentRect.top(),
                                     slotW, contentRect.height());
                    drawRect = d->getAlignedRectInContainer(slot, sz, Qt::AlignLeft | Qt::AlignVCenter, {}, subMargins);
                    leftOffset += slotW + spacing;
                }
            };
            placeSubIndicator(first, firstDrawRect);
            placeSubIndicator(second, secondDrawRect);

            if (first && menuItem->checkType != QStyleOptionMenuItem::NotCheckable && menuItem->checked)
                d->drawControlIndicator(first, visualRect(menuItem->direction, contentRect, firstDrawRect), p);

            if (second && menuItem->menuItemType == QStyleOptionMenuItem::SubMenu)
                d->drawControlIndicator(second, visualRect(menuItem->direction, contentRect, secondDrawRect), p);

            // text
            QRect textRect = contentRect.adjusted(leftOffset, 0, -rightOffset, 0)
                                        .marginsRemoved(r.metrics->textPadding);
            if (!menuItem->icon.isNull()) {
                const int iconW = menuItem->maxIconWidth;
                const QIcon::Mode mode = (menuItem->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled;
                const int iconExtent = proxy()->pixelMetric(PM_SmallIconSize, opt, w);
                const QPixmap pm = menuItem->icon.pixmap(QSize(iconExtent, iconExtent),
                                                          p->device()->devicePixelRatio(), mode);
                const QSize pmSize = pm.size() / pm.devicePixelRatio();
                const int pmW = pmSize.width();
                const int pmH = pmSize.height();
                p->drawPixmap(visualRect(menuItem->direction, textRect,
                    QRect(textRect.left(), textRect.top() + (textRect.height() - pmH) / 2, pmW, pmH)), pm);
                textRect.setLeft(textRect.left() + iconW + r.metrics->spacing);
            }

            const int tabIdx = menuItem->text.indexOf(QLatin1Char('\t'));
            const QString label = tabIdx >= 0 ? menuItem->text.left(tabIdx) : menuItem->text;
            const QString shortcut = tabIdx >= 0 ? menuItem->text.mid(tabIdx + 1) : QString();
            uint textFlags = Qt::TextShowMnemonic;
            if (!styleHint(SH_UnderlineShortcut, opt, w))
                textFlags |= Qt::TextHideMnemonic;
            QRect labelRect = textRect;
            if (!shortcut.isEmpty() && menuItem->reservedShortcutWidth > 0) {
                const QRect shortcutRect(textRect.right() - menuItem->reservedShortcutWidth + 1,
                                         textRect.top(), menuItem->reservedShortcutWidth,
                                         textRect.height());
                labelRect.setRight(shortcutRect.left() - r.metrics->spacing - 1);
                const int shortcutFlags =
                    int(QStyle::visualAlignment(menuItem->direction,
                                                Qt::AlignRight | Qt::AlignVCenter))
                    | int(textFlags);
                p->save();
                p->setFont(r.font().resolve(p->font()));
                p->setBrush(Qt::NoBrush);
                if (const auto *shortcutText = r.text())
                    p->setPen(shortcutText->color());
                p->drawText(visualRect(menuItem->direction, textRect, shortcutRect),
                            shortcutFlags, shortcut);
                p->restore();
            }
            d->drawControlText(r.text(), r.font(),
                               visualRect(menuItem->direction, textRect, labelRect),
                               label, textFlags, p, Qt::AlignLeft | Qt::AlignVCenter);
            return;
        }
        break;
    // no tearoff or bottom scroller support in Controls style,
    //  so make it consistent and don't draw them
    case CE_MenuScroller:
    case CE_MenuTearoff:
        return;
    case CE_MenuEmptyArea:
        return;
#endif // QT_CONFIG(menu)
#if QT_CONFIG(menubar)
    case CE_MenuBarEmptyArea:
        return;
    case CE_MenuBarItem:
        if (const auto *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            QStyle::State adjustedState = menuItem->state;
            if (adjustedState & State_Selected)
                adjustedState |= State_MouseOver;
            const auto r = d->resolveSubElement(w, opt, QQStyleKitReader::ControlType::MenuBarItem, adjustedState);
            if (!r.isValid())
                break;
            d->drawStyledItemRect(r.background(), opt->rect.marginsRemoved(r.metrics->margins), p);
            const QRect textRect = opt->rect.marginsRemoved(
                r.metrics->margins + r.metrics->padding + r.metrics->textPadding);
            uint textFlags = Qt::TextShowMnemonic;
            if (!styleHint(SH_UnderlineShortcut, opt, w))
                textFlags |= Qt::TextHideMnemonic;
            d->drawControlText(r.text(), r.font(), textRect, menuItem->text, textFlags, p);
            return;
        }
        break;
#endif // QT_CONFIG(menubar)
#if QT_CONFIG(tabbar)
    case CE_TabBarTab:
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            const auto r = d->resolveSubElement(w, opt, QQStyleKitReader::ControlType::TabButton, tab->state);
            if (!r.isValid())
                break;
            const auto &m = *r.metrics;

            const QRect backgroundRect = opt->rect.marginsRemoved(m.margins);
            d->drawStyledItemRect(r.background(), backgroundRect, p);

            const QRect contentRect = backgroundRect.marginsRemoved(m.padding);
            uint textFlags = Qt::TextShowMnemonic;
            if (!styleHint(SH_UnderlineShortcut, opt, w))
                textFlags |= Qt::TextHideMnemonic;

            if (!tab->icon.isNull()) {
                const QIcon::Mode mode = (tab->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled;
                const QIcon::State iconState = (tab->state & State_Selected) ? QIcon::On : QIcon::Off;
                const QPixmap pm = tab->icon.pixmap(tab->iconSize, p->device()->devicePixelRatio(), mode, iconState);
                const QSize pmSize = pm.size() / pm.devicePixelRatio();
                const QRect pr(contentRect.x(), contentRect.y() + (contentRect.height() - pmSize.height()) / 2,
                               pmSize.width(), pmSize.height());
                p->drawPixmap(visualRect(tab->direction, contentRect, pr), pm);
                QRect tr = contentRect;
                tr.setLeft(contentRect.x() + pmSize.width() + m.spacing);
                d->drawControlText(r.text(), r.font(), visualRect(tab->direction, contentRect, tr),
                                   tab->text, textFlags, p, Qt::AlignCenter);
            } else {
                d->drawControlText(r.text(), r.font(), contentRect, tab->text, textFlags, p, Qt::AlignCenter);
            }
            return;
        }
        break;
#endif // QT_CONFIG(tabbar)
#if QT_CONFIG(toolbar)
    case CE_ToolBar:
        if (const auto *toolBar = qstyleoption_cast<const QStyleOptionToolBar *>(opt)) {
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::ToolBar, toolBar->state);
            if (!r.isValid())
                break;
            d->drawStyledItemRect(r.background(), opt->rect.marginsRemoved(r.metrics->margins), p);
            return;
        }
        break;
#endif // QT_CONFIG(toolbar)
    default:
        break;
    }
    QCommonStyle::drawControl(element, opt, p, w);
}

/*! \reimp */
QRect QStyleKitStyle::subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget) const
{
    using QtPrivate::qSaturateRound;
    Q_D(const QStyleKitStyle);

    switch (r) {
    case SE_PushButtonLayoutItem:
    case SE_PushButtonBevel:
        if (const auto *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            const auto controlType = btn->features & QStyleOptionButton::Flat
                ? QQStyleKitReader::ControlType::FlatButton
                : QQStyleKitReader::ControlType::Button;
            const auto resolved = d->resolveLayout(controlType, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            QRect rect = opt->rect.marginsRemoved(metrics.margins);
            return visualRect(opt->direction, opt->rect, rect);
        }
        break;
    case SE_PushButtonContents:
        if (const auto *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            const auto controlType = btn->features & QStyleOptionButton::Flat
                ? QQStyleKitReader::ControlType::FlatButton
                : QQStyleKitReader::ControlType::Button;
            const auto resolved = d->resolveLayout(controlType, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            QRect rect = opt->rect.marginsRemoved(metrics.margins + metrics.padding + metrics.textPadding);
            return visualRect(opt->direction, opt->rect, rect);
        }
        break;
    case SE_CheckBoxContents:
    case SE_RadioButtonContents: {
        const auto controlType = r == SE_CheckBoxContents ? QQStyleKitReader::ControlType::CheckBox
                                                            : QQStyleKitReader::ControlType::RadioButton;
        const auto resolved = d->resolveLayout(controlType, opt->state);
        if (!resolved.isValid())
            break;
        const auto &metrics = *resolved.metrics;
        QRect contentsRect = opt->rect.marginsRemoved(metrics.margins + metrics.padding);
        const auto subElement = r == SE_CheckBoxContents ? SE_CheckBoxIndicator : SE_RadioButtonIndicator;
        QRect indicatorRect = visualRect(opt->direction, opt->rect, subElementRect(subElement, opt, widget));
        const int spacing = metrics.spacing;
        const auto *indicator = resolved.indicator();
        const uint alignment = indicator
            ? resolvedAlignment(indicator->alignment(), Qt::AlignLeft, Qt::AlignVCenter)
            : uint(Qt::AlignLeft | Qt::AlignVCenter);
        if (alignment & Qt::AlignLeft) {
            contentsRect.setLeft(indicatorRect.right() + spacing + metrics.textPadding.left());
        } else if (alignment & Qt::AlignHCenter) {
            contentsRect.setLeft(indicatorRect.right() + spacing + metrics.textPadding.left());
            contentsRect.setRight(indicatorRect.left() - spacing - metrics.textPadding.right());
        } else {
            contentsRect.setRight(indicatorRect.left() - spacing - metrics.textPadding.right());
        }
        return visualRect(opt->direction, opt->rect, contentsRect);
    }
    case SE_CheckBoxIndicator:
    case SE_RadioButtonIndicator: {
        const auto controlType = r == SE_CheckBoxIndicator
            ? QQStyleKitReader::ControlType::CheckBox
            : QQStyleKitReader::ControlType::RadioButton;
        const auto resolved = d->resolveLayout(controlType, opt->state);
        if (!resolved.isValid())
            break;
        const auto &metrics = *resolved.metrics;
        QRect rect = opt->rect.marginsRemoved(metrics.margins);
        const auto *indicator = resolved.indicator();
        if (!indicator || !indicator->visible() || indicator->opacity() == 0)
            return rect;

        const int w = qSaturateRound(resolvedWidth(indicator,
            rect.width() - metrics.padding.left() - metrics.padding.right()
                - metrics.indicatorMargins.left() - metrics.indicatorMargins.right()));
        const int h = qSaturateRound(resolvedHeight(indicator,
            rect.height() - metrics.padding.top() - metrics.padding.bottom()
                - metrics.indicatorMargins.top() - metrics.indicatorMargins.bottom()));
        const uint alignment = resolvedAlignment(indicator->alignment(), Qt::AlignLeft, Qt::AlignVCenter);
        const QRect indicatorRect = d->getAlignedRectInContainer(
            rect, QSize(w, h), alignment, metrics.padding, metrics.indicatorMargins);
        return visualRect(opt->direction, rect, indicatorRect);
    }
#if QT_CONFIG(itemviews)
    case SE_ItemViewItemCheckIndicator: {
        const auto resolved = d->resolveLayout(itemViewControlType(opt), opt->state);
        if (!resolved.isValid())
            break;
        const auto &metrics = *resolved.metrics;
        QRect rect = opt->rect.marginsRemoved(metrics.margins);
        const auto *indicator = resolved.indicator();
        if (!indicator || !indicator->visible() || indicator->opacity() == 0)
            return rect;

        const int w = qSaturateRound(resolvedWidth(indicator,
            rect.width() - metrics.padding.left() - metrics.padding.right()
                - metrics.indicatorMargins.left() - metrics.indicatorMargins.right()));
        const int h = qSaturateRound(resolvedHeight(indicator,
            rect.height() - metrics.padding.top() - metrics.padding.bottom()
                - metrics.indicatorMargins.top() - metrics.indicatorMargins.bottom()));
        const uint alignment = resolvedAlignment(indicator->alignment(), Qt::AlignLeft, Qt::AlignVCenter);
        const QRect indicatorRect = d->getAlignedRectInContainer(
            rect, QSize(w, h), alignment, metrics.padding, metrics.indicatorMargins);
        return visualRect(opt->direction, rect, indicatorRect);
    }
    case SE_ItemViewItemDecoration:
        if (const auto *itemViewOption = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            if (!(itemViewOption->features & QStyleOptionViewItem::HasDecoration))
                return QRect();
            const auto resolved = d->resolveLayout(itemViewControlType(opt), opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            QRect contentsRect = opt->rect.marginsRemoved(metrics.margins + metrics.padding);
            if (itemViewOption->features & QStyleOptionViewItem::HasCheckIndicator) {
                const QRect indicatorRect = subElementRect(
                    SE_ItemViewItemCheckIndicator, opt, widget);
                const auto *checkIndicator = resolved.indicator();
                const uint checkAlign = checkIndicator
                    ? resolvedAlignment(checkIndicator->alignment(), Qt::AlignLeft,
                                        Qt::AlignVCenter)
                    : uint(Qt::AlignLeft | Qt::AlignVCenter);
                if (checkAlign & Qt::AlignLeft) {
                    contentsRect.setLeft(indicatorRect.right()
                        + metrics.indicatorMargins.right() + metrics.spacing);
                } else if (checkAlign & Qt::AlignRight) {
                    contentsRect.setRight(indicatorRect.left()
                        - metrics.indicatorMargins.left() - metrics.spacing);
                }
            }

            const QSize decorationSize = itemViewOption->decorationSize
                .boundedTo(contentsRect.size());
            QRect decorationRect;
            switch (itemViewOption->decorationPosition) {
            case QStyleOptionViewItem::Top:
                decorationRect = QRect(contentsRect.left(), contentsRect.top(),
                                       contentsRect.width(), decorationSize.height());
                break;
            case QStyleOptionViewItem::Bottom:
                decorationRect = QRect(contentsRect.left(),
                                       contentsRect.bottom() - decorationSize.height() + 1,
                                       contentsRect.width(), decorationSize.height());
                break;
            case QStyleOptionViewItem::Right:
                decorationRect = QRect(contentsRect.right() - decorationSize.width() + 1,
                                       contentsRect.top(), decorationSize.width(),
                                       contentsRect.height());
                break;
            case QStyleOptionViewItem::Left:
            default:
                decorationRect = QRect(contentsRect.left(), contentsRect.top(),
                                       decorationSize.width(), contentsRect.height());
                break;
            }
            const uint decorationAlign = resolvedAlignment(itemViewOption->decorationAlignment,
                                                           Qt::AlignLeft, Qt::AlignVCenter);
            decorationRect = d->getAlignedRectInContainer(
                    decorationRect, decorationSize, decorationAlign, QMargins(0, 0, 0, 0),
                    QMargins(0, 0, 0, 0));
            return visualRect(opt->direction, opt->rect, decorationRect);
        }
        break;
    case SE_ItemViewItemText:
        if (const auto *itemViewOption = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            const auto resolved = d->resolveLayout(itemViewControlType(opt), opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            QRect contentsRect = opt->rect.marginsRemoved(metrics.margins + metrics.padding);
            const int spacing = metrics.spacing;

            if (itemViewOption->features & QStyleOptionViewItem::HasCheckIndicator) {
                const QRect indicatorRect = subElementRect(
                    SE_ItemViewItemCheckIndicator, opt, widget);
                const auto *checkIndicator = resolved.indicator();
                const uint checkAlign = checkIndicator
                    ? resolvedAlignment(checkIndicator->alignment(), Qt::AlignLeft,
                                        Qt::AlignVCenter)
                    : uint(Qt::AlignLeft | Qt::AlignVCenter);
                if (checkAlign & Qt::AlignLeft) {
                    contentsRect.setLeft(
                        indicatorRect.right() + metrics.indicatorMargins.right() + spacing);
                } else if (checkAlign & Qt::AlignRight) {
                    contentsRect.setRight(
                        indicatorRect.left() - metrics.indicatorMargins.left() - spacing);
                }
            }

            QRect textRect = contentsRect;
            if (itemViewOption->features & QStyleOptionViewItem::HasDecoration) {
                const QRect decorationRect = subElementRect(
                    SE_ItemViewItemDecoration, opt, widget);
                switch (itemViewOption->decorationPosition) {
                case QStyleOptionViewItem::Top:
                    textRect.setTop(decorationRect.bottom() + spacing);
                    break;
                case QStyleOptionViewItem::Bottom:
                    textRect.setBottom(decorationRect.top() - spacing);
                    break;
                case QStyleOptionViewItem::Right:
                    textRect.setRight(decorationRect.left() - spacing);
                    break;
                case QStyleOptionViewItem::Left:
                default:
                    textRect.setLeft(decorationRect.right() + spacing);
                    break;
                }
            }

            const auto *textProps = resolved.text();
            const uint textAlign = textProps
                ? resolvedAlignment(textProps->alignment(), Qt::AlignLeft, Qt::AlignVCenter)
                : uint(Qt::AlignLeft | Qt::AlignVCenter);
        if (textAlign & Qt::AlignLeft) {
            textRect.setLeft(textRect.left() + metrics.textPadding.left());
        } else if (textAlign & Qt::AlignHCenter) {
            textRect.setLeft(textRect.left() + metrics.textPadding.left() / 2);
            textRect.setRight(textRect.right() - metrics.textPadding.right() / 2);
        } else {
            textRect.setRight(textRect.right() - metrics.textPadding.right());
        }
        if (textAlign & Qt::AlignTop) {
            textRect.setTop(textRect.top() + metrics.textPadding.top());
        } else if (textAlign & Qt::AlignVCenter) {
            textRect.setTop(textRect.top() + metrics.textPadding.top() / 2);
            textRect.setBottom(textRect.bottom() - metrics.textPadding.bottom() / 2);
        } else {
            textRect.setBottom(textRect.bottom() - metrics.textPadding.bottom());
        }
        return visualRect(opt->direction, opt->rect, textRect);
    }
    break;
#endif // QT_CONFIG(itemviews)
    case SE_PushButtonFocusRect:
    case SE_CheckBoxClickRect:
    case SE_RadioButtonClickRect:
        return opt->rect;
#if QT_CONFIG(progressbar)
    case SE_ProgressBarGroove: {
        const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ProgressBar, opt->state);
        if (!resolved.isValid())
            break;
        const auto &metrics = *resolved.metrics;
        QRect rect = opt->rect.marginsRemoved(metrics.margins);
        const auto *indicator = resolved.indicator();
        if (!indicator || !indicator->visible() || indicator->opacity() == 0)
            return rect;

        const int w = qSaturateRound(resolvedWidth(indicator,
            rect.width() - metrics.padding.left() - metrics.padding.right()
                - metrics.indicatorMargins.left() - metrics.indicatorMargins.right()));
        const int h = qSaturateRound(resolvedHeight(indicator,
            rect.height() - metrics.padding.top() - metrics.padding.bottom()
                - metrics.indicatorMargins.top() - metrics.indicatorMargins.bottom()));
        const uint alignment = resolvedAlignment(indicator->alignment(), Qt::AlignLeft, Qt::AlignVCenter);
        const QRect indicatorRect = d->getAlignedRectInContainer(
            rect, QSize(w, h), alignment, metrics.padding, metrics.indicatorMargins);
        return visualRect(opt->direction, rect, indicatorRect);
    }
    case SE_ProgressBarContents: {
        if (qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ProgressBar, opt->state);
            if (!resolved.isValid())
                break;
            const auto *foreground = resolved.indicator() ? resolved.indicator()->foreground() : nullptr;
            if (!foreground || !foreground->visible() || foreground->opacity() == 0)
                return QRect();

            QRect indicatorRect = subElementRect(SE_ProgressBarGroove, opt, widget);
            const int foregroundW = qSaturateRound(resolvedWidth(foreground,
                indicatorRect.width() - foreground->leftMargin() - foreground->rightMargin()));
            const int foregroundH = qSaturateRound(resolvedHeight(foreground,
                indicatorRect.height() - foreground->topMargin() - foreground->bottomMargin()));
            const uint foregroundAlign = resolvedAlignment(foreground->alignment(), Qt::AlignLeft, Qt::AlignVCenter);
            const QMargins foregroundMargins = elementMargins(foreground);
            const QRect foregroundRect = d->getAlignedRectInContainer(
                indicatorRect, QSize(foregroundW, foregroundH), foregroundAlign, QMargins(0, 0, 0, 0), foregroundMargins);
            return visualRect(opt->direction, opt->rect, foregroundRect);
        }
        break;
    }
#endif // QT_CONFIG(progressbar)
    case SE_LineEditContents:
        if (qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
#if QT_CONFIG(spinbox)
            const bool isInSpinBox = widget && qobject_cast<const QSpinBox *>(widget->parentWidget());
#else
            const bool isInSpinBox = false;
#endif
            const auto controlType = isInSpinBox ? QQStyleKitReader::ControlType::SpinBox : QQStyleKitReader::ControlType::TextField;
            const auto resolved = d->resolveLayout(controlType, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            QRect contentsRect = opt->rect.marginsRemoved(metrics.padding);
            const auto *textProps = resolved.text();
            const uint textAlign = resolvedAlignment(
                textProps ? textProps->alignment() : 0u, Qt::AlignLeft, Qt::AlignVCenter);
            if (textAlign & Qt::AlignLeft)
                contentsRect.setLeft(contentsRect.left() + metrics.textPadding.left());
            else if (textAlign & Qt::AlignHCenter) {
                contentsRect.setLeft(contentsRect.left() + metrics.textPadding.left() / 2);
                contentsRect.setRight(contentsRect.right() - metrics.textPadding.right() / 2);
            } else {
                contentsRect.setRight(contentsRect.right() - metrics.textPadding.right());
            }
            if (textAlign & Qt::AlignTop)
                contentsRect.setTop(contentsRect.top() + metrics.textPadding.top());
            else if (textAlign & Qt::AlignVCenter) {
                contentsRect.setTop(contentsRect.top() + metrics.textPadding.top() / 2);
                contentsRect.setBottom(contentsRect.bottom() - metrics.textPadding.bottom() / 2);
            } else {
                contentsRect.setBottom(contentsRect.bottom() - metrics.textPadding.bottom());
            }
            return visualRect(opt->direction, opt->rect, contentsRect);
        }
        break;
    case SE_ShapedFrameContents:
#if QT_CONFIG(combobox)
        if (widget && widget->inherits("QComboBoxPrivateContainer")) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::Popup, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            QRect contentsRect = opt->rect.marginsRemoved(metrics.margins + metrics.padding + metrics.textPadding);
            return visualRect(opt->direction, opt->rect, contentsRect);
        }
#endif
#if QT_CONFIG(textedit)
        if (qobject_cast<const QTextEdit *>(widget)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::TextArea, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            QRect contentsRect = opt->rect.marginsRemoved(metrics.margins + metrics.padding + metrics.textPadding);
            return visualRect(opt->direction, opt->rect, contentsRect);
        }
#endif // textedit
        break;
#if QT_CONFIG(tabbar)
    case SE_TabBarTabText:
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::TabButton, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            QRect rect = tab->rect.marginsRemoved(metrics.margins + metrics.padding + metrics.textPadding);
            if (!tab->icon.isNull())
                rect.setLeft(rect.left() + tab->iconSize.width() + metrics.spacing);
            return visualRect(tab->direction, tab->rect, rect);
        }
        break;
    case SE_TabBarTearIndicatorRight:
    case SE_TabBarTearIndicatorLeft:
    case SE_TabBarScrollLeftButton:
    case SE_TabBarScrollRightButton:
        // Not supported
        return QRect();
#endif // QT_CONFIG(tabbar)
#if QT_CONFIG(tabwidget)
    case SE_TabWidgetTabContents:
        if (qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
            // TabWidget corresponds to Page
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::Page, opt->state);
            if (!resolved.isValid())
                break;
            const QRect paneRect = subElementRect(SE_TabWidgetTabPane, opt, widget);
            const QRect rect = paneRect.marginsRemoved(resolved.metrics->padding);
            return visualRect(opt->direction, opt->rect, rect);
        }
        break;
#endif // QT_CONFIG(tabwidget)
#if QT_CONFIG(toolbar)
    case SE_ToolBarHandle:
        // Not supported
        return QRect();
#endif // QT_CONFIG(toolbar)
    default:
        break;
    }
    return QCommonStyle::subElementRect(r, opt, widget);
}

/*! \reimp */
void QStyleKitStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
    const QWidget *w) const
{
    Q_D(const QStyleKitStyle);

    switch (cc) {
#if QT_CONFIG(slider)
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::Slider, slider->state);
            if (!r.isValid())
                break;
            const auto &metrics = *r.metrics;
            QRect backgroundRect = opt->rect.marginsRemoved(metrics.margins);

            // background
            d->drawStyledItemRect(r.background(), backgroundRect, p);

            // groove
            if (slider->subControls & SC_SliderGroove) {
                // groove
                const auto grooveRect = proxy()->subControlRect(CC_Slider, opt, SC_SliderGroove, w);
                const auto *indicator = r.indicator();
                d->drawStyledItemRect(indicator, grooveRect, p);

                // track
                const auto *foreground = indicator ? indicator->foreground() : nullptr;
                if (foreground && foreground->visible() && foreground->opacity() > 0) {
                    const bool isHorizontal = slider->orientation == Qt::Horizontal;
                    const qreal availableW = grooveRect.width()  - foreground->leftMargin() - foreground->rightMargin();
                    const qreal availableH = grooveRect.height() - foreground->topMargin()  - foreground->bottomMargin();

                    const qreal range = slider->maximum - slider->minimum;
                    const qreal ratio = range > 0 ? (slider->sliderPosition - slider->minimum) / range : 0;

                    const auto hAlign = foreground->alignment() & Qt::AlignHorizontal_Mask;
                    const auto vAlign = foreground->alignment() & Qt::AlignVertical_Mask;
                    const qreal fgW = resolvedWidth(foreground, availableW);
                    const qreal fgH = resolvedHeight(foreground, availableH);
                    const qreal fgX = hAlign & Qt::AlignRight
                        ? grooveRect.left() + foreground->leftMargin() + availableW - fgW
                        : hAlign & Qt::AlignHCenter
                        ? grooveRect.left() + foreground->leftMargin() + (availableW - fgW) / 2
                        : grooveRect.left() + foreground->leftMargin();
                    const qreal fgY = vAlign & Qt::AlignBottom
                        ? grooveRect.top() + foreground->topMargin() + availableH - fgH
                        : vAlign & Qt::AlignVCenter
                        ? grooveRect.top() + foreground->topMargin() + (availableH - fgH) / 2
                        : grooveRect.top() + foreground->topMargin();

                    const qreal minW = foreground->minimumWidth();
                    QRectF trackRect;
                    if (isHorizontal) {
                        const qreal trackW = foreground->fillWidth()
                            ? minW + ratio * (fgW - minW)
                            : ratio * fgW;
                        trackRect = QRectF(fgX, fgY, trackW, fgH);
                    } else {
                        const qreal trackH = foreground->fillHeight()
                            ? minW + ratio * (fgH - minW)
                            : ratio * fgH;
                        const qreal trackY = fgY + (1.0 - ratio) * (foreground->fillHeight() ? fgH - minW : fgH);
                        trackRect = QRectF(fgX, trackY, fgW, trackH);
                    }
                    // The track is the groove's foreground child, so it inherits the
                    // indicator's transform.
                    const QRect foregroundBox = visualRect(
                        opt->direction, grooveRect, QRectF(fgX, fgY, fgW, fgH).toAlignedRect());
                    QPainterStateGuard stateGuard(p);
                    if (applyDelegateTransform(p, indicator, grooveRect)
                        && applyDelegateTransform(p, foreground, foregroundBox)) {
                        d->drawStyledItemContents(
                            foreground,
                            visualRect(opt->direction, grooveRect, trackRect.toAlignedRect()), p);
                    }
                }
            }

            // handle
            if (slider->subControls & SC_SliderHandle) {
                QStyleOptionSlider handleOpt(*slider);
                handleOpt.rect = subControlRect(CC_Slider, opt, SC_SliderHandle, w);
                const auto *handle = r.handle();
                if (handle && handle->visible() && handle->opacity() > 0)
                    d->drawStyledItemRect(handle, handleOpt.rect, p);
            }
            return;
        }
        break;
#endif // QT_CONFIG(slider)
#if QT_CONFIG(combobox)
    case CC_ComboBox:
        if (const QStyleOptionComboBox *combo = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            // background
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::ComboBox, combo->state);
            if (!r.isValid())
                break;
            QRect frameRect = subControlRect(CC_ComboBox, opt, SC_ComboBoxFrame, w);
            d->drawStyledItemRect(r.background(), frameRect, p);

            // indicator
            if (combo->subControls & SC_ComboBoxArrow) {
                QStyleOptionComboBox indicatorOpt(*combo);
                indicatorOpt.rect = subControlRect(CC_ComboBox, opt, SC_ComboBoxArrow, w);
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &indicatorOpt, p, w);
            }

            // The editable combobox paints its own line edit using the QPalette::Text role
            // for the text color, so update that color in the palette
            if (auto *cb = qobject_cast<const QComboBox *>(w); cb && cb->isEditable()) {
                if (const auto *txt = r.text(); txt && txt->isDefined(QQSK::Property::Color)) {
                    QPalette stylePalette;
                    stylePalette.setColor(QPalette::Text, txt->color());
                    d->setStylePalette(cb->lineEdit(), stylePalette);
                }
            }
            return;
        }
        break;
#endif // QT_CONFIG(combobox)
#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spin = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::SpinBox, spin->state);
            if (!r.isValid())
                break;
            QRect frameRect = opt->rect.marginsRemoved(r.metrics->margins);
            // background
            d->drawStyledItemRect(r.background(), frameRect, p);
            // up/down buttons
            if (spin->subControls & SC_SpinBoxUp) {
                QStyleOptionSpinBox upOpt(*spin);
                upOpt.rect = subControlRect(CC_SpinBox, opt, SC_SpinBoxUp, w);
                proxy()->drawPrimitive(PE_IndicatorSpinUp, &upOpt, p, w);
            }
            if (spin->subControls & SC_SpinBoxDown) {
                QStyleOptionSpinBox downOpt(*spin);
                downOpt.rect = subControlRect(CC_SpinBox, opt, SC_SpinBoxDown, w);
                proxy()->drawPrimitive(PE_IndicatorSpinDown, &downOpt, p, w);
            }
            // The spinbox line edit paints its text using the QPalette::Text role,
            // so update that color in the palette
            if (auto *sb = qobject_cast<const QSpinBox *>(w)) {
                if (const auto *txt = r.text(); txt && txt->isDefined(QQSK::Property::Color)) {
                    QLineEdit *lineEdit = sb->findChild<QLineEdit *>();
                    if (lineEdit) {
                        QPalette p = lineEdit->palette();
                        if (p.color(QPalette::Text) != txt->color()) {
                            p.setColor(QPalette::Text, txt->color());
                            lineEdit->setPalette(p);
                        }
                    }
                }
            }
            return;
        }
        break;
#endif // QT_CONFIG(spinbox)
#if QT_CONFIG(groupbox)
    case CC_GroupBox:
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::GroupBox, groupBox->state);
            if (!r.isValid())
                break;

            // background/frame
            if (groupBox->subControls & SC_GroupBoxFrame) {
                QStyleOptionFrame frameOpt;
                frameOpt.QStyleOption::operator=(*groupBox);
                frameOpt.rect = subControlRect(CC_GroupBox, opt, SC_GroupBoxFrame, w);
                drawPrimitive(PE_FrameGroupBox, &frameOpt, p, w);
            }

            // title
            if (groupBox->subControls & SC_GroupBoxLabel && !groupBox->text.isEmpty()) {
                const auto *textProps = r.text();
                const QFont textFont = r.font();
                QRect titleRect = subControlRect(CC_GroupBox, opt, SC_GroupBoxLabel, w);
                d->drawControlText(textProps, textFont, titleRect.marginsRemoved(r.metrics->textPadding),
                                   groupBox->text, Qt::TextShowMnemonic, p, Qt::AlignLeft | Qt::AlignVCenter);
            }
            // don't draw checkmark as the Controls style doesn't draw it
            // and StyleKit doesn't provide a way to style it
            return;
        }
        break;
#endif // QT_CONFIG(groupbox)
#if QT_CONFIG(scrollbar)
    case CC_ScrollBar:
        if (const auto *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::ScrollBar, scrollbar->state);
            if (!r.isValid())
                break;

            // background = groove
            if (scrollbar->subControls & SC_ScrollBarGroove) {
                QRect backgroundRect = proxy()->subControlRect(CC_ScrollBar, opt, SC_ScrollBarGroove, w);
                d->drawStyledItemRect(r.background(), backgroundRect, p);
            }

            // TODO: increase button
            // TODO: decrease button

            // slider = indicator
            if (scrollbar->subControls & SC_ScrollBarSlider) {
                QStyleOptionSlider newScrollbar(*scrollbar);
                newScrollbar.rect = proxy()->subControlRect(CC_ScrollBar, opt, SC_ScrollBarSlider, w);
                proxy()->drawControl(CE_ScrollBarSlider, &newScrollbar, p, w);
            }
            return;
        }
        break;
#endif // QT_CONFIG(scrollbar)
#if QT_CONFIG(toolbutton)
    case CC_ToolButton:
        if (const QStyleOptionToolButton *tool = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::ToolButton, tool->state);
            if (!r.isValid())
                break;

            // background
            d->drawStyledItemRect(r.background(), opt->rect, p);

            // menu arrow indicator
            if ((tool->features & (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::PopupDelay))
                    == QStyleOptionToolButton::MenuButtonPopup
                && (tool->subControls & SC_ToolButtonMenu)) {
                const QRect menuRect = subControlRect(CC_ToolButton, opt, SC_ToolButtonMenu, w);
                d->drawControlIndicator(r.indicator(), menuRect, p);
            }

            // label
            State bflags = tool->state & ~State_Sunken;
            if (bflags & State_AutoRaise) {
                if (!(bflags & State_MouseOver) || !(bflags & State_Enabled))
                    bflags &= ~State_Raised;
            }
            if (tool->state & State_Sunken && tool->activeSubControls & SC_ToolButton)
                bflags |= State_Sunken;
            QStyleOptionToolButton label = *tool;
            label.state = bflags;
            label.rect = subControlRect(CC_ToolButton, opt, SC_ToolButton, w);
            proxy()->drawControl(CE_ToolButtonLabel, &label, p, w);
            return;
        }
        break;
#endif // QT_CONFIG(toolbutton)
    default:
        break;
    }
    QCommonStyle::drawComplexControl(cc, opt, p, w);
}

/*! \reimp */
QStyle::SubControl QStyleKitStyle::hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
    const QPoint &pt, const QWidget *w) const
{
    return QCommonStyle::hitTestComplexControl(cc, opt, pt, w);
}

/*! \reimp */
QRect QStyleKitStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
    const QWidget *w) const
{
    using QtPrivate::qSaturateRound;
    Q_D(const QStyleKitStyle);

    switch (cc) {
#if QT_CONFIG(slider)
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::Slider, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            const bool horizontal = slider->orientation == Qt::Horizontal;
            QRect contentsRect = opt->rect.marginsRemoved(metrics.margins).marginsRemoved(metrics.padding);

            const auto *indicator = resolved.indicator();
            if (!indicator)
                return contentsRect;
            switch (sc) {
            case SC_SliderGroove: {
                const auto availableW = contentsRect.width()  - indicator->leftMargin() - indicator->rightMargin();
                const auto availableH = contentsRect.height() - indicator->topMargin()  - indicator->bottomMargin();
                const qreal grooveW = resolvedWidth(indicator, availableW);
                const qreal grooveH = resolvedHeight(indicator, availableH);

                QRectF grooveRect(0, 0, grooveW, grooveH);
                const uint rawHAlign = indicator->alignment() & Qt::AlignHorizontal_Mask;
                const uint rawVAlign = indicator->alignment() & Qt::AlignVertical_Mask;
                const uint hAlign = rawHAlign ? static_cast<Qt::Alignment>(rawHAlign) : Qt::AlignLeft;
                const uint vAlign = rawVAlign ? static_cast<Qt::Alignment>(rawVAlign) : Qt::AlignVCenter;

                if (hAlign & Qt::AlignLeft) {
                    grooveRect.moveLeft(contentsRect.x() + indicator->leftMargin());
                } else if (hAlign & Qt::AlignHCenter) {
                    const qreal availableWidth = contentsRect.width()
                                                - indicator->leftMargin()
                                                - indicator->rightMargin();
                    grooveRect.moveLeft(contentsRect.x() + indicator->leftMargin()
                                        + (availableWidth - grooveW) / 2.0);
                } else {
                    grooveRect.moveLeft(contentsRect.x() + contentsRect.width()
                                        - indicator->rightMargin() - grooveW);
                }

                if (vAlign & Qt::AlignTop) {
                    grooveRect.moveTop(contentsRect.y() + indicator->topMargin());
                } else if (vAlign & Qt::AlignVCenter) {
                    const qreal availableHeight = contentsRect.height()
                                                - indicator->topMargin()
                                                - indicator->bottomMargin();
                    grooveRect.moveTop(contentsRect.y() + indicator->topMargin()
                                        + (availableHeight - grooveH) / 2.0);
                } else {
                    grooveRect.moveTop(contentsRect.y() + contentsRect.height()
                                        - indicator->bottomMargin() - grooveH);
                }
                return visualRect(opt->direction, opt->rect, grooveRect.toAlignedRect());
            }
            case SC_SliderHandle: {
                const auto *handle = resolved.handle();
                if (!handle || !handle->visible() || handle->opacity() == 0)
                    return contentsRect;

                const qreal handleW = resolvedWidth(handle,
                    contentsRect.width() - handle->leftMargin() - handle->rightMargin());
                const qreal handleH = resolvedHeight(handle,
                    contentsRect.height() - handle->topMargin() - handle->bottomMargin());
                QRectF handleRect(0, 0, handleW, handleH);
                if (horizontal) {
                    const int range = qSaturateRound(contentsRect.width()
                        - handle->leftMargin() - handle->rightMargin() - handleW);
                    const int sliderPos = QStyle::sliderPositionFromValue(
                        slider->minimum, slider->maximum, slider->sliderPosition, range, false);
                    handleRect.moveLeft(contentsRect.x() + handle->leftMargin() + sliderPos);
                    handleRect.moveTop(contentsRect.y() + handle->topMargin() - handle->bottomMargin()
                        + (contentsRect.height() - handleH) / 2.0);
                } else {
                    const int range = qSaturateRound(contentsRect.height()
                        - handle->topMargin() - handle->bottomMargin() - handleH);
                    const int sliderPos = QStyle::sliderPositionFromValue(
                        slider->minimum, slider->maximum, slider->sliderPosition, range, true);
                    handleRect.moveLeft(contentsRect.x() + handle->leftMargin() - handle->rightMargin()
                        + (contentsRect.width() - handleW) / 2.0);
                    handleRect.moveTop(contentsRect.y() + handle->topMargin() + sliderPos);
                }
                return visualRect(opt->direction, opt->rect, handleRect.toAlignedRect());
            }
            default:
                break;
            }
        }
        break;
#endif // QT_CONFIG(slider)
#if QT_CONFIG(combobox)
    case CC_ComboBox:
        if (const QStyleOptionComboBox *combo = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            const auto r = d->resolveLayout(QQStyleKitReader::ControlType::ComboBox, combo->state);
            if (!r.isValid())
                break;
            const auto &metrics = *r.metrics;
            QRect frameRect = opt->rect.marginsRemoved(metrics.margins);
            switch (sc) {
            case SC_ComboBoxFrame:
                return visualRect(opt->direction, opt->rect, frameRect);
            case SC_ComboBoxEditField: {
                QRect contentsRect = frameRect.marginsRemoved(metrics.padding);
                QRect indicatorRect = subControlRect(CC_ComboBox, opt, SC_ComboBoxArrow, w);
                const int spacing = metrics.spacing;
                const auto *indicator = r.indicator();
                const uint indicatorAlign = indicator
                    ? resolvedAlignment(indicator->alignment(), Qt::AlignRight, Qt::AlignVCenter)
                    : uint(Qt::AlignRight | Qt::AlignVCenter);
                if (indicatorAlign & Qt::AlignLeft) {
                    contentsRect.setLeft(indicatorRect.right() + spacing);
                } else if (indicatorAlign & Qt::AlignRight) {
                    contentsRect.setRight(indicatorRect.left() - spacing);
                }
                return visualRect(opt->direction, opt->rect, contentsRect);
            }
            case SC_ComboBoxArrow: {
                QRect contentsRect = frameRect.marginsRemoved(metrics.padding);
                const auto *indicator = r.indicator();
                if (!indicator || !indicator->visible() || indicator->opacity() == 0)
                    return QRect();

                const int w = qSaturateRound(resolvedWidth(indicator,
                    contentsRect.width() - indicator->leftMargin() - indicator->rightMargin()));
                const int h = qSaturateRound(resolvedHeight(indicator,
                    contentsRect.height() - indicator->topMargin() - indicator->bottomMargin()));
                const uint indicatorAlign = resolvedAlignment(indicator->alignment(), Qt::AlignRight, Qt::AlignVCenter);
                const QMargins indicatorMargins = elementMargins(indicator);
                return visualRect(opt->direction, opt->rect,
                    d->getAlignedRectInContainer(contentsRect, QSize(w, h), indicatorAlign, QMargins(), indicatorMargins));
            }
            case SC_ComboBoxListBoxPopup: {
                QRect popupRect = opt->rect;
                popupRect.setTop(opt->rect.bottom());
                return visualRect(opt->direction, opt->rect, popupRect);
            }
            default:
                break;
            }
        }
        break;
#endif // QT_CONFIG(combobox)
#if QT_CONFIG(spinbox)
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(opt)) {
            const auto r = d->resolveLayout(QQStyleKitReader::ControlType::SpinBox, spinBox->state);
            if (!r.isValid())
                break;
            const auto &metrics = *r.metrics;
            QRect frameRect = opt->rect.marginsRemoved(metrics.margins);
            QRect contentsRect = frameRect.marginsRemoved(metrics.padding);
            switch (sc) {
            case SC_SpinBoxFrame:
                return visualRect(opt->direction, opt->rect, frameRect);
            case SC_SpinBoxEditField: {
                QRect upIndicatorRect = subControlRect(CC_SpinBox, opt, SC_SpinBoxUp, w);
                QRect downIndicatorRect = subControlRect(CC_SpinBox, opt, SC_SpinBoxDown, w);
                const int spacing = metrics.spacing;
                const auto *upIndicator = r.indicator() ? r.indicator()->first() : nullptr;
                const auto *downIndicator = r.indicator() ? r.indicator()->second() : nullptr;
                const bool hasUpIndicator = upIndicator && upIndicator->visible() && upIndicator->opacity() > 0;
                const bool hasDownIndicator = downIndicator && downIndicator->visible() && downIndicator->opacity() > 0;
                const uint upAlign = hasUpIndicator
                    ? resolvedAlignment(upIndicator->alignment(), Qt::AlignLeft, Qt::AlignVCenter)
                    : uint(Qt::AlignLeft | Qt::AlignVCenter);
                const uint downAlign = hasDownIndicator
                    ? resolvedAlignment(downIndicator->alignment(), Qt::AlignLeft, Qt::AlignVCenter)
                    : uint(Qt::AlignLeft | Qt::AlignVCenter);
                if (hasUpIndicator && hasDownIndicator) {
                    if ((upAlign & Qt::AlignLeft) && (downAlign & Qt::AlignRight)) {
                        contentsRect.setLeft(upIndicatorRect.right() + spacing);
                        contentsRect.setRight(downIndicatorRect.left() - spacing);
                    } else if ((upAlign & Qt::AlignRight) && (downAlign & Qt::AlignLeft)) {
                        contentsRect.setRight(upIndicatorRect.left() - spacing);
                        contentsRect.setLeft(downIndicatorRect.right() + spacing);
                    }
                } else if (hasUpIndicator) {
                    if (upAlign & Qt::AlignLeft) {
                        contentsRect.setLeft(upIndicatorRect.right() + spacing);
                    } else if (upAlign & Qt::AlignRight) {
                        contentsRect.setRight(upIndicatorRect.left() - spacing);
                    }
                } else if (hasDownIndicator) {
                    if (downAlign & Qt::AlignLeft) {
                        contentsRect.setLeft(downIndicatorRect.right() + spacing);
                    } else if (downAlign & Qt::AlignRight) {
                        contentsRect.setRight(downIndicatorRect.left() - spacing);
                    }
                }
                return visualRect(opt->direction, opt->rect, contentsRect);
            }
            case SC_SpinBoxUp: {
                const auto *upIndicator = r.indicator() ? r.indicator()->first() : nullptr;
                if (!upIndicator || !upIndicator->visible() || upIndicator->opacity() == 0)
                    return contentsRect;

                const int w = qSaturateRound(resolvedWidth(upIndicator,
                    contentsRect.width() - upIndicator->leftMargin() - upIndicator->rightMargin()));
                const int h = qSaturateRound(resolvedHeight(upIndicator,
                    contentsRect.height() - upIndicator->topMargin() - upIndicator->bottomMargin()));
                const uint upAlign = resolvedAlignment(upIndicator->alignment(), Qt::AlignLeft, Qt::AlignVCenter);
                const QMargins upMargins = elementMargins(upIndicator);
                return visualRect(opt->direction, opt->rect,
                    d->getAlignedRectInContainer(contentsRect, QSize(w, h), upAlign, QMargins(), upMargins));
            }
            case SC_SpinBoxDown: {
                const auto *downIndicator = r.indicator() ? r.indicator()->second() : nullptr;
                if (!downIndicator || !downIndicator->visible() || downIndicator->opacity() == 0)
                    return frameRect;

                const int w = qSaturateRound(resolvedWidth(downIndicator,
                    contentsRect.width() - downIndicator->leftMargin() - downIndicator->rightMargin()));
                const int h = qSaturateRound(resolvedHeight(downIndicator,
                    contentsRect.height() - downIndicator->topMargin() - downIndicator->bottomMargin()));
                const uint downAlign = resolvedAlignment(downIndicator->alignment(), Qt::AlignLeft, Qt::AlignVCenter);
                const QMargins downMargins = elementMargins(downIndicator);
                return visualRect(opt->direction, opt->rect,
                    d->getAlignedRectInContainer(contentsRect, QSize(w, h), downAlign, QMargins(), downMargins));
            }
            default:
                break;
            }
        }
        break;
#endif // QT_CONFIG(spinbox)
#if QT_CONFIG(groupbox)
    case CC_GroupBox:
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {
            const auto r = d->resolveLayout(QQStyleKitReader::ControlType::GroupBox, groupBox->state);
            if (!r.isValid())
                break;
            const auto &metrics = *r.metrics;
            QRect frameRect = opt->rect.marginsRemoved(metrics.margins);
            switch (sc) {
            case SC_GroupBoxFrame:
                return visualRect(opt->direction, opt->rect, frameRect);
            case SC_GroupBoxLabel: {
                const int fontHeight = opt->fontMetrics.height();
                const int labelHeight = metrics.textPadding.top() + fontHeight
                                        + metrics.textPadding.bottom();
                const QRect labelContainer(opt->rect.left(), opt->rect.top(),
                                            opt->rect.width(),
                                            labelHeight);
                return visualRect(opt->direction, opt->rect, labelContainer);
            }
            case SC_GroupBoxCheckBox: {
                // The controls style doesn't draw the checkbox, so we don't return a rect for it
                return QRect();
            }
            case SC_GroupBoxContents: {
                const auto labelHeight = subControlRect(CC_GroupBox, opt, SC_GroupBoxLabel, w).height();
                const auto offset = metrics.padding.top() + qMax(labelHeight, metrics.margins.top() + metrics.spacing);
                QRect contentsRect(opt->rect.left() + metrics.padding.left(),
                                   opt->rect.top() + offset,
                                   opt->rect.width() - metrics.padding.left() - metrics.padding.right(),
                                   opt->rect.bottom() - opt->rect.top() - offset - metrics.padding.bottom());
                return visualRect(opt->direction, opt->rect, contentsRect);
            }
            default:
                break;
            }
        }
        break;
#endif // QT_CONFIG(groupbox)
#if QT_CONFIG(scrollbar)
    case CC_ScrollBar:
        if (const auto *scrollbar = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ScrollBar, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            const bool horizontal = scrollbar->orientation == Qt::Horizontal;
            const QRectF grooveRect = opt->rect.marginsRemoved(metrics.margins);

            switch (sc) {
            // TODO: support up/down buttons
            case SC_ScrollBarAddLine:
            case SC_ScrollBarSubLine:
            case SC_ScrollBarSubPage:
            case SC_ScrollBarAddPage:
                return QRect();
            case SC_ScrollBarGroove:
                return visualRect(opt->direction, opt->rect, grooveRect.toAlignedRect());
            case SC_ScrollBarSlider: {
                const auto *indicator = resolved.indicator();
                if (!indicator || !indicator->visible() || indicator->opacity() == 0)
                    return QRect();
                const QRectF contentsRect = grooveRect.marginsRemoved(metrics.padding);

                const qreal availableW = contentsRect.width()  - indicator->leftMargin() - indicator->rightMargin();
                const qreal availableH = contentsRect.height() - indicator->topMargin()  - indicator->bottomMargin();

                const int totalRange = scrollbar->maximum - scrollbar->minimum + scrollbar->pageStep;
                const qreal ratio = totalRange > 0 ? qreal(scrollbar->pageStep) / totalRange : 1.0;

                QRectF sliderRect;
                if (horizontal) {
                    const qreal sliderW = qMax(qreal(metrics.indicatorImplicitSize.width()), availableW * ratio);
                    const qreal sliderH = resolvedHeight(indicator, availableH);
                    const qreal travelRange = qMax(0.0, availableW - sliderW);
                    const int sliderPos = QStyle::sliderPositionFromValue(
                        scrollbar->minimum, scrollbar->maximum,
                        scrollbar->sliderPosition, int(travelRange), scrollbar->upsideDown);
                    sliderRect = QRectF(
                        contentsRect.x() + indicator->leftMargin() + sliderPos,
                        contentsRect.y() + indicator->topMargin() + (availableH - sliderH) / 2.0,
                        sliderW, sliderH);
                } else {
                    const qreal sliderH = qMax(qreal(metrics.indicatorImplicitSize.width()), availableH * ratio);
                    const qreal sliderW = resolvedWidth(indicator, availableW);
                    const qreal travelRange = qMax(0.0, availableH - sliderH);
                    const int sliderPos = QStyle::sliderPositionFromValue(
                        scrollbar->minimum, scrollbar->maximum,
                        scrollbar->sliderPosition, int(travelRange), scrollbar->upsideDown);
                    sliderRect = QRectF(
                        contentsRect.x() + indicator->leftMargin() + (availableW - sliderW) / 2.0,
                        contentsRect.y() + indicator->topMargin() + sliderPos,
                        sliderW, sliderH);
                }
                return visualRect(opt->direction, opt->rect, sliderRect.toAlignedRect());
            }
            default:
                break;
            }
        }
        break;
#endif // QT_CONFIG(scrollbar)
#if QT_CONFIG(toolbutton)
    case CC_ToolButton:
        if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ToolButton, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            QRect frameRect = opt->rect.marginsRemoved(metrics.margins);
            switch (sc) {
            case SC_ToolButton: {
                QRect contentsRect = frameRect.marginsRemoved(metrics.padding);
                const bool hasMenuButton = (tool->features
                    & (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::HasMenu))
                    != 0;
                const auto *indicator = resolved.indicator();
                if (hasMenuButton && indicator && indicator->visible() && indicator->opacity() > 0) {
                    const QRect menuRect = subControlRect(CC_ToolButton, opt, SC_ToolButtonMenu, w);
                    const int spacing = metrics.spacing;
                    const uint indicatorAlign = resolvedAlignment(indicator->alignment(),
                                                                  Qt::AlignRight, Qt::AlignVCenter);
                    if (indicatorAlign & Qt::AlignLeft)
                        contentsRect.setLeft(menuRect.right() + spacing);
                    else if (indicatorAlign & Qt::AlignRight)
                        contentsRect.setRight(menuRect.left() - spacing);
                }
                return visualRect(opt->direction, opt->rect, contentsRect);
            }
            case SC_ToolButtonMenu: {
                QRect contentsRect = frameRect.marginsRemoved(metrics.padding);
                if ((tool->features
                    & (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::PopupDelay))
                   != QStyleOptionToolButton::MenuButtonPopup)
                    break;

                const auto *indicator = resolved.indicator();
                if (!indicator || !indicator->visible() || indicator->opacity() == 0)
                    return QRect();

                const int w = qSaturateRound(resolvedWidth(indicator,
                    contentsRect.width() - indicator->leftMargin() - indicator->rightMargin()));
                const int h = qSaturateRound(resolvedHeight(indicator,
                    contentsRect.height() - indicator->topMargin() - indicator->bottomMargin()));
                const uint indicatorAlign = resolvedAlignment(indicator->alignment(), Qt::AlignRight, Qt::AlignVCenter);
                const QMargins indicatorMargins = elementMargins(indicator);
                return visualRect(opt->direction, opt->rect,
                    d->getAlignedRectInContainer(contentsRect, QSize(w, h), indicatorAlign, QMargins(), indicatorMargins));
            }
            default:
                break;
            }
        }
        break;
#endif // QT_CONFIG(toolbutton)
    default:
        break;
    }
    return QCommonStyle::subControlRect(cc, opt, sc, w);
}

/*! \reimp */
QSize QStyleKitStyle::sizeFromContents(ContentsType ct, const QStyleOption *opt,
                                       const QSize &contentsSize, const QWidget *widget) const
{
    Q_D(const QStyleKitStyle);

    switch (ct) {
#if QT_CONFIG(pushbutton)
    case CT_PushButton:
#endif
#if QT_CONFIG(toolbutton)
    case CT_ToolButton: {
        auto controlType = QQStyleKitReader::ControlType::ToolButton;
        if (const auto *buttonOpt = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            controlType = (buttonOpt->features & QStyleOptionButton::Flat)
                ? QQStyleKitReader::ControlType::FlatButton
                : QQStyleKitReader::ControlType::Button;
        }
        const auto resolved = d->resolveLayout(controlType, opt->state);
        if (!resolved.isValid())
            break;

        const auto &metrics = *resolved.metrics;
        const QSize contentSize = contentsSize
            + QSize(metrics.padding.left() + metrics.padding.right(),
                    metrics.padding.top() + metrics.padding.bottom())
            + QSize(metrics.textPadding.left() + metrics.textPadding.right(),
                    metrics.textPadding.top() + metrics.textPadding.bottom());
        const QSize bgSize = metrics.bgImplicitSize
            + QSize(metrics.margins.left() + metrics.margins.right(),
                    metrics.margins.top() + metrics.margins.bottom());
        return contentSize.expandedTo(bgSize);
    }
#endif
    case CT_CheckBox:
    case CT_RadioButton:
        if (const auto *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            const auto controlType = ct == CT_CheckBox ? QQStyleKitReader::ControlType::CheckBox
                                                       : QQStyleKitReader::ControlType::RadioButton;
            const auto resolved = d->resolveLayout(controlType, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            const QSize textSize = opt->fontMetrics.size(Qt::TextShowMnemonic, btn->text);
            const int bgWidth = metrics.bgImplicitSize.width() + metrics.margins.left() + metrics.margins.right();
            const int bgHeight = metrics.bgImplicitSize.height() + metrics.margins.top() + metrics.margins.bottom();
            const int contentWidth = std::max(textSize.width(), contentsSize.width()) + metrics.padding.left() + metrics.padding.right()
                                     + metrics.textPadding.left() + metrics.textPadding.right();
            const int contentHeight = std::max(textSize.height(), contentsSize.height()) + metrics.padding.top() + metrics.padding.bottom()
                                      + metrics.textPadding.top() + metrics.textPadding.bottom();
            const int indicatorWidth = metrics.indicatorImplicitSize.width() + metrics.indicatorMargins.left()
                                       + metrics.indicatorMargins.right();
            const int indicatorHeight = metrics.indicatorImplicitSize.height() + metrics.indicatorMargins.top()
                                        + metrics.indicatorMargins.bottom();
            return QSize(std::max({contentWidth + indicatorWidth + metrics.spacing, bgWidth}),
                         std::max({contentHeight, indicatorHeight, bgHeight}));
        }
        break;
#if QT_CONFIG(itemviews)
    case CT_ItemViewItem:
        if (const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            const auto resolved = d->resolveLayout(itemViewControlType(opt), opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            const QSize textSize = opt->fontMetrics.size(Qt::TextShowMnemonic, item->text);
            const int bgWidth = metrics.bgImplicitSize.width() + metrics.margins.left() + metrics.margins.right();
            const int bgHeight = metrics.bgImplicitSize.height() + metrics.margins.top() + metrics.margins.bottom();
            const int contentWidth = std::max(textSize.width(), contentsSize.width()) + metrics.padding.left() + metrics.padding.right()
                                     + metrics.textPadding.left() + metrics.textPadding.right();
            const int contentHeight = std::max(textSize.height(), contentsSize.height()) + metrics.padding.top() + metrics.padding.bottom()
                                      + metrics.textPadding.top() + metrics.textPadding.bottom();
            const int indicatorWidth = metrics.indicatorImplicitSize.width() + metrics.indicatorMargins.left()
                                       + metrics.indicatorMargins.right();
            const int indicatorHeight = metrics.indicatorImplicitSize.height() + metrics.indicatorMargins.top()
                                        + metrics.indicatorMargins.bottom();
            return QSize(std::max({contentWidth + indicatorWidth + metrics.spacing, bgWidth}),
                         std::max({contentHeight, indicatorHeight, bgHeight}));
        }
        break;
#endif // QT_CONFIG(itemviews)
    case CT_ProgressBar:
        if (qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ProgressBar, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            const auto indicatorW = std::max(metrics.indicatorImplicitSize.width()
                                                + metrics.indicatorMargins.left()
                                                + metrics.indicatorMargins.right(),
                                             metrics.foregroundImplicitSize.width()
                                                + metrics.foregroundMargins.left()
                                                + metrics.foregroundMargins.right());
            const auto indicatorH = std::max(metrics.indicatorImplicitSize.height()
                                                + metrics.indicatorMargins.top()
                                                + metrics.indicatorMargins.bottom(),
                                             metrics.foregroundImplicitSize.height()
                                                + metrics.foregroundMargins.top()
                                                + metrics.foregroundMargins.bottom());
            const int bgW = metrics.bgImplicitSize.width() + metrics.margins.left() + metrics.margins.right();
            const int bgH = metrics.bgImplicitSize.height() + metrics.margins.top() + metrics.margins.bottom();
            // For progress bar in Controls, the content size is based on the indicator size
            const int contentW = indicatorW + metrics.padding.left() + metrics.padding.right();
            const int contentH = indicatorH + metrics.padding.top() + metrics.padding.bottom();
            return QSize(std::max(contentW, bgW), std::max(contentH, bgH));
        }
        break;
    case CT_Slider:
        if (qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::Slider, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            // background = indicator = groove + track
            const int bgW = std::max(
                metrics.padding.left() + metrics.padding.right()
                    + metrics.indicatorImplicitSize.width()
                    + metrics.indicatorMargins.left() + metrics.indicatorMargins.right(),
                metrics.bgImplicitSize.width() + metrics.margins.left() + metrics.margins.right());
            const int bgH = std::max(
                metrics.padding.top() + metrics.padding.bottom()
                    + metrics.indicatorImplicitSize.height()
                    + metrics.indicatorMargins.top() + metrics.indicatorMargins.bottom(),
                metrics.bgImplicitSize.height() + metrics.margins.top() + metrics.margins.bottom());
            const int handleW = metrics.handleImplicitSize.width() + metrics.padding.left() + metrics.padding.right();
            const int handleH = metrics.handleImplicitSize.height() + metrics.padding.top() + metrics.padding.bottom();
            return QSize(std::max(handleW, bgW), std::max(handleH, bgH));
        }
        break;
#if QT_CONFIG(lineedit)
    case CT_LineEdit:
        if (const auto *lineEdit = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            QStyleOption lineEditOpt(*lineEdit);
            lineEditOpt.state &= ~QStyle::State_Sunken;
#  if QT_CONFIG(spinbox)
            const bool isInSpinBox = widget && qobject_cast<const QSpinBox *>(widget->parent());
#  else
            const bool isInSpinBox = false;
#  endif
            auto controlType = isInSpinBox ? QQStyleKitReader::ControlType::SpinBox : QQStyleKitReader::ControlType::TextField;
            const auto resolved = d->resolveLayout(controlType, lineEditOpt.state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            QSize bgSize(0, 0);
            // For spinbox, the line edit doesn't have its own background in the Controls style
            if (!isInSpinBox)
                bgSize = metrics.bgImplicitSize.grownBy(metrics.margins);
            const QSize contentSizeWithPadding = contentsSize.grownBy(metrics.textPadding).grownBy(metrics.padding);
            return contentSizeWithPadding.expandedTo(bgSize);
        }
        break;
#endif // QT_CONFIG(lineedit)
#if QT_CONFIG(combobox)
    case CT_ComboBox:
        if (const auto *comboBox = qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ComboBox, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            const QSize textSize = opt->fontMetrics.size(Qt::TextShowMnemonic, comboBox->currentText);
            const int bgW = metrics.bgImplicitSize.width() + metrics.margins.left() + metrics.margins.right();
            const int bgH = metrics.bgImplicitSize.height() + metrics.margins.top() + metrics.margins.bottom();
            const int contentW = textSize.width() + metrics.padding.left() + metrics.padding.right()
                                 + metrics.textPadding.left() + metrics.textPadding.right();
            const int contentH = textSize.height() + metrics.padding.top() + metrics.padding.bottom()
                                  + metrics.textPadding.top() + metrics.textPadding.bottom();
            const int indicatorW = metrics.indicatorImplicitSize.width() + metrics.indicatorMargins.left()
                               + metrics.indicatorMargins.right();
            const int indicatorH = metrics.indicatorImplicitSize.height() + metrics.indicatorMargins.top()
                                + metrics.indicatorMargins.bottom();
            return QSize(std::max({contentW + indicatorW + metrics.spacing, bgW}),
                         std::max({contentH, indicatorH, bgH}));
        }
        break;
#endif // QT_CONFIG(combobox)
#if QT_CONFIG(spinbox)
    case CT_SpinBox: {
        const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::SpinBox, opt->state);
        if (!resolved.isValid())
            break;
        const auto &metrics = *resolved.metrics;
        const int bgW = metrics.bgImplicitSize.width() + metrics.margins.left() + metrics.margins.right();
        const int bgH = metrics.bgImplicitSize.height() + metrics.margins.top() + metrics.margins.bottom();
        const int contentW = contentsSize.width() + metrics.padding.left() + metrics.padding.right()
                                + metrics.textPadding.left() + metrics.textPadding.right();
        const int contentH = contentsSize.height() + metrics.padding.top() + metrics.padding.bottom()
                                + metrics.textPadding.top() + metrics.textPadding.bottom();
        // TODO: Support vertical layout for spinbox, currently we assume horizontal layout
        // TODO: Calculate each indicator (up/down) size separately if they have different sizes in the style
        const int indicatorW = (metrics.indicatorImplicitSize.width() + metrics.indicatorMargins.left()
                            + metrics.indicatorMargins.right()) * 2;
        const int indicatorH = metrics.indicatorImplicitSize.height() + metrics.indicatorMargins.top()
                            + metrics.indicatorMargins.bottom();
        return QSize(std::max({contentW + indicatorW + metrics.spacing, bgW}),
                     std::max({contentH, indicatorH, bgH}));
    }
#endif // QT_CONFIG(spinbox)
#if QT_CONFIG(tabbar)
    case CT_TabBarTab:
        if (qstyleoption_cast<const QStyleOptionTab *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::TabButton, opt->state);
            if (!resolved.isValid())
                break;
            const auto &m = *resolved.metrics;
            const auto bgSize = m.bgImplicitSize.grownBy(m.margins);
            const auto contentSizeWithPadding = contentsSize.grownBy(m.padding + m.textPadding);
            return contentSizeWithPadding.expandedTo(bgSize);
        }
        break;
#endif // QT_CONFIG(tabbar)
#if QT_CONFIG(tabwidget)
    case CT_TabWidget:
        if (qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
            // TabWidget corresponds to Page
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::Page, opt->state);
            if (!resolved.isValid())
                break;
            const auto &m = *resolved.metrics;
            const auto bgSize = m.bgImplicitSize.grownBy(m.margins);
            const auto contentSizeWithPadding = contentsSize.grownBy(m.padding);
            return contentSizeWithPadding.expandedTo(bgSize);
        }
        break;
#endif // QT_CONFIG(tabwidget)
#if QT_CONFIG(groupbox)
    case CT_GroupBox:
        if (const auto *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::GroupBox, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            const QSize bgSize = metrics.bgImplicitSize + QSize(metrics.margins.left() + metrics.margins.right(),
                                                metrics.margins.top() + metrics.margins.bottom());
            const QSize contentSize = contentsSize.grownBy(metrics.padding);
            const QSize textSize = opt->fontMetrics.size(Qt::TextShowMnemonic, groupBox->text).grownBy(metrics.textPadding);
            return textSize.expandedTo(contentSize).expandedTo(bgSize);
        }
        break;
#endif // QT_CONFIG(groupbox)
#if QT_CONFIG(scrollbar)
    case CT_ScrollBar:
        if (qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ScrollBar, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;

            const int iH = metrics.indicatorImplicitSize.height()
                + metrics.padding.top() + metrics.padding.bottom()
                + metrics.indicatorMargins.top() + metrics.indicatorMargins.bottom();
            const int iW = metrics.indicatorImplicitSize.width()
                + metrics.padding.left() + metrics.padding.right()
                + metrics.indicatorMargins.left() + metrics.indicatorMargins.right();
            const int bgH = metrics.bgImplicitSize.height()
                + metrics.margins.top() + metrics.margins.bottom();
            const int bgW = metrics.bgImplicitSize.width()
                + metrics.margins.left() + metrics.margins.right();
            return QSize(std::max(iW, bgW), std::max(iH, bgH));
        }
        break;
#endif // QT_CONFIG(scrollbar)
#if QT_CONFIG(menu)
    case CT_MenuItem:
        if (const auto *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            // The menu dictates the item width
            // QMenu uses for PM_MenuHMargin for both left/right paddings,
            // so the available width is bgImplicitWidth + margins - 2 * padding.left.
            const auto menuType = widget && widget->inherits("QComboBoxPrivateContainer")
                ? QQStyleKitReader::ControlType::Popup
                : QQStyleKitReader::ControlType::Menu;
            const auto menuResolved = d->resolveLayout(menuType, opt->state);
            int minMenuW = 0;
            if (menuResolved.isValid()) {
                const auto &mm = *menuResolved.metrics;
                minMenuW = mm.bgImplicitSize.width() + mm.margins.left() + mm.margins.right()
                            - 2 * mm.padding.left();
            }

            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::MenuSeparator, opt->state);
                if (!resolved.isValid())
                    break;
                const auto &m = *resolved.metrics;
                const int bgWidth = m.bgImplicitSize.width() + m.margins.left() + m.margins.right();
                const int bgHeight = m.bgImplicitSize.height() + m.margins.top() + m.margins.bottom();
                return QSize(std::max(bgWidth, minMenuW), bgHeight);
            }

            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::MenuItem, opt->state);
            if (!resolved.isValid())
                break;
            const auto &m = *resolved.metrics;
            const auto *indicator = resolved.indicator();
            const auto *first = indicator ? indicator->first() : nullptr;
            const auto *second = indicator ? indicator->second() : nullptr;
            int indicatorW = 0;
            int indicatorH = 0;
            using QtPrivate::qSaturateRound;
            if (first && menuItem->checkType != QStyleOptionMenuItem::NotCheckable) {
                indicatorW += qSaturateRound(first->leftMargin() + first->width()
                                             + first->rightMargin()) + m.spacing;
                indicatorH = std::max(qSaturateRound(first->topMargin() + first->bottomMargin()
                                                     + first->height()), indicatorH);
            }
            if (second && menuItem->menuItemType == QStyleOptionMenuItem::SubMenu) {
                indicatorW += qSaturateRound(second->leftMargin() + second->width()
                                             + second->rightMargin()) + m.spacing;
                indicatorH = std::max(qSaturateRound(second->topMargin() + second->bottomMargin()
                                                     + second->height()), indicatorH);
            }
            const int bgH = m.bgImplicitSize.height() + m.margins.top() + m.margins.bottom();
            const int iconReserved = menuItem->maxIconWidth > 0 ? menuItem->maxIconWidth + m.spacing : 0;
            const int contentW = contentsSize.width() + m.padding.left() + m.padding.right()
                                   + m.textPadding.left() + m.textPadding.right()
                                   + indicatorW + iconReserved;
            const int contentH = contentsSize.height() + m.padding.top() + m.padding.bottom()
                                    + m.textPadding.top() + m.textPadding.bottom();
            return QSize(std::max(contentW, minMenuW),
                         std::max({contentH, indicatorH, bgH}));
        }
        break;
    case CT_Menu: {
        const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::Menu, opt->state);
        if (!resolved.isValid())
            break;
        const auto &m = *resolved.metrics;
        const int height = std::max(contentsSize.height(), m.bgImplicitSize.height())
            + m.margins.top() + m.margins.bottom();
        return QSize(contentsSize.width(), height);
    }
#endif // QT_CONFIG(menu)
#if QT_CONFIG(menubar)
    case CT_MenuBar: {
        const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::MenuBar, opt->state);
        if (!resolved.isValid())
            break;
        const auto &m = *resolved.metrics;
        const int bgWidth = m.bgImplicitSize.width() + m.margins.left() + m.margins.right();
        const int bgHeight = m.bgImplicitSize.height() + m.margins.top() + m.margins.bottom();
        const int contentW = contentsSize.width() + m.padding.left() + m.padding.right();
        const int contentH = contentsSize.height() + m.padding.top() + m.padding.bottom();
        return QSize(std::max(contentW, bgWidth), std::max(contentH, bgHeight));
    }
    case CT_MenuBarItem: {
        const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::MenuBarItem, opt->state);
        if (!resolved.isValid())
            break;
        const auto &m = *resolved.metrics;
        const int bgWidth = m.bgImplicitSize.width() + m.margins.left() + m.margins.right();
        const int bgHeight = m.bgImplicitSize.height() + m.margins.top() + m.margins.bottom();
        const int contentW = contentsSize.width() + m.padding.left() + m.padding.right()
                        + m.textPadding.left() + m.textPadding.right();
        const int contentH = contentsSize.height() + m.padding.top() + m.padding.bottom()
                        + m.textPadding.top() + m.textPadding.bottom();
        return QSize(std::max(contentW, bgWidth), std::max(contentH, bgHeight));
    }
#endif // QT_CONFIG(menubar)
    default:
        break;
    }
    return QCommonStyle::sizeFromContents(ct, opt, contentsSize, widget);
}

/*! \reimp */
int QStyleKitStyle::pixelMetric(PixelMetric m, const QStyleOption *opt, const QWidget *widget) const
{
    Q_D(const QStyleKitStyle);
    switch (m) {
#if QT_CONFIG(menu)
    case PM_MenuPanelWidth:
        // The panel border is drawn as part of the resolved background (see PE_PanelMenu);
        return 0;
    case PM_MenuTearoffHeight:
    case PM_MenuScrollerHeight:
        return 0;
    case PM_MenuHMargin:
    case PM_MenuVMargin: {
        const auto controlType = widget && widget->inherits("QComboBoxPrivateContainer")
            ? QQStyleKitReader::ControlType::Popup
            : QQStyleKitReader::ControlType::Menu;
        const auto resolved = d->resolveLayout(controlType, opt ? opt->state : QStyle::State_None);
        if (!resolved.isValid())
            break;
        return m == PM_MenuHMargin ? resolved.metrics->padding.left()
                                   : resolved.metrics->padding.top();
    }
#endif
#if QT_CONFIG(menubar)
    case PM_MenuBarItemSpacing: {
        const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::MenuBarItem,
                                               opt ? opt->state : QStyle::State_None);
        if (resolved.isValid())
            return resolved.metrics->spacing;
        break;
    }
#endif
#if QT_CONFIG(toolbar)
    case PM_ToolBarFrameWidth:
        // The frame is folded into the resolved background/margins (see CE_ToolBar);
        return 0;
    case PM_ToolBarItemMargin: {
        const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ToolBar,
                                               opt ? opt->state : QStyle::State_None);
        if (!resolved.isValid())
            break;
        return resolved.metrics->padding.left();
    }
    case PM_ToolBarItemSpacing: {
        const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ToolBar,
                                               opt ? opt->state : QStyle::State_None);
        if (resolved.isValid())
            return resolved.metrics->spacing;
        break;
    }
    case PM_ToolBarHandleExtent: {
        // The only lever available to honor the style's background width/height
        // QToolBarLayout uses this to calculate the minimum size of the toolbar
        const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ToolBar,
                                               opt ? opt->state : QStyle::State_None);
        if (!resolved.isValid())
            break;
        const auto &metrics = *resolved.metrics;
        const bool horizontal = !opt || (opt->state & State_Horizontal);
        const int crossImplicit = horizontal ? metrics.bgImplicitSize.height() : metrics.bgImplicitSize.width();
        const int crossMargins = horizontal ? metrics.margins.top() + metrics.margins.bottom()
                                             : metrics.margins.left() + metrics.margins.right();
        const int crossPadding = horizontal ? metrics.padding.top() + metrics.padding.bottom()
                                             : metrics.padding.left() + metrics.padding.right();
        return qMax(0, crossImplicit + crossMargins - crossPadding);
    }
#endif // QT_CONFIG(toolbar)
#if QT_CONFIG(tabbar)
    case PM_TabBarTabHSpace:
    case PM_TabBarTabVSpace:
    case PM_TabBarTabOverlap:
    // Not supported in StyleKit
    case PM_TabCloseIndicatorWidth:
    case PM_TabCloseIndicatorHeight:
        return 0;
#endif // QT_CONFIG(tabbar)
    default:
        break;
    }
    return QCommonStyle::pixelMetric(m, opt, widget);
}

/*! \reimp */
int QStyleKitStyle::styleHint(StyleHint sh, const QStyleOption *opt, const QWidget *w,
                              QStyleHintReturn *shret) const
{
    switch (sh) {
    // keep consistent with the Controls style behavior
    case SH_SpinBox_SelectOnStep:
    case SH_ToolBar_Movable:
    case SH_TabBar_PreferNoArrows:
        return 0;
    case SH_Menu_MouseTracking:
    case SH_MenuBar_MouseTracking:
    case SH_Menu_Scrollable:
        return 1;
    default:
        break;
    }
    return QCommonStyle::styleHint(sh, opt, w, shret);
}

/*! \reimp */
QPalette QStyleKitStyle::standardPalette() const
{
    return QCommonStyle::standardPalette();
}

/*! \reimp */
void QStyleKitStyle::polish(QWidget *widget)
{
    if (!widget)
        return;

    Q_D(QStyleKitStyle);

    // When no user style is loaded, create the empty fallback style now,
    // before any reader is created, so that all widgets are styled consistently
    if (!d->style)
        d->ensureDefaultStyle();

    widget->setAttribute(Qt::WA_Hover);

#if QT_CONFIG(scrollbar)
    // QScrollBar sets WA_OpaquePaintEvent in its constructor, which skips
    // background erase before paint. This can leave artifacts when the
    // style's background is invisible, so disable it
    if (qobject_cast<QScrollBar *>(widget))
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
#endif

    // Create per-widget reader for interactive controls (transitions)
    const bool isInteractiveControl = false
#if QT_CONFIG(pushbutton)
        || qobject_cast<const QPushButton *>(widget)
#endif
#if QT_CONFIG(checkbox)
        || qobject_cast<const QCheckBox *>(widget)
#endif
#if QT_CONFIG(radiobutton)
        || qobject_cast<const QRadioButton *>(widget)
#endif
#if QT_CONFIG(combobox)
        || qobject_cast<const QComboBox *>(widget)
        || (widget && widget->inherits("QComboBoxPrivateContainer"))
#endif
#if QT_CONFIG(lineedit)
        || qobject_cast<const QLineEdit *>(widget)
#endif
#if QT_CONFIG(textedit)
        || qobject_cast<const QTextEdit *>(widget)
        || qobject_cast<const QPlainTextEdit *>(widget)
#endif
#if QT_CONFIG(label)
        || qobject_cast<const QLabel *>(widget)
#endif
#if QT_CONFIG(progressbar)
        || qobject_cast<const QProgressBar *>(widget)
#endif
#if QT_CONFIG(slider)
        || qobject_cast<const QSlider *>(widget)
#endif
#if QT_CONFIG(scrollbar)
        || qobject_cast<const QScrollBar *>(widget)
#endif
#if QT_CONFIG(spinbox)
        || qobject_cast<const QSpinBox *>(widget)
#endif
#if QT_CONFIG(tabbar)
        || qobject_cast<const QTabBar *>(widget)
#endif
#if QT_CONFIG(tabwidget)
        || qobject_cast<const QTabWidget *>(widget)
#endif
#if QT_CONFIG(menu)
        || qobject_cast<const QMenu *>(widget)
        || qobject_cast<const QMenuBar *>(widget)
#endif
        ;
    if (isInteractiveControl)
        d->readerForWidget(widget);

    // Disable the viewport's autoFillBackground so the styled background shows through.
    // Only flip it when it was enabled, and store the widget so unpolish() can restore it
    // (e.g. when the application switches to a non-StyleKit style).
    if (QWidget *vp = managedViewport(widget); vp && vp->autoFillBackground()) {
        vp->setAutoFillBackground(false);
        d->bgFillDisabledWidgets.insert(widget);
    }
#if QT_CONFIG(menu)
    if (qobject_cast<QMenu *>(widget)
#  if QT_CONFIG(combobox)
        || widget->inherits("QComboBoxPrivateContainer")
#  endif
    ) {
        // Enable translucent backgrounds so that we can draw rounded corners on popup menus.
        // Store it in bgFillDisabledWidgets so that we can restore it in unpolish()
        if (!widget->testAttribute(Qt::WA_TranslucentBackground)) {
            widget->setAttribute(Qt::WA_TranslucentBackground, true);
            d->bgFillDisabledWidgets.insert(widget);
        }
    }
#endif

#if QT_CONFIG(lineedit)
    if (auto *lineEdit = qobject_cast<QLineEdit *>(widget)) {
        if (!lineEdit->property("_q_stylekit_alignment_set").toBool()) {
            QQStyleKitReader *r = d->readerForWidget(widget);
            if (r) {
                const QWidget *target = containerWidget(widget);
                QQStyleKitReader::ControlType ct = controlTypeForWidget(target);
                r->setControlType(ct);
                const auto *textProps = r->global()->text();
                if (textProps) {
                    const uint align = resolvedAlignment(
                        textProps->alignment(), Qt::AlignLeft, Qt::AlignVCenter);
                    lineEdit->setAlignment(Qt::Alignment(align));
                }
            }
        }
    }
#endif

    d->refreshStyleFont(widget);
    d->refreshStylePalette(widget);

    bool needsEventFilter = isSelfPaintingWidget(widget);
#if QT_CONFIG(menu)
    // Menus are transient: drop their sub-element tracking when they hide
    needsEventFilter = needsEventFilter || qobject_cast<QMenu *>(widget);
#endif
    if (needsEventFilter)
        widget->installEventFilter(this);

    QCommonStyle::polish(widget);
}

/*! \reimp */
void QStyleKitStyle::polish(QApplication *app)
{
    QCommonStyle::polish(app);
}

/*! \reimp */
void QStyleKitStyle::polish(QPalette &palette)
{
    QCommonStyle::polish(palette);
}

/*! \reimp */
void QStyleKitStyle::unpolish(QWidget *widget)
{
    Q_D(QStyleKitStyle);
    d->unsetStylePalette(widget);
    d->unsetStyleFont(widget);
    bool hasEventFilter = isSelfPaintingWidget(widget);
#if QT_CONFIG(menu)
    hasEventFilter = hasEventFilter || qobject_cast<QMenu *>(widget);
#endif
    if (hasEventFilter)
        widget->removeEventFilter(this);
    if (d->bgFillDisabledWidgets.remove(widget)) {
        if (QWidget *vp = managedViewport(widget))
            vp->setAutoFillBackground(true);
#if QT_CONFIG(menu)
        else if (qobject_cast<QMenu *>(widget)
#  if QT_CONFIG(combobox)
                 || widget->inherits("QComboBoxPrivateContainer")
#  endif
        ) {
            widget->setAttribute(Qt::WA_TranslucentBackground, false);
        }
#endif
    }
    d->cleanupWidgetReader(widget);
#if QT_CONFIG(scrollbar)
    if (qobject_cast<QScrollBar *>(widget))
        widget->setAttribute(Qt::WA_OpaquePaintEvent);
#endif
    QCommonStyle::unpolish(widget);
}

/*! \reimp */
void QStyleKitStyle::unpolish(QApplication *app)
{
    Q_D(QStyleKitStyle);
    d->clearMetricsCache();
    QCommonStyle::unpolish(app);
}

/*! \reimp */
bool QStyleKitStyle::eventFilter(QObject *obj, QEvent *event)
{
    Q_D(QStyleKitStyle);
    switch (event->type()) {
    case QEvent::EnabledChange:
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
        if (auto *w = qobject_cast<QWidget *>(obj)) {
            if (d->customPaletteWidgets.contains(w))
                d->refreshStylePalette(w);
            if (d->customFontWidgets.contains(w))
                d->refreshStyleFont(w);
        }
        break;
#if QT_CONFIG(menu)
    case QEvent::Hide:
        if (auto *menu = qobject_cast<QMenu *>(obj))
            d->cleanupSubElements(menu);
        break;
#endif
    default:
        break;
    }
    return QCommonStyle::eventFilter(obj, event);
}

/*! \reimp */
bool QStyleKitStyle::event(QEvent *event)
{
    return QCommonStyle::event(event);
}

QT_END_NAMESPACE

#include "moc_qstylekitstyle.cpp"
