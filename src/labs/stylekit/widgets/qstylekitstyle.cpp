// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qstylekitstyle.h"
#include "qstylekitstyle_p.h"
#include <QtWidgets/qstyleoption.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qapplication.h>
#if QT_CONFIG(scrollarea)
#include <QtWidgets/qabstractscrollarea.h>
#endif
#if QT_CONFIG(itemviews)
#include <QtWidgets/qabstractitemview.h>
#endif
#if QT_CONFIG(lineedit)
#include <QtWidgets/qlineedit.h>
#endif
#if QT_CONFIG(pushbutton)
#include <QtWidgets/qpushbutton.h>
#endif
#if QT_CONFIG(checkbox)
#include <QtWidgets/qcheckbox.h>
#endif
#if QT_CONFIG(radiobutton)
#include <QtWidgets/qradiobutton.h>
#endif
#if QT_CONFIG(combobox)
#include <QtWidgets/qcombobox.h>
#endif
#if QT_CONFIG(slider)
#include <QtWidgets/qslider.h>
#endif
#if QT_CONFIG(scrollbar)
#include <QtWidgets/qscrollbar.h>
#endif
#if QT_CONFIG(spinbox)
#include <QtWidgets/qspinbox.h>
#endif
#if QT_CONFIG(progressbar)
#include <QtWidgets/qprogressbar.h>
#endif
#if QT_CONFIG(textedit)
#include <QtWidgets/qtextedit.h>
#include <QtWidgets/qplaintextedit.h>
#endif
#if QT_CONFIG(tabbar)
#include <QtWidgets/qtabbar.h>
#endif
#if QT_CONFIG(toolbar)
#include <QtWidgets/qtoolbar.h>
#endif
#if QT_CONFIG(groupbox)
#include <QtWidgets/qgroupbox.h>
#endif
#if QT_CONFIG(menu)
#include <QtWidgets/qmenu.h>
#endif
#if QT_CONFIG(label)
#include <QtWidgets/qlabel.h>
#endif
#include <QtWidgets/private/qwidget_p.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/qpainterstateguard.h>
#include <QtGui/qstylehints.h>
#include <QtQml/private/qqmlcomponent_p.h>
#include <QtQml/qqmlengine.h>
#include <QtLabsStyleKit/private/qqstylekit_p.h>
#include <QtLabsStyleKit/private/qqstylekitcontrolproperties_p.h>
#include <QtLabsStyleKit/private/qqstylekitstyle_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QStyleKitStyle
    \inmodule QtLabsStyleKit
    \ingroup appearance
    \since 6.12

    \brief The QStyleKitStyle class applies a \l {Qt Labs StyleKit} style
    to Qt Widgets.

    QStyleKitStyle is a QStyle implementation that uses a \l StyleKit \l Style
    to style Qt Widgets. It loads the Style at the local path specified by
    \l stylePath, and uses it to resolve style properties such as colors,
    fonts, and sizes for widgets in various states, in order to paint them
    according to the style's design. This allows the same StyleKit style to be
    shared between Qt Quick Controls and Qt Widgets.

    \note StyleKit is a Qt Labs module, and its API may change between
    Qt releases.

    \section1 Loading a Style

    A style is a QML file whose root object is a \l Style. To load it,
    pass the file path to the constructor or to \l setStylePath():

    \code
    auto *style = new QStyleKitStyle(QStringLiteral(":/styles/MyStyle.qml"));
    QApplication::setStyle(style);
    \endcode

    QStyleKitStyle is also registered as a QStyleFactory plugin under the
    key \c StyleKit, which can be selected via the \c -style command line
    argument or QApplication::setStyle(). When created through the
    factory, set \l stylePath after construction to load a style file.

    \code
    auto *style = QStyleFactory::create("StyleKit");
    style->setProperty("stylePath", QStringLiteral(":/styles/MyStyle.qml"));
    QApplication::setStyle(style);
    \endcode

    The Style is loaded with an internal QQmlEngine owned by the
    QStyleKitStyle instance. If the path is invalid or the root object is
    not a \l Style, a warning is emitted and the style behaves like
    QCommonStyle until a valid \l stylePath is set.

    \section1 Themes

    A Style may define one or more named \l {Theme}{themes}. The active
    theme is selected with \l setThemeName(); the list of available
    themes is exposed through \l themeNames. The special theme name
    \c System makes the style follow the platform color scheme: when the
    OS color scheme changes, the active theme is recreated automatically
    and all widgets are repolished.

    \sa QStyle, QCommonStyle, QStyleFactory, {Qt Labs StyleKit}, Style, Theme
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

    The value must be one of the entries in \l themeNames, or the
    special name \c System to follow the platform color scheme.
    Setting this property updates all widgets to repaint with the
    new theme.
*/

/*!
    \property QStyleKitStyle::themeNames
    \brief the list of theme names exposed by the loaded \l Style.

    This list includes the built-in \c Light and \c Dark themes as well
    as any custom themes defined by the style.
*/

/*!
    \property QStyleKitStyle::customThemeNames
    \brief the list of custom theme names defined by the loaded \l Style.

    Unlike \l themeNames, this list excludes the built-in \c Light and
    \c Dark themes and contains only the themes explicitly defined by the
    style author. Returns an empty list when no style is loaded.

    \sa themeNames, themeName
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
#if QT_CONFIG(toolbar)
    if (qobject_cast<const QToolBar *>(widget))
        return QQStyleKitReader::ToolBar;
#endif
#if QT_CONFIG(groupbox)
    if (qobject_cast<const QGroupBox *>(widget))
        return QQStyleKitReader::GroupBox;
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

static qreal resolvedImplicitWidth(const QQStyleKitDelegateProperties *element, qreal availableW)
{
    return element->fillWidth() ? availableW : qMax(0.0, element->implicitWidth());
}

static qreal resolvedImplicitHeight(const QQStyleKitDelegateProperties *element, qreal avilableH)
{
    return element->fillHeight() ? avilableH : qMax(0.0, element->implicitHeight());
}

static QMargins elementMargins(const QQStyleKitDelegateProperties *element)
{
    return QMargins(element->leftMargin(), element->topMargin(),
                    element->rightMargin(), element->bottomMargin());
}

// Copied from qstylesheetstyle.cpp
static const QWidget *containerWidget(const QWidget *w)
{
#if QT_CONFIG(lineedit)
    if (qobject_cast<const QLineEdit *>(w)) {
        //if the QLineEdit is an embeddedWidget, we need the real widget
        QWidget *parent = w->parentWidget();
        if (false
#if QT_CONFIG(combobox)
            || qobject_cast<const QComboBox *>(parent)
#endif
#if QT_CONFIG(spinbox)
            || qobject_cast<const QAbstractSpinBox *>(parent)
#endif
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

void QStyleKitStylePrivate::updateStyle()
{
    clearMetricsCache();

    if (sharedReader && sharedReader->style() != style)
        sharedReader->setExplicitStyle(style);

    for (const auto &byItem : std::as_const(itemViewItemReaders)) {
        for (auto *reader : std::as_const(byItem)) {
            if (reader && reader->style() != style)
                reader->setExplicitStyle(style);
        }
    }

    for (auto *wr : std::as_const(widgetReaders)) {
        if (wr->style() != style)
            wr->setExplicitStyle(style);
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
    if (!style || !widget)
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
    if (!style || !widget)
        return;

    const QWidget *targetWidget = containerWidget(widget);
    QQStyleKitReader::ControlType controlType = controlTypeForWidget(targetWidget);

    auto *shared = ensureSharedReader();
    if (!shared)
        return;

    QStyleOption opt;
    opt.initFrom(targetWidget);
    const QQSK::State currentState = resolvedStateFor(controlType, opt.state);

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
    if (!style || !widget)
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
    if (!style || !widget)
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
    const QQSK::State currentState = resolvedStateFor(controlType, opt.state);

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
    if (!style || !widget)
        return nullptr;

    if (auto it = widgetReaders.find(widget); it != widgetReaders.end())
        return *it;

    auto *widgetReader = new QQStyleKitReader(const_cast<QStyleKitStyle *>(q));
    widgetReader->setExplicitStyle(style);
    widgetReader->setTarget(const_cast<QWidget *>(widget));
    widgetReader->setCompleted(true);
    widgetReaders.insert(widget, widgetReader);
    QObject::connect(widget, &QObject::destroyed, q, [this, widget]() {
        cleanupWidgetReader(widget);
    });
    return widgetReader;
}

void QStyleKitStylePrivate::cleanupWidgetReader(const QWidget *widget) const
{
    if (auto *reader = widgetReaders.take(widget))
        reader->deleteLater();
    cleanupItemViewItemReaders(widget);
}

/*! \internal
    Returns a unique key for the given item view item, or 0 if the option is not an item view item.
    The key is used to cache a reader for the item.
*/
quint64 QStyleKitStylePrivate::itemViewItemKeyForOption(const QStyleOption *opt)
{
    if (const auto *viewOpt = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
        const auto &idx = viewOpt->index;
        if (!idx.isValid())
            return 0;

        // Generate a unique key for the item using its row, column, and internalId
        return qHashMulti(0, idx.row(), idx.column(), quint64(idx.internalId()));
    }
    return 0;
}

/*! \internal
    Returns a reader for the given item view item, creating and caching it if needed.
    Each item gets its own reader so that transitions for different items animate independently.
*/
QQStyleKitReader *QStyleKitStylePrivate::readerForItemViewItem(
    const QWidget *widget, quint64 itemKey) const
{
    Q_Q(const QStyleKitStyle);
    if (!style)
        return nullptr;

    auto &byItem = itemViewItemReaders[widget];
    if (auto it = byItem.find(itemKey); it != byItem.end())
        return *it;

    auto *reader = new QQStyleKitReader(const_cast<QStyleKitStyle *>(q));
    reader->setExplicitStyle(style);
    reader->setTarget(const_cast<QWidget *>(widget));
    reader->setCompleted(true);
    byItem.insert(itemKey, reader);
    return reader;
}

void QStyleKitStylePrivate::cleanupItemViewItemReaders(const QWidget *widget) const
{
    const auto readers = itemViewItemReaders.take(widget);
    for (auto *reader : readers)
        reader->deleteLater();
}

/*! \internal
    Resolves the given QStyle::State into a QQSK::State based on the mapping of
    state flags for the given control type.
*/
QQSK::State QStyleKitStylePrivate::resolvedStateFor(
    QQStyleKitReader::ControlType type, QStyle::State state) const
{
    QQSK::State flags;
    flags.setFlag(QQSK::StateFlag::Disabled, !(state & QStyle::State_Enabled));
    flags.setFlag(QQSK::StateFlag::Hovered, (state & QStyle::State_MouseOver));
    flags.setFlag(QQSK::StateFlag::Pressed, state & QStyle::State_Sunken);
    flags.setFlag(QQSK::StateFlag::Checked, state & QStyle::State_On);
    flags.setFlag(QQSK::StateFlag::Focused, state & QStyle::State_HasFocus);
    flags.setFlag(QQSK::StateFlag::Highlighted, state & QStyle::State_Selected);
    flags.setFlag(QQSK::StateFlag::Vertical, !(state & QStyle::State_Horizontal));

    // ComboBox uses QStyle::State_On to indicate the popup is open, which is
    // not a "checked" semantic.
    if (type == QQStyleKitReader::ControlType::ComboBox)
        flags.setFlag(QQSK::StateFlag::Checked, false);

    // Popup has no hover state in the Controls style
    if (type == QQStyleKitReader::ControlType::Popup)
        flags.setFlag(QQSK::StateFlag::Hovered, false);

    return flags;
}

/*! \internal
    Returns the ControlMetrics for the given control type and state,
    reading from the style if needed and caching the result.
*/
const QStyleKitStylePrivate::ControlMetrics &QStyleKitStylePrivate::metricsFor(
    QQStyleKitReader::ControlType type, QQSK::State state) const
{
    if (auto it = metricsCache.find({type, state}); it != metricsCache.end())
        return *it;

    auto *reader = ensureSharedReader();
    Q_ASSERT(reader);
    reader->setControlTypeAndState(type, state);
    return *metricsCache.insert({type, state}, metricsForReader(reader));
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

    const QQSK::State resolvedState = resolvedStateFor(type, state);
    reader->setControlTypeAndState(type, resolvedState);

    out.reader = reader;
    out.metrics = &metricsFor(type, resolvedState);
    return out;
}

/*! \internal
    Resolves the properties for the given item view item, control type and state,
    and returns them as a QQStyleKitResolved.
    Falls back to resolve() if the option is not an item view item.
*/
QStyleKitStylePrivate::QQStyleKitResolved QStyleKitStylePrivate::resolveItemViewItem(
    const QWidget *w, const QStyleOption *opt,
    QQStyleKitReader::ControlType type, QStyle::State state) const
{
    // Same machinery as resolve(), but for sub-elements that have their own state
    // (item view rows/cells). Each (widget, itemKey) gets its own reader
    // so transitions for different items animate independently.
    // Falls back to the widget reader
    const quint64 itemKey = itemViewItemKeyForOption(opt);
    if (itemKey == 0)
        return resolve(w, type, state);

    auto *reader = readerForItemViewItem(w, itemKey);
    if (!reader)
        return resolve(w, type, state);

    const QQSK::State resolvedState = resolvedStateFor(type, state);
    reader->setControlTypeAndState(type, resolvedState);

    QQStyleKitResolved out;
    out.widget = w;
    out.reader = reader;
    out.metrics = &metricsFor(type, resolvedState);
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
    if (!style)
        return nullptr;
    sharedReader = new QQStyleKitReader(const_cast<QStyleKitStyle *>(q));
    // Disable transitions since this reader is used for one-off metric reads in layout queries
    sharedReader->setTransitionsEnabled(false);
    sharedReader->setExplicitStyle(style);
    sharedReader->setCompleted(true);
    return sharedReader;
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
    if (!indicator || !rect.isValid())
        return;

    // indicator (background)
    QRectF indicatorRect = rect;
    if (indicator->visible() && indicator->opacity() > 0)
        drawStyledItemRect(indicator, indicatorRect, painter);

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
    const auto foregroundW = resolvedImplicitWidth(foreground,
        indicatorRect.width() - foreground->leftMargin() - foreground->rightMargin());
    const auto foregroundH = resolvedImplicitHeight(foreground,
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
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setClipping(props->clip());

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

    // rotation
    if (props->rotation() != 0) {
        const auto center = rect.center();
        painter->translate(center);
        painter->rotate(props->rotation());
        painter->translate(-center);
    }

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
            QQuickGradientStop *stop = static_cast<QQuickGradientStop*>(stops.at(&stops, i));
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
    const QQStyleKitControlProperties *props = reader->global();
    ControlMetrics metrics;
    metrics.bgImplicitSize = QSize(0, 0);
    metrics.textPadding = QMargins(0, 0, 0, 0);
    metrics.padding = QMargins(props->leftPadding(), props->topPadding(),
                               props->rightPadding(), props->bottomPadding());
    metrics.spacing = props->spacing();
    metrics.margins = QMargins(0, 0, 0, 0);
    metrics.indicatorImplicitSize = QSize(0, 0);
    metrics.indicatorMargins = QMargins(0, 0, 0, 0);
    metrics.foregroundImplicitSize = QSize(0, 0);
    metrics.foregroundMargins = QMargins(0, 0, 0, 0);
    const auto *background = props->background();
    if (background) {
        auto scale = background->scale();
        if (scale == 0)
            scale = 1.0;
        metrics.bgImplicitSize = QSize(static_cast<int>(background->implicitWidth()), static_cast<int>(background->implicitHeight())) * scale;
        metrics.margins = elementMargins(background);
    }
    const auto *textProps = props->text();
    if (textProps)
        metrics.textPadding = QMargins(textProps->leftPadding(), textProps->topPadding(),textProps->rightPadding(), textProps->bottomPadding());
    const auto *indicator = props->indicator();
    if (indicator) {
        auto scale = indicator->scale();
        if (scale == 0)
            scale = 1.0;
        metrics.indicatorMargins = elementMargins(indicator);
        metrics.indicatorImplicitSize = QSize(std::max(.0, indicator->implicitWidth()),
                                             std::max(.0, indicator->implicitHeight())) * scale;

        const auto *foreground = indicator->foreground();
        if (foreground) {
            auto scale = foreground->scale();
            if (scale == 0)
                scale = 1.0;
            metrics.foregroundMargins = elementMargins(foreground);
            const auto foregroundW = resolvedImplicitWidth(foreground,
                std::max(.0, qreal(metrics.indicatorImplicitSize.width() - metrics.foregroundMargins.left() - metrics.foregroundMargins.right())));
            const auto foregroundH = resolvedImplicitHeight(foreground,
                std::max(.0, qreal(metrics.indicatorImplicitSize.height() - metrics.foregroundMargins.top() - metrics.foregroundMargins.bottom())));
            metrics.foregroundImplicitSize = QSize(foregroundW, foregroundH) * scale;
        }
    }
    const auto *handle = props->handle();
    if (handle) {
        auto scale = handle->scale();
        if (scale == 0)
            scale = 1.0;
        metrics.handleImplicitSize = QSize(std::max(.0, handle->implicitWidth()),
                                           std::max(.0, handle->implicitHeight())) * scale;
        metrics.handleMargins = elementMargins(handle);
    }
    return metrics;
}

/*!
    Constructs a QStyleKitStyle with no style loaded.

    Use \l setStylePath() to load a QML \l Style after construction.
    Until a style is loaded, painting and metrics behave as in
    QCommonStyle.
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
    is not a \l Style, a warning is emitted and the constructed style behaves as
    QCommonStyle until a valid \l stylePath is set.
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

    \sa setThemeName(), themeNames()
*/
QString QStyleKitStyle::themeName() const
{
    Q_D(const QStyleKitStyle);
    return d->style ? d->style->themeName() : QString();
}

/*!
    Activates the theme named \a themeName.

    \a themeName must be one of the entries in \l themeNames(), or the
    special name \c System to follow the platform color scheme. If no
    \l Style has been loaded, this function emits a warning and returns
    without changing the active theme.

    \sa themeName(), themeNames()
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
QStringList QStyleKitStyle::themeNames() const
{
    Q_D(const QStyleKitStyle);
    return d->style ? d->style->themeNames() : QStringList();
}

/*!
    Returns the names of the custom themes defined by the loaded
    \l Style, excluding the built-in \c Light and \c Dark themes.
    Returns an empty list when no style is loaded.

    \sa themeNames()
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
    if (!d->style) {
        qWarning("QStyleKitStyle: No StyleKit style loaded, drawing primitive with QCommonStyle: %d", int(pe));
        QCommonStyle::drawPrimitive(pe, opt, p, w);
        return;
    }

    switch (pe) {
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
#if QT_CONFIG(menu)
            || qobject_cast<const QMenu *>(w)
#endif
#if QT_CONFIG(combobox)
            || (w && w->inherits("QComboBoxPrivateContainer"))
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
#if QT_CONFIG(spinbox)
            const bool isInSpinBox = qobject_cast<const QAbstractSpinBox *>(parent);
#else
            const bool isInSpinBox = false;
#endif
#if QT_CONFIG(combobox)
            const bool isInComboBox = qobject_cast<const QComboBox *>(parent);
#else
            const bool isInComboBox = false;
#endif
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
        const auto r = d->resolveItemViewItem(w, opt, QQStyleKitReader::ControlType::ItemDelegate, opt->state);
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
        const auto r = d->resolveItemViewItem(w, opt, QQStyleKitReader::ControlType::ItemDelegate, opt->state);
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
    if (!d->style) {
        qWarning("QStyleKitStyle: No StyleKit style loaded, drawing control with QCommonStyle: %d", int(element));
        QCommonStyle::drawControl(element, opt, p, w);
        return;
    }

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
            proxy()->drawControl(CE_ProgressBarGroove, &contents, p, w);
            // track
            contents.rect = subElementRect(SE_ProgressBarContents, progressBar, w);
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
            if (foreground && foreground->visible() && foreground->opacity() > 0)
                d->drawStyledItemRect(foreground, contentsRect, p);
            return;
        }
        break;
#endif // QT_CONFIG(progressbar)
#if QT_CONFIG(itemviews)
    case CE_ItemViewItem:
        if (const auto *itemViewOption = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
            const auto r = d->resolveItemViewItem(w, opt, QQStyleKitReader::ControlType::ItemDelegate, opt->state);
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
    case CE_ShapedFrame:
#if QT_CONFIG(combobox)
        if (w && w->inherits("QComboBoxPrivateContainer")) {
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::Popup, opt->state);
            if (!r.isValid())
                break;
            QRect backgroundRect = opt->rect.marginsRemoved(r.metrics->margins);
            d->drawStyledItemRect(r.background(), backgroundRect, p);
            return;
        }
#endif // QT_CONFIG(combobox)
#if QT_CONFIG(textedit)
        if (qobject_cast<const QTextEdit *>(w)) {
            const auto r = d->resolve(w, QQStyleKitReader::ControlType::TextArea, opt->state);
            if (!r.isValid())
                break;
            QRect backgroundRect = opt->rect.marginsRemoved(r.metrics->margins);
            d->drawStyledItemRect(r.background(), backgroundRect, p);
            return;
        }
#endif // QT_CONFIG(textedit)
        break;
#if QT_CONFIG(scrollbar)
    case CE_ScrollBarSlider: {
        const auto r = d->resolve(w, QQStyleKitReader::ControlType::ScrollBar, opt->state);
        if (!r.isValid())
            break;
        d->drawControlIndicator(r.indicator(), opt->rect, p);
        return;
    }
#endif
#endif // QT_NO_FRAME
    default:
        break;
    }
    QCommonStyle::drawControl(element, opt, p, w);
}

/*! \reimp */
QRect QStyleKitStyle::subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget) const
{
    Q_D(const QStyleKitStyle);
    if (!d->style) {
        qWarning("QStyleKitStyle: No StyleKit style loaded, calculating subElementRect with QCommonStyle: %d", int(r));
        return QCommonStyle::subElementRect(r, opt, widget);
    }

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

        const int w = resolvedImplicitWidth(indicator,
            rect.width() - metrics.padding.left() - metrics.padding.right()
                - metrics.indicatorMargins.left() - metrics.indicatorMargins.right());
        const int h = resolvedImplicitHeight(indicator,
            rect.height() - metrics.padding.top() - metrics.padding.bottom()
                - metrics.indicatorMargins.top() - metrics.indicatorMargins.bottom());
        const uint alignment = resolvedAlignment(indicator->alignment(), Qt::AlignLeft, Qt::AlignVCenter);
        const QRect indicatorRect = d->getAlignedRectInContainer(
            rect, QSize(w, h), alignment, metrics.padding, metrics.indicatorMargins);
        return visualRect(opt->direction, rect, indicatorRect);
    }
#if QT_CONFIG(itemviews)
    case SE_ItemViewItemCheckIndicator: {
        const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ItemDelegate, opt->state);
        if (!resolved.isValid())
            break;
        const auto &metrics = *resolved.metrics;
        QRect rect = opt->rect.marginsRemoved(metrics.margins);
        const auto *indicator = resolved.indicator();
        if (!indicator || !indicator->visible() || indicator->opacity() == 0)
            return rect;

        const int w = resolvedImplicitWidth(indicator,
            rect.width() - metrics.padding.left() - metrics.padding.right()
                - metrics.indicatorMargins.left() - metrics.indicatorMargins.right());
        const int h = resolvedImplicitHeight(indicator,
            rect.height() - metrics.padding.top() - metrics.padding.bottom()
                - metrics.indicatorMargins.top() - metrics.indicatorMargins.bottom());
        const uint alignment = resolvedAlignment(indicator->alignment(), Qt::AlignLeft, Qt::AlignVCenter);
        const QRect indicatorRect = d->getAlignedRectInContainer(
            rect, QSize(w, h), alignment, metrics.padding, metrics.indicatorMargins);
        return visualRect(opt->direction, rect, indicatorRect);
    }
    case SE_ItemViewItemText:
    if (const auto *itemViewOption = qstyleoption_cast<const QStyleOptionViewItem *>(opt)) {
        const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ItemDelegate, opt->state);
        if (!resolved.isValid())
            break;
        const auto &metrics = *resolved.metrics;
        QRect contentsRect = opt->rect.marginsRemoved(metrics.margins + metrics.padding);
        QRect indicatorRect;
        if (itemViewOption->features & QStyleOptionViewItem::HasCheckIndicator)
            indicatorRect = subElementRect(SE_ItemViewItemCheckIndicator, opt, widget);
        const int spacing = metrics.spacing;
        QRect textRect = contentsRect;
        const auto *textProps = resolved.text();
        uint textAlign;
        if (textProps)
            textAlign = resolvedAlignment(textProps->alignment(), Qt::AlignLeft, Qt::AlignVCenter);
        else
            textAlign = Qt::AlignLeft | Qt::AlignVCenter;
        if (indicatorRect.isValid()) {
            if (textAlign & Qt::AlignLeft) {
                textRect.setLeft(indicatorRect.right() + spacing + metrics.textPadding.left());
            } else if (textAlign & Qt::AlignHCenter) {
                textRect.setLeft(indicatorRect.right() + spacing + metrics.textPadding.left());
                textRect.setRight(indicatorRect.left() - spacing - metrics.textPadding.right());
            } else {
                textRect.setRight(indicatorRect.left() - spacing - metrics.textPadding.right());
            }
        } else {
            if (textAlign & Qt::AlignLeft) {
                textRect.setLeft(textRect.left() + metrics.textPadding.left());
            } else if (textAlign & Qt::AlignHCenter) {
                textRect.setLeft(textRect.left() + metrics.textPadding.left() / 2);
                textRect.setRight(textRect.right() - metrics.textPadding.right() / 2);
            } else {
                textRect.setRight(textRect.right() - metrics.textPadding.right());
            }
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

        const int w = resolvedImplicitWidth(indicator,
            rect.width() - metrics.padding.left() - metrics.padding.right()
                - metrics.indicatorMargins.left() - metrics.indicatorMargins.right());
        const int h = resolvedImplicitHeight(indicator,
            rect.height() - metrics.padding.top() - metrics.padding.bottom()
                - metrics.indicatorMargins.top() - metrics.indicatorMargins.bottom());
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
            const auto foregroundW = resolvedImplicitWidth(foreground,
                indicatorRect.width() - foreground->leftMargin() - foreground->rightMargin());
            const auto foregroundH = resolvedImplicitHeight(foreground,
                indicatorRect.height() - foreground->topMargin() - foreground->bottomMargin());
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
    if (!d->style) {
        qWarning("QStyleKitStyle: No StyleKit style loaded, drawing complex control with QCommonStyle: %d", int(cc));
        QCommonStyle::drawComplexControl(cc, opt, p, w);
        return;
    }

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
                    const auto availableW = isHorizontal
                        ? grooveRect.width() - foreground->leftMargin() - foreground->rightMargin()
                        : grooveRect.width() - foreground->topMargin() - foreground->bottomMargin();
                    const auto availableH = isHorizontal
                        ? grooveRect.height() - foreground->topMargin() - foreground->bottomMargin()
                        : grooveRect.height() - foreground->leftMargin() - foreground->rightMargin();

                    const qreal range = slider->maximum - slider->minimum;
                    const qreal ratio = range > 0 ? (slider->sliderPosition - slider->minimum) / range : 0;

                    qreal trackW, trackH;
                    if (isHorizontal) {
                        trackW = ratio * availableW;
                        trackH = resolvedImplicitHeight(foreground, availableH);
                    } else {
                        trackW = resolvedImplicitWidth(foreground, availableW);
                        trackH = ratio * availableH;
                    }
                    QRect trackRect(0, 0, trackW, trackH);
                    // alignment
                    const uint rawHAlign = foreground->alignment() & Qt::AlignHorizontal_Mask;
                    const uint rawVAlign = foreground->alignment() & Qt::AlignVertical_Mask;
                    // For vertical orientation, swap alignment
                    uint hAlign, vAlign;
                    if (isHorizontal) {
                        hAlign = rawHAlign;
                        vAlign = rawVAlign;
                    } else {
                        if (rawVAlign == Qt::AlignTop)
                            hAlign = Qt::AlignLeft;
                        else if (rawVAlign == Qt::AlignVCenter)
                            hAlign = Qt::AlignHCenter;
                        else if (rawVAlign == Qt::AlignBottom)
                            hAlign = Qt::AlignRight;
                        else
                            hAlign = 0;

                        if (rawHAlign == Qt::AlignLeft)
                            vAlign = Qt::AlignTop;
                        else if (rawHAlign == Qt::AlignHCenter)
                            vAlign = Qt::AlignVCenter;
                        else if (rawHAlign == Qt::AlignRight)
                            vAlign = Qt::AlignBottom;
                        else
                            vAlign = 0;
                    }
                    if (hAlign & Qt::AlignLeft)
                        trackRect.moveLeft(grooveRect.left() + foreground->leftMargin());
                    else if (hAlign & Qt::AlignHCenter)
                        trackRect.moveLeft(grooveRect.left() + foreground->leftMargin()
                                           + (availableW - trackRect.width()) / 2);
                    else if (hAlign & Qt::AlignRight)
                        trackRect.moveLeft(grooveRect.right() - foreground->rightMargin() - trackRect.width());
                    if (vAlign & Qt::AlignTop) {
                        if (!isHorizontal)
                            trackRect.moveTop(grooveRect.bottom() - foreground->bottomMargin() - trackRect.height());
                        else
                            trackRect.moveTop(grooveRect.top() + foreground->topMargin());
                    } else if (vAlign & Qt::AlignVCenter) {
                        trackRect.moveTop(grooveRect.top() + (grooveRect.height() - trackRect.height()) / 2);
                    } else if (vAlign & Qt::AlignBottom) {
                        if (!isHorizontal)
                             trackRect.moveTop(grooveRect.top() + foreground->topMargin());
                        else
                            trackRect.moveTop(grooveRect.bottom() - foreground->bottomMargin() - trackRect.height());
                    }
                    d->drawStyledItemRect(foreground, visualRect(opt->direction, grooveRect, trackRect), p);
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
    Q_D(const QStyleKitStyle);
    if (!d->style) {
        qWarning("QStyleKitStyle: No StyleKit style loaded, calculating subControlRect with QCommonStyle: %d %d",
                 int(cc), int(sc));
        return QCommonStyle::subControlRect(cc, opt, sc, w);
    }

    switch (cc) {
#if QT_CONFIG(slider)
    case CC_Slider:
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::Slider, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            const bool horizontal = slider->orientation == Qt::Horizontal;
            QRect contentsRect = opt->rect.marginsRemoved(horizontal
                ? QMargins(metrics.margins.left(), metrics.margins.top(), metrics.margins.right(), metrics.margins.bottom())
                : QMargins(metrics.margins.top(), metrics.margins.left(), metrics.margins.bottom(), metrics.margins.right())
            ).marginsRemoved(horizontal
                ? QMargins(metrics.padding.left(), metrics.padding.top(), metrics.padding.right(), metrics.padding.bottom())
                : QMargins(metrics.padding.top(), metrics.padding.left(), metrics.padding.bottom(), metrics.padding.right())
            );

            const auto *indicator = resolved.indicator();
            if (!indicator)
                return contentsRect;
            switch (sc) {
            case SC_SliderGroove: {
                const auto availableW = horizontal
                    ? contentsRect.width() - indicator->leftMargin() - indicator->rightMargin()
                    : contentsRect.width() - indicator->topMargin() - indicator->bottomMargin();
                const auto availableH = horizontal
                    ? contentsRect.height() - indicator->topMargin() - indicator->bottomMargin()
                    : contentsRect.height() - indicator->leftMargin() - indicator->rightMargin();

                qreal grooveW, grooveH;
                if (horizontal) {
                    grooveW = resolvedImplicitWidth(indicator, availableW);
                    grooveH = resolvedImplicitHeight(indicator, availableH);
                } else {
                    grooveW = resolvedImplicitHeight(indicator, availableW);
                    grooveH = resolvedImplicitWidth(indicator, availableH);
                }

                QRectF grooveRect(0, 0, grooveW, grooveH);
                // alignment
                const uint rawHAlign = indicator->alignment() & Qt::AlignHorizontal_Mask;
                const uint rawVAlign = indicator->alignment() & Qt::AlignVertical_Mask;
                // For vertical orientation, swap alignment axes
                uint hAlign, vAlign;
                if (horizontal) {
                    hAlign = rawHAlign ? static_cast<Qt::Alignment>(rawHAlign) : Qt::AlignLeft;
                    vAlign = rawVAlign ? static_cast<Qt::Alignment>(rawVAlign) : Qt::AlignVCenter;
                } else {
                    if (rawVAlign & Qt::AlignTop)
                        hAlign = Qt::AlignLeft;
                    else if (rawVAlign & Qt::AlignBottom)
                        hAlign = Qt::AlignRight;
                    else if (rawVAlign & Qt::AlignVCenter)
                        hAlign = Qt::AlignHCenter;
                    else
                        hAlign = Qt::AlignLeft;

                    if (rawHAlign & Qt::AlignLeft)
                        vAlign = Qt::AlignTop;
                    else if (rawHAlign & Qt::AlignRight)
                        vAlign = Qt::AlignBottom;
                    else if (rawHAlign & Qt::AlignHCenter)
                        vAlign = Qt::AlignVCenter;
                    else
                        vAlign = Qt::AlignVCenter;
                }

                if (hAlign & Qt::AlignLeft) {
                    grooveRect.moveLeft(contentsRect.x() + indicator->leftMargin());
                } else if (hAlign & Qt::AlignHCenter) {
                    const int availableWidth = contentsRect.width()
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
                    const int availableHeight = contentsRect.height()
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

                QRect handleRect;
                const auto handleW = resolvedImplicitWidth(handle,
                    contentsRect.width() - handle->leftMargin() - handle->rightMargin());
                const auto handleH = resolvedImplicitHeight(handle,
                    contentsRect.height() - handle->topMargin() - handle->bottomMargin());
                const int range = horizontal ? contentsRect.width() - handleW : contentsRect.height() - handleH;
                const int sliderPos = QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                    slider->sliderPosition, range, !horizontal);
                if (horizontal)
                    handleRect = QRect(contentsRect.x() + sliderPos, contentsRect.y() + handle->topMargin() + (contentsRect.height() - handleH) / 2, handleW, handleH);
                else
                    handleRect = QRect(contentsRect.x() + handle->leftMargin() + (contentsRect.width() - handleW) / 2, contentsRect.y() + sliderPos, handleW, handleH);
                return visualRect(opt->direction, opt->rect, handleRect);
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
            QRect frameRect = combo->rect;
            const auto r = d->resolveLayout(QQStyleKitReader::ControlType::ComboBox, combo->state);
            if (!r.isValid())
                break;
            const auto &metrics = *r.metrics;
            frameRect = frameRect.marginsRemoved(metrics.margins);
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
                    return contentsRect;

                const int w = resolvedImplicitWidth(indicator,
                    contentsRect.width() - indicator->leftMargin() - indicator->rightMargin());
                const int h = resolvedImplicitHeight(indicator,
                    contentsRect.height() - indicator->topMargin() - indicator->bottomMargin());
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

                const int w = resolvedImplicitWidth(upIndicator,
                    contentsRect.width() - upIndicator->leftMargin() - upIndicator->rightMargin());
                const int h = resolvedImplicitHeight(upIndicator,
                    contentsRect.height() - upIndicator->topMargin() - upIndicator->bottomMargin());
                const uint upAlign = resolvedAlignment(upIndicator->alignment(), Qt::AlignLeft, Qt::AlignVCenter);
                const QMargins upMargins = elementMargins(upIndicator);
                return visualRect(opt->direction, opt->rect,
                    d->getAlignedRectInContainer(contentsRect, QSize(w, h), upAlign, QMargins(), upMargins));
            }
            case SC_SpinBoxDown: {
                const auto *downIndicator = r.indicator() ? r.indicator()->second() : nullptr;
                if (!downIndicator || !downIndicator->visible() || downIndicator->opacity() == 0)
                    return frameRect;

                const int w = resolvedImplicitWidth(downIndicator,
                    contentsRect.width() - downIndicator->leftMargin() - downIndicator->rightMargin());
                const int h = resolvedImplicitHeight(downIndicator,
                    contentsRect.height() - downIndicator->topMargin() - downIndicator->bottomMargin());
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
            const QRectF grooveRect = opt->rect.marginsRemoved(horizontal
                ? QMargins(metrics.margins.left(), metrics.margins.top(), metrics.margins.right(), metrics.margins.bottom())
                : QMargins(metrics.margins.top(), metrics.margins.left(), metrics.margins.bottom(), metrics.margins.right()));

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
                QRectF contentsRect = grooveRect.marginsRemoved(horizontal
                    ? QMargins(metrics.padding.left(), metrics.padding.top(), metrics.padding.right(), metrics.padding.bottom())
                    : QMargins(metrics.padding.top(), metrics.padding.left(), metrics.padding.bottom(), metrics.padding.right()));

                const qreal availableW = (horizontal ? contentsRect.width()  - indicator->leftMargin() - indicator->rightMargin()
                                                     : contentsRect.height() - indicator->topMargin()  - indicator->bottomMargin());
                const qreal availableH = (horizontal ? contentsRect.height() - indicator->topMargin()  - indicator->bottomMargin()
                                                     : contentsRect.width()  - indicator->leftMargin() - indicator->rightMargin());

                const int totalRange = scrollbar->maximum - scrollbar->minimum + scrollbar->pageStep;
                const qreal ratio = totalRange > 0 ? qreal(scrollbar->pageStep) / totalRange : 1.0;
                const qreal sliderW = qMax(qreal(metrics.indicatorImplicitSize.width()),
                                               availableW * ratio);
                const qreal sliderH = resolvedImplicitHeight(indicator, availableH);

                const qreal travelRange = qMax(0.0, availableW - sliderW);
                const int sliderPos = QStyle::sliderPositionFromValue(
                    scrollbar->minimum, scrollbar->maximum,
                    scrollbar->sliderPosition, int(travelRange), scrollbar->upsideDown);

                QRectF sliderRect;
                if (horizontal) {
                    sliderRect = QRectF(
                        contentsRect.x() + indicator->leftMargin() + sliderPos,
                        contentsRect.y() + indicator->topMargin() + (availableH - sliderH) / 2.0,
                        sliderW, sliderH);
                } else {
                    sliderRect = QRectF(
                        contentsRect.x() + indicator->leftMargin() + (availableH - sliderH) / 2.0,
                        contentsRect.y() + indicator->topMargin() + sliderPos,
                        sliderH, sliderW);
                }
                return visualRect(opt->direction, opt->rect, sliderRect.toAlignedRect());
            }
            default:
                break;
            }
        }
        break;
#endif // QT_CONFIG(scrollbar)
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
    if (!d->style) {
        qWarning("QStyleKitStyle: No StyleKit style loaded, calculating sizeFromContents with QCommonStyle: %d", int(ct));
        return QCommonStyle::sizeFromContents(ct, opt, contentsSize, widget);
    }

    switch (ct) {
    case CT_PushButton:
        if (const auto *btn = qstyleoption_cast<const QStyleOptionButton *>(opt)) {
            const auto controlType = btn->features & QStyleOptionButton::Flat
                                        ? QQStyleKitReader::ControlType::FlatButton
                                        : QQStyleKitReader::ControlType::Button;
            const auto resolved = d->resolveLayout(controlType, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            const QSize textSize = opt->fontMetrics.size(Qt::TextShowMnemonic, btn->text);
            const QSize contentSizeWithPadding = textSize.expandedTo(contentsSize)
                                            + QSize(metrics.padding.left() + metrics.padding.right(),
                                            metrics.padding.top() + metrics.padding.bottom())
                                            + QSize(metrics.textPadding.left() + metrics.textPadding.right(),
                                            metrics.textPadding.top() + metrics.textPadding.bottom());
            const QSize bgSizeWithMargins = metrics.bgImplicitSize + QSize(metrics.margins.left() + metrics.margins.right(),
                                            metrics.margins.top() + metrics.margins.bottom());
            return contentSizeWithPadding.expandedTo(bgSizeWithMargins);
        }
        break;
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
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ItemDelegate, opt->state);
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
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::Slider, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            // background in Controls = indicator = groove + track
            const int bgW = std::max(metrics.indicatorImplicitSize.width() + metrics.indicatorMargins.left() + metrics.indicatorMargins.right(),
                                     metrics.bgImplicitSize.width() + metrics.margins.left() + metrics.margins.right());
            const int bgH = std::max(metrics.indicatorImplicitSize.height() + metrics.indicatorMargins.top() + metrics.indicatorMargins.bottom(),
                                     metrics.bgImplicitSize.height() + metrics.margins.top() + metrics.margins.bottom());
            const int handleW = metrics.handleImplicitSize.width() + metrics.padding.left() + metrics.padding.right();
            const int handleH = metrics.handleImplicitSize.height() + metrics.padding.top() + metrics.padding.bottom();
            return slider->orientation == Qt::Horizontal
                    ? QSize(std::max(handleW, bgW), std::max(handleH, bgH))
                    : QSize(std::max(handleH, bgH), std::max(handleW, bgW));
        }
        break;
#if QT_CONFIG(lineedit)
    case CT_LineEdit:
        if (const auto *lineEdit = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            QStyleOption lineEditOpt(*lineEdit);
            lineEditOpt.state &= ~QStyle::State_Sunken;
#if QT_CONFIG(spinbox)
            const bool isInSpinBox = widget && qobject_cast<const QSpinBox *>(widget->parent());
#else
            const bool isInSpinBox = false;
#endif
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
        if (const auto *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
            const auto resolved = d->resolveLayout(QQStyleKitReader::ControlType::ScrollBar, opt->state);
            if (!resolved.isValid())
                break;
            const auto &metrics = *resolved.metrics;
            const bool horizontal = scrollBar->orientation == Qt::Horizontal;

            const int iH = metrics.indicatorImplicitSize.height()
                + (horizontal ? metrics.padding.top()  + metrics.padding.bottom()
                              : metrics.padding.left() + metrics.padding.right())
                + (horizontal ? metrics.indicatorMargins.top()  + metrics.indicatorMargins.bottom()
                              : metrics.indicatorMargins.left() + metrics.indicatorMargins.right());
            const int iW = metrics.indicatorImplicitSize.width()
                + (horizontal ? metrics.padding.left() + metrics.padding.right()
                              : metrics.padding.top()  + metrics.padding.bottom())
                + (horizontal ? metrics.indicatorMargins.left() + metrics.indicatorMargins.right()
                              : metrics.indicatorMargins.top()  + metrics.indicatorMargins.bottom());
            const int bgH = metrics.bgImplicitSize.height()
                + (horizontal ? metrics.margins.top()  + metrics.margins.bottom()
                              : metrics.margins.left() + metrics.margins.right());
            const int bgW = metrics.bgImplicitSize.width()
                + (horizontal ? metrics.margins.left() + metrics.margins.right()
                              : metrics.margins.top()  + metrics.margins.bottom());
            return horizontal
                    ? QSize(std::max(iW, bgW), std::max(iH, bgH))
                    : QSize(std::max(iH, bgH), std::max(iW, bgW));
        }
        break;
#endif // QT_CONFIG(scrollbar)
    default:
        break;
    }
    return QCommonStyle::sizeFromContents(ct, opt, contentsSize, widget);
}

/*! \reimp */
int QStyleKitStyle::pixelMetric(PixelMetric m, const QStyleOption *opt, const QWidget *widget) const
{
    // case PM_LayoutBottomMargin:
    // case PM_LayoutTopMargin:
    // case PM_LayoutLeftMargin:
    // case PM_LayoutRightMargin:
    //     return -100.0;
    return QCommonStyle::pixelMetric(m, opt, widget);
}

/*! \reimp */
int QStyleKitStyle::styleHint(StyleHint sh, const QStyleOption *opt, const QWidget *w,
    QStyleHintReturn *shret) const
{
    switch (sh) {
    case SH_SpinBox_SelectOnStep:
        return 0;
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
        ;
    if (isInteractiveControl)
        d->readerForWidget(widget);

    // Disable the viewport's autoFillBackground so the styled background shows through.
    // Only flip it when it was enabled, and store the widget so unpolish() can restore it
    // (e.g. when the application switches to a non-StyleKit style).
    if (QWidget *vp = managedViewport(widget); vp && vp->autoFillBackground()) {
        vp->setAutoFillBackground(false);
        d->autoFillDisabledWidgets.insert(widget);
    }

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

    if (isSelfPaintingWidget(widget))
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
    if (isSelfPaintingWidget(widget))
        widget->removeEventFilter(this);
    if (d->autoFillDisabledWidgets.remove(widget)) {
        if (QWidget *vp = managedViewport(widget))
            vp->setAutoFillBackground(true);
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
