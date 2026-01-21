// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquicksearchfield_p.h"
#include "qquickcontrol_p_p.h"
#include <private/qquickindicatorbutton_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>
#include "qquickpopup_p_p.h"
#include "qquickdeferredexecute_p_p.h"
#include <private/qqmldelegatemodel_p.h>
#include "qquickabstractbutton_p.h"
#include "qquickabstractbutton_p_p.h"
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/private/qquickaccessibleattached_p.h>
#if QT_CONFIG(quick_itemview)
#  include <QtQuick/private/qquickitemview_p.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmltype SearchField
    \inherits Control
 //!    \nativetype QQuickSearchField
    \inqmlmodule QtQuick.Controls
    \since 6.10
    \ingroup qtquickcontrols-input
    \ingroup qtquickcontrols-focusscopes
    \brief A specialized input field designed to use for search functionality.

    SearchField is a specialized input field designed to use for search functionality.
    The control includes a text field, search and clear icons, and a popup that
    displays suggestions or search results.

    \note The iOS style does not provide a built-in popup for SearchField in
        order to preserve the native look and feel. If a popup is still wanted,
        it has to be defined by the user.

    \image qtquickcontrols-searchfield.gif
           {Search field with search icon and clear button}

    \section1 SearchField's Indicators

    SearchField provides two optional embedded indicator buttons: \l searchIndicator and
    \l clearIndicator.

    These are not indicators in the sense of \l BusyIndicator or \l ProgressBar. Instead,
    they are interactive controls embedded into the field (similar to the up/down buttons
    in \l SpinBox). Pressing \l searchIndicator triggers \l searchButtonPressed(), and pressing
    \l clearIndicator triggers \l clearButtonPressed().

    In addition to exposing the actions, the indicator buttons provide interaction state
    (pressed/hovered/focused, etc.) that can be used by styles.

    \section2 Customizing indicator content

    The \l searchIndicator and \l clearIndicator properties are read-only. Customization is
    supported through their internal properties.

    In particular, the button's visual content is provided by its \c indicator item, which is
    writable. This allows the default content to be replaced or removed entirely.

    For example, to remove both indicator icons:

    \code
    SearchField {
        searchIndicator.indicator: null
        clearIndicator.indicator: null
    }
    \endcode

    This is a supported customization scenario. Different SearchField variants may omit one
    of the buttons (for example, providing only a search button) or replace the indicator
    content with an alternative item (for example, a microphone icon to trigger speech input).

    \sa searchIndicator, clearIndicator, searchButtonPressed(), clearButtonPressed()

    \section1 SearchField Model Roles

    SearchField is able to visualize standard \l {qml-data-models}{data models}
    that provide the \c modelData role:
    \list
    \li models that have only one role
    \li models that do not have named roles (JavaScript array, integer)
    \endlist

    When using models that have multiple named roles, SearchField must be configured
    to use a specific \l {textRole}{text role} for its \l {text}{text}
    and \l delegate instances.

    \code
    ListModel {
        id : fruitModel
        ListElement { name: "Apple"; color: "green" }
        ListElement { name: "Cherry"; color: "red" }
        ListElement { name: "Banana"; color: "yellow" }
        ListElement { name: "Orange"; color: "orange" }
        ListElement { name: "WaterMelon"; color: "pink" }
    }

    SortFilterProxyModel {
        id: fruitFilter
        model: fruitModel
        sorters: [
            RoleSorter {
                roleName: "name"
            }
        ]
        filters: [
            FunctionFilter {
                component CustomData: QtObject { property string name }
                property var regExp: new RegExp(fruitSearch.text, "i")
                onRegExpChanged: invalidate()
                function filter(data: CustomData): bool {
                    return regExp.test(data.name);
                }
            }
        ]
    }

    SearchField {
        id: fruitSearch
        suggestionModel: fruitFilter
        textRole: "name"
        anchors.horizontalCenter: parent.horizontalCenter
    }
    \endcode
 */

/*!
    \qmlsignal void QtQuick.Controls::SearchField::activated(int index)

    This signal is emitted when the item at \a index is activated by the user.

    An item is activated when it is selected while the popup is open,
    causing the popup to close (and \l currentIndex to change).
    The \l currentIndex property is set to \a index.

    \sa currentIndex
*/


/*!
    \qmlsignal void QtQuick.Controls::SearchField::highlighted(int index)

    This signal is emitted when the item at \a index in the popup list is highlighted by the user.

    The highlighted signal is only emitted when the popup is open and an item
    is highlighted, but not necessarily \l activated.

    \sa highlightedIndex
*/

/*!
    \qmlsignal void QtQuick.Controls::SearchField::accepted()

    This signal is emitted when the user confirms their input by pressing
    the Enter or Return key.

    This signal is typically used to trigger a search or action based on
    the final text input, and it indicates the user's intention to complete
    or submit the query.

    \sa searchTriggered()
 */

/*!
    \qmlsignal void QtQuick.Controls::SearchField::searchTriggered()

    This signal is emitted when a search action is initiated.

    It occurs in two cases:
    1. When the Enter or Return key is pressed, it will be emitted together
    with accepted() signal
    2. When the text is edited and if the \l live property is set to \c true,
    this signal will be emitted.

    This signal is ideal for initiating searches both on-demand and in real-time as
    the user types, depending on the desired interaction model.

    \sa accepted(), textEdited()
 */

/*!
    \qmlsignal void QtQuick.Controls::SearchField::textEdited()

    This signal is emitted every time the user modifies the text in the
    search field, typically with each keystroke.

    \sa searchTriggered()
 */

namespace {
    enum Activation { NoActivate, Activate };
    enum Highlighting { NoHighlight, Highlight };
}

class QQuickSearchFieldPrivate : public QQuickControlPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickSearchField)

    bool isPopupVisible() const;
    void showPopup();
    void hidePopup(bool accept);
    static void hideOldPopup(QQuickPopup *popup);
    void popupVisibleChanged();
    void popupDestroyed();

    void itemClicked();
    void itemHovered();

    void createdItem(int index, QObject *object);
    void suggestionCountChanged();

    void increaseCurrentIndex();
    void decreaseCurrentIndex();
    void setCurrentIndex(int index);
    void setCurrentItemAtIndex(int index, Activation activate);
    void updateHighlightedIndex();
    void setHighlightedIndex(int index, Highlighting highlight);

    void createDelegateModel();

    QString currentTextRole() const;
    void selectAll();
    void updateText();
    void updateDisplayText();
    QString textAt(int index) const;
    bool isValidIndex(int index) const;

    void cancelPopup();
    void executePopup(bool complete = false);

    bool handlePress(const QPointF &point, ulong timestamp) override;
    bool handleRelease(const QPointF &point, ulong timestamp) override;

    void startSearch();
    void startClear();

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    void itemDestroyed(QQuickItem *item) override;

    static inline QString popupName() { return QStringLiteral("popup"); }

    QVariant suggestionModel;
    bool hasCurrentIndex = false;
    int highlightedIndex = -1;
    int currentIndex = -1;
    QString text;
    QString textRole;
    bool live = true;
    bool searchPressed = false;
    bool clearPressed = false;
    bool searchFlat = false;
    bool clearFlat = false;
    bool searchDown = false;
    bool clearDown = false;
    bool hasSearchDown = false;
    bool hasClearDown = false;
    bool ownModel = false;
    QQmlInstanceModel *delegateModel = nullptr;
    QQmlComponent *delegate = nullptr;
    QQuickIndicatorButton *searchIndicator = nullptr;
    QQuickIndicatorButton *clearIndicator = nullptr;
    QQuickDeferredPointer<QQuickPopup> popup;
};

bool QQuickSearchFieldPrivate::isPopupVisible() const
{
    return popup && popup->isVisible();
}

void QQuickSearchFieldPrivate::showPopup()
{
    if (!popup)
        executePopup(true);

    if (popup && !popup->isVisible())
        popup->open();
}

void QQuickSearchFieldPrivate::hidePopup(bool accept)
{
    Q_Q(QQuickSearchField);
    if (accept) {
        setCurrentItemAtIndex(highlightedIndex, NoActivate);
        // hiding the popup on user interaction should always emit activated,
        // even if the current index didn't change
        emit q->activated(highlightedIndex);
    }
    if (popup && popup->isVisible())
        popup->close();
}

void QQuickSearchFieldPrivate::hideOldPopup(QQuickPopup *popup)
{
    if (!popup)
        return;

    qCDebug(lcItemManagement) << "hiding old popup" << popup;

    popup->setVisible(false);
    popup->setParentItem(nullptr);
#if QT_CONFIG(accessibility)
    // Remove the item from the accessibility tree.
    QQuickAccessibleAttached *accessible = accessibleAttached(popup);
    if (accessible)
        accessible->setIgnored(true);
#endif
}

void QQuickSearchFieldPrivate::popupVisibleChanged()
{
    if (isPopupVisible())
        QGuiApplication::inputMethod()->reset();

#if QT_CONFIG(quick_itemview)
    QQuickItemView *itemView = popup->findChild<QQuickItemView *>();
    if (itemView)
        itemView->setHighlightRangeMode(QQuickItemView::NoHighlightRange);
#endif

    updateHighlightedIndex();

#if QT_CONFIG(quick_itemview)
    if (itemView)
        itemView->positionViewAtIndex(highlightedIndex, QQuickItemView::Beginning);
#endif
}

void QQuickSearchFieldPrivate::popupDestroyed()
{
    Q_Q(QQuickSearchField);
    popup = nullptr;
    emit q->popupChanged();
}

void QQuickSearchFieldPrivate::itemClicked()
{
    Q_Q(QQuickSearchField);
    int index = delegateModel->indexOf(q->sender(), nullptr);
    if (index != -1) {
        setHighlightedIndex(index, Highlight);
        hidePopup(true);
    }
}

void QQuickSearchFieldPrivate::itemHovered()
{
    Q_Q(QQuickSearchField);

    QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(q->sender());
    if (!button || !button->isHovered() || !button->isEnabled()
        || QQuickAbstractButtonPrivate::get(button)->touchId != -1)
        return;

    int index = delegateModel->indexOf(button, nullptr);
    if (index != -1) {
        setHighlightedIndex(index, Highlight);

#if QT_CONFIG(quick_itemview)
        if (QQuickItemView *itemView = popup->findChild<QQuickItemView *>())
            itemView->positionViewAtIndex(index, QQuickItemView::Contain);
#endif
    }
}

void QQuickSearchFieldPrivate::createdItem(int index, QObject *object)
{
    Q_UNUSED(index);
    Q_Q(QQuickSearchField);
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (item && !item->parentItem()) {
        if (popup)
            item->setParentItem(popup->contentItem());
        else
            item->setParentItem(q);
        QQuickItemPrivate::get(item)->setCulled(true);
    }

    QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(object);
    if (button) {
        button->setFocusPolicy(Qt::NoFocus);
        connect(button, &QQuickAbstractButton::clicked, this,
                &QQuickSearchFieldPrivate::itemClicked);
        connect(button, &QQuickAbstractButton::hoveredChanged, this,
                &QQuickSearchFieldPrivate::itemHovered);
    }
}

void QQuickSearchFieldPrivate::suggestionCountChanged()
{
    Q_Q(QQuickSearchField);
    if (q->suggestionCount() == 0)
        setCurrentItemAtIndex(-1, NoActivate);
    // If the suggestionModel has been updated and the current text matches an item in
    // the model, update currentIndex and highlightedIndex to the index of that item.
    if (!text.isEmpty()) {
        for (int idx = 0; idx < q->suggestionCount(); ++idx) {
            QString t = textAt(idx);
            if (t == text) {
                setCurrentItemAtIndex(idx, NoActivate);
                updateHighlightedIndex();
                break;
            }
        }
    }

    emit q->suggestionCountChanged();
}

void QQuickSearchFieldPrivate::increaseCurrentIndex()
{
    Q_Q(QQuickSearchField);
    if (isPopupVisible()) {
        if (highlightedIndex < q->suggestionCount() - 1)
            setHighlightedIndex(highlightedIndex + 1, Highlight);
    }
}

void QQuickSearchFieldPrivate::decreaseCurrentIndex()
{
    if (isPopupVisible()) {
        if (highlightedIndex > 0)
            setHighlightedIndex(highlightedIndex - 1, Highlight);
    }
}

void QQuickSearchFieldPrivate::setCurrentIndex(int index)
{
    Q_Q(QQuickSearchField);
    if (currentIndex == index)
        return;

    currentIndex = index;
    emit q->currentIndexChanged();
}

void QQuickSearchFieldPrivate::setCurrentItemAtIndex(int index, Activation activate)
{
    Q_Q(QQuickSearchField);
    if (currentIndex == index)
        return;

    currentIndex = index;
    emit q->currentIndexChanged();

    updateDisplayText();

    if (activate)
        emit q->activated(index);
}

void QQuickSearchFieldPrivate::updateHighlightedIndex()
{
    Q_Q(QQuickSearchField);
    int index = -1;

    if (isPopupVisible()) {
        if (currentIndex >= 0)
            index = currentIndex;
        else if (q->suggestionCount() > 0)
            index = 0; // auto-highlight first suggestion
    }

    setHighlightedIndex(index, NoHighlight);
}

void QQuickSearchFieldPrivate::setHighlightedIndex(int index, Highlighting highlight)
{
    Q_Q(QQuickSearchField);
    if (highlightedIndex == index)
        return;

    highlightedIndex = index;
    emit q->highlightedIndexChanged();

    if (highlight)
        emit q->highlighted(index);
}

void QQuickSearchFieldPrivate::createDelegateModel()
{
    Q_Q(QQuickSearchField);
    bool ownedOldModel = ownModel;
    QQmlInstanceModel *oldModel = delegateModel;

    if (oldModel) {
        disconnect(delegateModel, &QQmlInstanceModel::countChanged, this,
                   &QQuickSearchFieldPrivate::suggestionCountChanged);
        disconnect(delegateModel, &QQmlInstanceModel::createdItem, this,
                   &QQuickSearchFieldPrivate::createdItem);
    }

    ownModel = false;
    delegateModel = suggestionModel.value<QQmlInstanceModel *>();

    if (!delegateModel && suggestionModel.isValid()) {
        QQmlDelegateModel *dataModel = new QQmlDelegateModel(qmlContext(q), q);
        dataModel->setModel(suggestionModel);
        dataModel->setDelegate(delegate);
        if (q->isComponentComplete())
            dataModel->componentComplete();

        ownModel = true;
        delegateModel = dataModel;
    }

    if (delegateModel) {
        connect(delegateModel, &QQmlInstanceModel::countChanged, this,
                &QQuickSearchFieldPrivate::suggestionCountChanged);
        connect(delegateModel, &QQmlInstanceModel::createdItem, this,
                &QQuickSearchFieldPrivate::createdItem);
    }

    emit q->delegateModelChanged();

    if (ownedOldModel)
        delete oldModel;
}

QString QQuickSearchFieldPrivate::currentTextRole() const
{
    return textRole.isEmpty() ? QStringLiteral("modelData") : textRole;
}

void QQuickSearchFieldPrivate::selectAll()
{
    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(contentItem);
    if (!input)
        return;
    input->selectAll();
}

void QQuickSearchFieldPrivate::updateText()
{
    Q_Q(QQuickSearchField);
    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(contentItem);
    if (!input)
        return;

    const QString textInput = input->text();

    if (text != textInput) {
        q->setText(textInput);
        emit q->textEdited();

        setCurrentIndex(-1);
        updateHighlightedIndex();

        if (live)
            emit q->searchTriggered();
    }

    if (text.isEmpty()) {
        if (isPopupVisible())
            hidePopup(false);
    } else {
        if (delegateModel && delegateModel->count() > 0) {
            if (!isPopupVisible())
                showPopup();
        } else {
            if (isPopupVisible())
                hidePopup(false);
        }
    }
}

void QQuickSearchFieldPrivate::updateDisplayText()
{
    Q_Q(QQuickSearchField);
    const QString currentText = textAt(currentIndex);

    if (text != currentText)
        q->setText(currentText);
}

QString QQuickSearchFieldPrivate::textAt(int index) const
{
    if (!isValidIndex(index))
        return QString();

    return delegateModel->stringValue(index, currentTextRole());
}

bool QQuickSearchFieldPrivate::isValidIndex(int index) const
{
    return delegateModel && index >= 0 && index < delegateModel->count();
}

void QQuickSearchFieldPrivate::cancelPopup()
{
    Q_Q(QQuickSearchField);
    quickCancelDeferred(q, popupName());
}

void QQuickSearchFieldPrivate::executePopup(bool complete)
{
    Q_Q(QQuickSearchField);
    if (popup.wasExecuted())
        return;

    if (!popup || complete)
        quickBeginDeferred(q, popupName(), popup);
    if (complete)
        quickCompleteDeferred(q, popupName(), popup);
}

bool QQuickSearchFieldPrivate::handlePress(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickSearchField);
    QQuickControlPrivate::handlePress(point, timestamp);

    QQuickItem *si = searchIndicator->indicator();
    QQuickItem *ci = clearIndicator->indicator();
    const bool isSearch = si && si->isEnabled() && si->contains(q->mapToItem(si, point));
    const bool isClear = ci && ci->isEnabled() && ci->contains(q->mapToItem(ci, point));

    if (isSearch) {
        searchIndicator->setPressed(true);
        startSearch();
    } else if (isClear) {
        clearIndicator->setPressed(true);
        startClear();
    }

    return true;
}

bool QQuickSearchFieldPrivate::handleRelease(const QPointF &point, ulong timestamp)
{
    QQuickControlPrivate::handleRelease(point, timestamp);
    if (searchIndicator->isPressed())
        searchIndicator->setPressed(false);
    else if (clearIndicator->isPressed())
        clearIndicator->setPressed(false);
    return true;
}

void QQuickSearchFieldPrivate::startSearch()
{
    Q_Q(QQuickSearchField);

    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(contentItem);
    if (!input)
        return;

    input->forceActiveFocus();
    emit q->searchButtonPressed();
}

void QQuickSearchFieldPrivate::startClear()
{
    Q_Q(QQuickSearchField);

    if (text.isEmpty())
        return;

    // if text is not null then clear text, also reset highlightedIndex and currentIndex
    if (!text.isEmpty()) {
        setCurrentIndex(-1);
        updateHighlightedIndex();
        q->setText(QString());

        if (isPopupVisible())
            hidePopup(false);

        emit q->clearButtonPressed();
    }
}

void QQuickSearchFieldPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    QQuickControlPrivate::itemImplicitWidthChanged(item);
    if (item == searchIndicator->indicator())
        emit searchIndicator->implicitIndicatorWidthChanged();
    if (item == clearIndicator->indicator())
        emit clearIndicator->implicitIndicatorWidthChanged();
}

void QQuickSearchFieldPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    QQuickControlPrivate::itemImplicitHeightChanged(item);
    if (item == searchIndicator->indicator())
        emit searchIndicator->implicitIndicatorHeightChanged();
    if (item == clearIndicator->indicator())
        emit clearIndicator->implicitIndicatorHeightChanged();
}

void QQuickSearchFieldPrivate::itemDestroyed(QQuickItem *item)
{
    QQuickControlPrivate::itemDestroyed(item);
    if (item == searchIndicator->indicator())
        searchIndicator->setIndicator(nullptr);
    if (item == clearIndicator->indicator())
        clearIndicator->setIndicator(nullptr);
}

QQuickSearchField::QQuickSearchField(QQuickItem *parent)
    : QQuickControl(*(new QQuickSearchFieldPrivate), parent)
{
    Q_D(QQuickSearchField);
    d->searchIndicator = new QQuickIndicatorButton(this);
    d->clearIndicator = new QQuickIndicatorButton(this);

    setFocusPolicy(Qt::StrongFocus);
    setFlag(QQuickItem::ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::LeftButton);
#if QT_CONFIG(cursor)
    setCursor(Qt::ArrowCursor);
#endif

    d->init();
}

QQuickSearchField::~QQuickSearchField()
{
    Q_D(QQuickSearchField);
    d->removeImplicitSizeListener(d->searchIndicator->indicator());
    d->removeImplicitSizeListener(d->clearIndicator->indicator());

    if (d->popup) {
        QObjectPrivate::disconnect(d->popup.data(), &QQuickPopup::visibleChanged, d,
                                   &QQuickSearchFieldPrivate::popupVisibleChanged);
        d->hideOldPopup(d->popup);
        d->popup = nullptr;
    }
}

/*!
    \qmlproperty model QtQuick.Controls::SearchField::suggestionModel

    This property holds the data model used to display search suggestions in the popup menu.

    \code
    SearchField {
        textRole: "age"
        suggestionModel: ListModel {
            ListElement { name: "Karen"; age: "66" }
            ListElement { name: "Jim"; age: "32" }
            ListElement { name: "Pamela"; age: "28" }
        }
    }
    \endcode

    \sa textRole
 */

QVariant QQuickSearchField::suggestionModel() const
{
    Q_D(const QQuickSearchField);
    return d->suggestionModel;
}

void QQuickSearchField::setSuggestionModel(const QVariant &model)
{
    Q_D(QQuickSearchField);

    QVariant suggestionModel = model;
    if (suggestionModel.userType() == qMetaTypeId<QJSValue>())
        suggestionModel = get<QJSValue>(std::move(suggestionModel)).toVariant();

    if (d->suggestionModel == suggestionModel)
        return;

    d->suggestionModel = suggestionModel;
    d->createDelegateModel();
    emit suggestionCountChanged();
    emit suggestionModelChanged();
}

/*!
    \readonly
    \qmlproperty model QtQuick.Controls::SearchField::delegateModel

    This property holds the model that provides delegate instances for the search field.

    It is typically assigned to a \l ListView in the \l {Popup::}{contentItem}
    of the \l popup.

 */
QQmlInstanceModel *QQuickSearchField::delegateModel() const
{
    Q_D(const QQuickSearchField);
    return d->delegateModel;
}

/*!
    \readonly
    \qmlproperty int QtQuick.Controls::SearchField::suggestionCount

    This property holds the number of suggestions to display from the suggestion model.
 */
int QQuickSearchField::suggestionCount() const
{
    Q_D(const QQuickSearchField);
    return d->delegateModel ? d->delegateModel->count() : 0;
}

/*!
    \qmlproperty int QtQuick.Controls::SearchField::currentIndex

    This property holds the index of the currently selected suggestion in the popup list.

    Its value is \c -1 when no suggestion is selected.

    currentIndex is not modified automatically when the model changes or when the user types
    or edits text. It is updated only when the user explicitly selects a suggestion, either
    by clicking an item in the popup, or by pressing Enter on a highlighted item.

    currentIndex can be set; for example, to display the first item in the model at startup.
    Before doing so, ensure that the model is not empty:

    \snippet qtquickcontrols-searchfield-currentIndex.qml currentIndex

    \sa activated(), text, highlightedIndex
 */
int QQuickSearchField::currentIndex() const
{
    Q_D(const QQuickSearchField);
    return d->currentIndex;
}

void QQuickSearchField::setCurrentIndex(int index)
{
    Q_D(QQuickSearchField);
    d->hasCurrentIndex = true;
    d->setCurrentIndex(index);
}

/*!
    \readonly
    \qmlproperty int QtQuick.Controls::SearchField::highlightedIndex

    This property holds the index of the currently highlighted item in
    the popup list.

    When the highlighted item is activated, the popup closes, \l currentIndex
    is updated to match \c highlightedIndex, and this property is reset to
    \c -1, indicating that no item is currently highlighted.

    \sa highlighted(), currentIndex
*/
int QQuickSearchField::highlightedIndex() const
{
    Q_D(const QQuickSearchField);
    return d->highlightedIndex;
}

/*!
    \qmlproperty string QtQuick.Controls::SearchField::text

    This property holds the current input text in the search field.

    Text is bound to the user input, triggering suggestion updates or search logic.

    \sa searchTriggered(), textEdited()
 */
QString QQuickSearchField::text() const
{
    Q_D(const QQuickSearchField);
    return d->text;
}

void QQuickSearchField::setText(const QString &text)
{
    Q_D(QQuickSearchField);
    if (d->text == text)
        return;

    d->text = text;
    emit textChanged();
}

/*!
    \qmlproperty string QtQuick.Controls::SearchField::textRole

    This property holds the model role used to display items in the suggestion model
    shown in the popup list.

    When the model has multiple roles, \c textRole can be set to determine
    which role should be displayed.
 */
QString QQuickSearchField::textRole() const
{
    Q_D(const QQuickSearchField);
    return d->textRole;
}

void QQuickSearchField::setTextRole(const QString &textRole)
{
    Q_D(QQuickSearchField);
    if (d->textRole == textRole)
        return;

    d->textRole = textRole;
}

/*!
    \qmlproperty bool QtQuick.Controls::SearchField::live

    This property holds a boolean value that determines whether the search is triggered
    on every text edit.

    When set to \c true, the \l searchTriggered() signal is emitted on each text change,
    allowing you to respond to every keystroke.
    When set to \c false, the \l searchTriggered() is only emitted when the user presses
    the Enter or Return key.

    \sa searchTriggered()
 */

bool QQuickSearchField::isLive() const
{
    Q_D(const QQuickSearchField);
    return d->live;
}

void QQuickSearchField::setLive(const bool live)
{
    Q_D(QQuickSearchField);

    if (d->live == live)
        return;

    d->live = live;
}

/*!
    \include qquickindicatorbutton.qdocinc {properties} {SearchField} {searchIndicator}

    This property holds the search indicator. Pressing it triggers
    \l searchButtonPressed().

    It is exposed so that styles and applications can customize it through its
    internal properties (for example, replacing or removing the \c searchIndicator
    via \c searchIndicator.indicator, or reacting to interaction state such as
    pressed and hovered).

    \sa {SearchField's Indicators}
 */
QQuickIndicatorButton *QQuickSearchField::searchIndicator() const
{
    Q_D(const QQuickSearchField);
    return d->searchIndicator;
}

/*!
    \include qquickindicatorbutton.qdocinc {properties} {SearchField} {clearIndicator}

    This property holds the clear indicator. Pressing it triggers
    \l clearButtonPressed().

    It is exposed so that styles and applications can customize it through its
    internal properties (for example, replacing or removing the \c clearIndicator
    via \c clearIndicator.indicator, or reacting to interaction state such as
    pressed and hovered).

    \sa {SearchField's Indicators}
*/
QQuickIndicatorButton *QQuickSearchField::clearIndicator() const
{
    Q_D(const QQuickSearchField);
    return d->clearIndicator;
}


/*!
    \qmlproperty Popup QtQuick.Controls::SearchField::popup

    This property holds the popup.

    The popup can be opened or closed manually, if necessary:

    \code
    onSpecialEvent: searchField.popup.close()
    \endcode
 */
QQuickPopup *QQuickSearchField::popup() const
{
    QQuickSearchFieldPrivate *d = const_cast<QQuickSearchFieldPrivate *>(d_func());
    if (!d->popup)
        d->executePopup(isComponentComplete());
    return d->popup;
}

void QQuickSearchField::setPopup(QQuickPopup *popup)
{
    Q_D(QQuickSearchField);
    if (d->popup == popup)
        return;

    if (!d->popup.isExecuting())
        d->cancelPopup();

    if (d->popup) {
        QObjectPrivate::disconnect(d->popup.data(), &QQuickPopup::destroyed, d,
                                   &QQuickSearchFieldPrivate::popupDestroyed);
        QObjectPrivate::disconnect(d->popup.data(), &QQuickPopup::visibleChanged, d,
                                   &QQuickSearchFieldPrivate::popupVisibleChanged);
        QQuickSearchFieldPrivate::hideOldPopup(d->popup);
    }

    if (popup) {
        QQuickPopupPrivate::get(popup)->allowVerticalFlip = true;
        popup->setClosePolicy(QQuickPopup::CloseOnEscape | QQuickPopup::CloseOnPressOutsideParent);
        QObjectPrivate::connect(popup, &QQuickPopup::visibleChanged, d,
                                &QQuickSearchFieldPrivate::popupVisibleChanged);
        // QQuickPopup does not derive from QQuickItemChangeListener, so we cannot use
        // QQuickItemChangeListener::itemDestroyed so we have to use QObject::destroyed
        QObjectPrivate::connect(popup, &QQuickPopup::destroyed, d,
                                &QQuickSearchFieldPrivate::popupDestroyed);

#if QT_CONFIG(quick_itemview)
        if (QQuickItemView *itemView = popup->findChild<QQuickItemView *>())
            itemView->setHighlightRangeMode(QQuickItemView::NoHighlightRange);
#endif
    }

    d->popup = popup;
    if (!d->popup.isExecuting())
        emit popupChanged();
}

/*!
    \qmlproperty Component QtQuick.Controls::SearchField::delegate

    This property holds a delegate that presents an item in the search field popup.

    It is recommended to use \l ItemDelegate (or any other \l AbstractButton
    derivatives) as the delegate. This ensures that the interaction works as
    expected, and the popup will automatically close when appropriate. When
    other types are used as the delegate, the popup must be closed manually.
    For example, if \l MouseArea is used:

    \code
    delegate: Rectangle {
        // ...
        MouseArea {
            // ...
            onClicked: searchField.popup.close()
        }
    }
    \endcode

    \include delegate-ownership.qdocinc {no-ownership-since-6.11} {SearchField}
*/
QQmlComponent *QQuickSearchField::delegate() const
{
    Q_D(const QQuickSearchField);
    return d->delegate;
}

void QQuickSearchField::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickSearchField);
    if (d->delegate == delegate)
        return;

    d->delegate = delegate;
    QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel *>(d->delegateModel);
    if (delegateModel)
        delegateModel->setDelegate(d->delegate);
    emit delegateChanged();
}

bool QQuickSearchField::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QQuickSearchField);

    switch (event->type()) {
    case QEvent::MouseButtonRelease: {
        QQuickTextInput *input = qobject_cast<QQuickTextInput *>(d->contentItem);
        if (input->hasFocus()) {
            if (!d->text.isEmpty() && !d->isPopupVisible() && (d->delegateModel && d->delegateModel->count() > 0))
                d->showPopup();
        }
        break;
    }
    case QEvent::FocusOut: {
        const bool hasActiveFocus = d->popup && d->popup->hasActiveFocus();
        const bool usingPopupWindows =
                d->popup ? QQuickPopupPrivate::get(d->popup)->usePopupWindow() : false;
        if (qGuiApp->focusObject() != this && !(hasActiveFocus && !usingPopupWindows))
            d->hidePopup(false);
        break;
    }
    default:
        break;
    }
    return QQuickControl::eventFilter(object, event);
}

void QQuickSearchField::focusInEvent(QFocusEvent *event)
{
    Q_D(QQuickSearchField);
    QQuickControl::focusInEvent(event);

    if ((event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason
         || event->reason() == Qt::ShortcutFocusReason)
        && d->contentItem)
        d->contentItem->forceActiveFocus(event->reason());
}

void QQuickSearchField::focusOutEvent(QFocusEvent *event)
{
    Q_D(QQuickSearchField);
    QQuickControl::focusOutEvent(event);

    const bool hasActiveFocus = d->popup && d->popup->hasActiveFocus();
    const bool usingPopupWindows = d->popup && QQuickPopupPrivate::get(d->popup)->usePopupWindow();

    if (qGuiApp->focusObject() != d->contentItem && !(hasActiveFocus && !usingPopupWindows))
        d->hidePopup(false);
}

void QQuickSearchField::hoverEnterEvent(QHoverEvent *event)
{
    Q_D(QQuickSearchField);
    QQuickControl::hoverEnterEvent(event);
    QQuickItem *si = d->searchIndicator->indicator();
    QQuickItem *ci = d->clearIndicator->indicator();
    d->searchIndicator->setHovered(si && si->isEnabled() && si->contains(mapToItem(si, event->position())));
    d->clearIndicator->setHovered(ci && ci->isEnabled() && ci->contains(mapToItem(ci, event->position())));
    event->ignore();
}

void QQuickSearchField::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(QQuickSearchField);
    QQuickControl::hoverMoveEvent(event);
    QQuickItem *si = d->searchIndicator->indicator();
    QQuickItem *ci = d->clearIndicator->indicator();
    d->searchIndicator->setHovered(si && si->isEnabled() && si->contains(mapToItem(si, event->position())));
    d->clearIndicator->setHovered(ci && ci->isEnabled() && ci->contains(mapToItem(ci, event->position())));
    event->ignore();
}

void QQuickSearchField::hoverLeaveEvent(QHoverEvent *event)
{
    Q_D(QQuickSearchField);
    QQuickControl::hoverLeaveEvent(event);
    d->searchIndicator->setHovered(false);
    d->clearIndicator->setHovered(false);
    event->ignore();
}

void QQuickSearchField::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickSearchField);

    const auto key = event->key();

    if (!d->suggestionModel.isNull() && !d->text.isEmpty()) {
        switch (key) {
        case Qt::Key_Escape:
        case Qt::Key_Back:
            if (d->isPopupVisible()) {
                d->hidePopup(false);
                event->accept();
            } else {
                setText(QString());
            }
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (d->isPopupVisible())
                d->hidePopup(true);
            emit accepted();
            emit searchTriggered();
            event->accept();
            break;
        case Qt::Key_Up:
            d->decreaseCurrentIndex();
            event->accept();
            break;
        case Qt::Key_Down:
            d->increaseCurrentIndex();
            event->accept();
            break;
        case Qt::Key_Home:
            if (d->isPopupVisible())
                d->setHighlightedIndex(0, Highlight);
            event->accept();
            break;
        case Qt::Key_End:
            if (d->isPopupVisible())
                d->setHighlightedIndex(suggestionCount() - 1, Highlight);
            event->accept();
            break;
        default:
            QQuickControl::keyPressEvent(event);
            break;
        }
    }
}

void QQuickSearchField::classBegin()
{
    Q_D(QQuickSearchField);
    QQuickControl::classBegin();

    QQmlContext *context = qmlContext(this);
    if (context) {
        QQmlEngine::setContextForObject(d->searchIndicator, context);
        QQmlEngine::setContextForObject(d->clearIndicator, context);
    }
}

void QQuickSearchField::componentComplete()
{
    Q_D(QQuickSearchField);
    QQuickIndicatorButtonPrivate::get(d->searchIndicator)->executeIndicator(true);
    QQuickIndicatorButtonPrivate::get(d->clearIndicator)->executeIndicator(true);
    QQuickControl::componentComplete();

    if (d->popup)
        d->executePopup(true);

    if (d->delegateModel && d->ownModel)
        static_cast<QQmlDelegateModel *>(d->delegateModel)->componentComplete();
}

void QQuickSearchField::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickSearchField);
    if (oldItem) {
        oldItem->removeEventFilter(this);
        if (QQuickTextInput *oldInput = qobject_cast<QQuickTextInput *>(oldItem)) {
            QObjectPrivate::disconnect(oldInput, &QQuickTextInput::textChanged, d,
                                       &QQuickSearchFieldPrivate::updateText);
        }
    }

    if (newItem) {
        newItem->installEventFilter(this);
        if (QQuickTextInput *newInput = qobject_cast<QQuickTextInput *>(newItem)) {
            QObjectPrivate::connect(newInput, &QQuickTextInput::textChanged, d,
                                    &QQuickSearchFieldPrivate::updateText);
        }
        #if QT_CONFIG(cursor)
                newItem->setCursor(Qt::IBeamCursor);
        #endif
    }
}

void QQuickSearchField::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickSearchField);
    QQuickControl::itemChange(change, data);
    if (change == ItemVisibleHasChanged && !data.boolValue) {
        d->hidePopup(false);
        d->setCurrentItemAtIndex(-1, NoActivate);
    }
}

QT_END_NAMESPACE

#include "moc_qquicksearchfield_p.cpp"
