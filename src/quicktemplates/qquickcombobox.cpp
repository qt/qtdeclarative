// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcombobox_p.h"
#include "qquickcontrol_p_p.h"
#include "qquickabstractbutton_p.h"
#include "qquickabstractbutton_p_p.h"
#include "qquickpopup_p_p.h"
#include "qquickdeferredexecute_p_p.h"

#include <QtCore/qregularexpression.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qglobal.h>
#include <QtGui/qinputmethod.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/private/qlazilyallocated_p.h>
#include <private/qqmldelegatemodel_p.h>
#include <QtQuick/private/qquickaccessibleattached_p.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuick/private/qquicktextinput_p.h>
#include <QtQuick/private/qquicktextinput_p_p.h>
#if QT_CONFIG(quick_itemview)
#include <QtQuick/private/qquickitemview_p.h>
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcCalculateWidestTextWidth, "qt.quick.controls.combobox.calculatewidesttextwidth")

/*!
    \qmltype ComboBox
    \inherits Control
//!     \instantiates QQuickComboBox
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-input
    \ingroup qtquickcontrols-focusscopes
    \brief Combined button and popup list for selecting options.

    \image qtquickcontrols-combobox.gif

    ComboBox is a combined button and popup list. It provides a means of
    presenting a list of options to the user in a way that takes up the
    minimum amount of screen space.

    ComboBox is populated with a data model. The data model is commonly
    a JavaScript array, a \l ListModel or an integer, but other types
    of \l {qml-data-models}{data models} are also supported.

    \code
    ComboBox {
        model: ["First", "Second", "Third"]
    }
    \endcode

    \section1 Editable ComboBox

    ComboBox can be made \l editable. An editable combo box auto-completes
    its text based on what is available in the model.

    The following example demonstrates appending content to an editable
    combo box by reacting to the \l accepted signal.

    \snippet qtquickcontrols-combobox-accepted.qml combobox

    \section1 ComboBox's Popup

    By default, clicking outside of ComboBox's popup will close it, and the
    event is propagated to items lower in the stacking order. To prevent the
    popup from closing, set its \l {Popup::}{closePolicy}:

    \snippet qtquickcontrols-combobox-popup.qml closePolicy

    To prevent event propagation, set its \l {Popup::}{modal} property to
    \c true:

    \snippet qtquickcontrols-combobox-popup.qml modal

    \section1 ComboBox Model Roles

    ComboBox is able to visualize standard \l {qml-data-models}{data models}
    that provide the \c modelData role:
    \list
    \li models that have only one role
    \li models that do not have named roles (JavaScript array, integer)
    \endlist

    When using models that have multiple named roles, ComboBox must be configured
    to use a specific \l {textRole}{text role} for its \l {displayText}{display text}
    and \l delegate instances. If you want to use a role of the model item
    that corresponds to the text role, set \l valueRole. The \l currentValue
    property and \l indexOfValue() method can then be used to get information
    about those values.

    For example:

    \snippet qtquickcontrols-combobox-valuerole.qml file

    \note If ComboBox is assigned a data model that has multiple named roles, but
    \l textRole is not defined, ComboBox is unable to visualize it and throws a
    \c {ReferenceError: modelData is not defined}.

    \sa {Customizing ComboBox}, {Input Controls}, {Focus Management in Qt Quick Controls}
*/

/*!
    \qmlsignal void QtQuick.Controls::ComboBox::activated(int index)

    This signal is emitted when the item at \a index is activated by the user.

    An item is activated when it is selected while the popup is open,
    causing the popup to close (and \l currentIndex to change),
    or while the popup is closed and the combo box is navigated via
    keyboard, causing the \l currentIndex to change.
    The \l currentIndex property is set to \a index.

    \sa currentIndex
*/

/*!
    \qmlsignal void QtQuick.Controls::ComboBox::highlighted(int index)

    This signal is emitted when the item at \a index in the popup list is highlighted by the user.

    The highlighted signal is only emitted when the popup is open and an item
    is highlighted, but not necessarily \l activated.

    \sa highlightedIndex
*/

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlsignal void QtQuick.Controls::ComboBox::accepted()

    This signal is emitted when the \uicontrol Return or \uicontrol Enter key is pressed
    on an \l editable combo box.

    You can handle this signal in order to add the newly entered
    item to the model, for example:

    \snippet qtquickcontrols-combobox-accepted.qml combobox

    Before the signal is emitted, a check is done to see if the string
    exists in the model. If it does, \l currentIndex will be set to its index,
    and \l currentText to the string itself.

    After the signal has been emitted, and if the first check failed (that is,
    the item did not exist), another check will be done to see if the item was
    added by the signal handler. If it was, the \l currentIndex and
    \l currentText are updated accordingly. Otherwise, they will be set to
    \c -1 and \c "", respectively.

    \note If there is a \l validator set on the combo box, the signal will only be
          emitted if the input is in an acceptable state.
*/

namespace {
    enum Activation { NoActivate, Activate };
    enum Highlighting { NoHighlight, Highlight };
}

// ### Qt7: Remove this class. Use QQmlDelegateModel instead.
class QQuickComboBoxDelegateModel : public QQmlDelegateModel
{
public:
    explicit QQuickComboBoxDelegateModel(QQuickComboBox *combo);
    QVariant variantValue(int index, const QString &role) override;

private:
    QQuickComboBox *combo = nullptr;
};

QQuickComboBoxDelegateModel::QQuickComboBoxDelegateModel(QQuickComboBox *combo)
    : QQmlDelegateModel(qmlContext(combo), combo),
      combo(combo)
{
}

QVariant QQuickComboBoxDelegateModel::variantValue(int index, const QString &role)
{
    // ### Qt7: Get rid of this. Why do we special case lists of variant maps with
    //          exactly one entry? There are many other ways of producing a list of
    //          map-like things with exactly one entry. And what if some of the maps
    //          in the list have more than one entry? You get inconsistent results.
    if (role == QLatin1String("modelData")) {
        const QVariant model = combo->model();
        if (model.metaType() == QMetaType::fromType<QVariantList>()) {
            const QVariant object = model.toList().value(index);
            if (object.metaType() == QMetaType::fromType<QVariantMap>()) {
                const QVariantMap data = object.toMap();
                if (data.size() == 1)
                    return data.first();
            }
        }
    }

    return QQmlDelegateModel::variantValue(index, role);
}

class QQuickComboBoxPrivate : public QQuickControlPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickComboBox)

    bool isPopupVisible() const;
    void showPopup();
    void hidePopup(bool accept);
    void togglePopup(bool accept);
    void popupVisibleChanged();
    void popupDestroyed();

    void itemClicked();
    void itemHovered();

    void createdItem(int index, QObject *object);
    void modelUpdated();
    void countChanged();

    QString effectiveTextRole() const;
    void updateEditText();
    void updateCurrentText();
    void updateCurrentValue();
    void updateCurrentTextAndValue();
    void updateAcceptableInput();

    bool isValidIndex(int index) const;

    void acceptInput();
    QString tryComplete(const QString &inputText);

    void incrementCurrentIndex();
    void decrementCurrentIndex();
    void setCurrentIndex(int index, Activation activate);
    void updateHighlightedIndex();
    void setHighlightedIndex(int index, Highlighting highlight);

    void keySearch(const QString &text);
    int match(int start, const QString &text, Qt::MatchFlags flags) const;

    void createDelegateModel();

    bool handlePress(const QPointF &point, ulong timestamp) override;
    bool handleMove(const QPointF &point, ulong timestamp) override;
    bool handleRelease(const QPointF &point, ulong timestamp) override;
    void handleUngrab() override;

    void cancelIndicator();
    void executeIndicator(bool complete = false);

    void cancelPopup();
    void executePopup(bool complete = false);

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    void itemDestroyed(QQuickItem *item) override;

    void setInputMethodHints(Qt::InputMethodHints hints, bool force = false);

    virtual qreal getContentWidth() const override;
    qreal calculateWidestTextWidth() const;
    void maybeUpdateImplicitContentWidth();

    static void hideOldPopup(QQuickPopup *popup);

    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::ComboBox); }

    bool flat = false;
    bool down = false;
    bool hasDown = false;
    bool pressed = false;
    bool ownModel = false;
    bool keyNavigating = false;
    bool hasDisplayText = false;
    bool hasCurrentIndex = false;
    bool hasCalculatedWidestText = false;
    int highlightedIndex = -1;
    int currentIndex = -1;
    QQuickComboBox::ImplicitContentWidthPolicy implicitContentWidthPolicy = QQuickComboBox::ContentItemImplicitWidth;
    QVariant model;
    QString textRole;
    QString currentText;
    QString displayText;
    QString valueRole;
    QVariant currentValue;
    QQuickItem *pressedItem = nullptr;
    QQmlInstanceModel *delegateModel = nullptr;
    QQmlComponent *delegate = nullptr;
    QQuickDeferredPointer<QQuickItem> indicator;
    QQuickDeferredPointer<QQuickPopup> popup;
    bool m_acceptableInput = true;

    struct ExtraData {
        bool editable = false;
        bool accepting = false;
        bool allowComplete = false;
        bool selectTextByMouse = false;
        Qt::InputMethodHints inputMethodHints = Qt::ImhNone;
        QString editText;
#if QT_CONFIG(validator)
        QValidator *validator = nullptr;
#endif
    };
    QLazilyAllocated<ExtraData> extra;
};

bool QQuickComboBoxPrivate::isPopupVisible() const
{
    return popup && popup->isVisible();
}

void QQuickComboBoxPrivate::showPopup()
{
    if (!popup)
        executePopup(true);

    if (popup && !popup->isVisible())
        popup->open();
}

void QQuickComboBoxPrivate::hidePopup(bool accept)
{
    Q_Q(QQuickComboBox);
    if (accept) {
        q->setCurrentIndex(highlightedIndex);
        emit q->activated(currentIndex);
    }
    if (popup && popup->isVisible())
        popup->close();
}

void QQuickComboBoxPrivate::togglePopup(bool accept)
{
    if (!popup || !popup->isVisible())
        showPopup();
    else
        hidePopup(accept);
}

void QQuickComboBoxPrivate::popupVisibleChanged()
{
    Q_Q(QQuickComboBox);
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

    if (!hasDown) {
        q->setDown(pressed || isPopupVisible());
        hasDown = false;
    }
}

void QQuickComboBoxPrivate::popupDestroyed()
{
    Q_Q(QQuickComboBox);
    popup = nullptr;
    emit q->popupChanged();
}

void QQuickComboBoxPrivate::itemClicked()
{
    Q_Q(QQuickComboBox);
    int index = delegateModel->indexOf(q->sender(), nullptr);
    if (index != -1) {
        setHighlightedIndex(index, Highlight);
        hidePopup(true);
    }
}

void QQuickComboBoxPrivate::itemHovered()
{
    Q_Q(QQuickComboBox);
    if (keyNavigating)
        return;

    QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(q->sender());
    if (!button || !button->isHovered() || !button->isEnabled() || QQuickAbstractButtonPrivate::get(button)->touchId != -1)
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

void QQuickComboBoxPrivate::createdItem(int index, QObject *object)
{
    Q_Q(QQuickComboBox);
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
        connect(button, &QQuickAbstractButton::clicked, this, &QQuickComboBoxPrivate::itemClicked);
        connect(button, &QQuickAbstractButton::hoveredChanged, this, &QQuickComboBoxPrivate::itemHovered);
    }

    if (index == currentIndex && !q->isEditable())
        updateCurrentTextAndValue();
}

void QQuickComboBoxPrivate::modelUpdated()
{
    if (componentComplete && (!extra.isAllocated() || !extra->accepting)) {
        updateCurrentTextAndValue();

        if (implicitContentWidthPolicy == QQuickComboBox::WidestText)
            updateImplicitContentSize();
    }
}

void QQuickComboBoxPrivate::countChanged()
{
    Q_Q(QQuickComboBox);
    if (q->count() == 0)
        q->setCurrentIndex(-1);
    emit q->countChanged();
}

QString QQuickComboBoxPrivate::effectiveTextRole() const
{
    return textRole.isEmpty() ? QStringLiteral("modelData") : textRole;
}

void QQuickComboBoxPrivate::updateEditText()
{
    Q_Q(QQuickComboBox);
    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(contentItem);
    if (!input)
        return;

    const QString text = input->text();

    if (extra.isAllocated() && extra->allowComplete && !text.isEmpty()) {
        const QString completed = tryComplete(text);
        if (completed.size() > text.size()) {
            input->setText(completed);
            // This will select the text backwards.
            input->select(completed.size(), text.size());
            return;
        }
    }
    q->setEditText(text);
}

void QQuickComboBoxPrivate::updateCurrentText()
{
    Q_Q(QQuickComboBox);
    const QString text = q->textAt(currentIndex);
    if (currentText != text) {
        currentText = text;
        if (!hasDisplayText)
           q->maybeSetAccessibleName(text);
        emit q->currentTextChanged();
    }
    if (!hasDisplayText && displayText != text) {
        displayText = text;
        emit q->displayTextChanged();
    }
    if (!extra.isAllocated() || !extra->accepting)
        q->setEditText(currentText);
}

void QQuickComboBoxPrivate::updateCurrentValue()
{
    Q_Q(QQuickComboBox);
    const QVariant value = q->valueAt(currentIndex);
    if (currentValue == value)
        return;

    currentValue = value;
    emit q->currentValueChanged();
}

void QQuickComboBoxPrivate::updateCurrentTextAndValue()
{
    updateCurrentText();
    updateCurrentValue();
}

void QQuickComboBoxPrivate::updateAcceptableInput()
{
    Q_Q(QQuickComboBox);

    if (!contentItem)
        return;

    const QQuickTextInput *textInputContentItem = qobject_cast<QQuickTextInput *>(contentItem);

    if (!textInputContentItem)
        return;

    const bool newValue = textInputContentItem->hasAcceptableInput();

    if (m_acceptableInput != newValue) {
        m_acceptableInput = newValue;
        emit q->acceptableInputChanged();
    }
}

bool QQuickComboBoxPrivate::isValidIndex(int index) const
{
    return delegateModel && index >= 0 && index < delegateModel->count();
}

void QQuickComboBoxPrivate::acceptInput()
{
    Q_Q(QQuickComboBox);
    int idx = q->find(extra.value().editText, Qt::MatchFixedString);
    if (idx > -1) {
        // The item that was accepted already exists, so make it the current item.
        q->setCurrentIndex(idx);
        // After accepting text that matches an existing entry, the selection should be cleared.
        QQuickTextInput *input = qobject_cast<QQuickTextInput *>(contentItem);
        if (input) {
            const auto text = input->text();
            input->select(text.size(), text.size());
        }
    }

    extra.value().accepting = true;
    emit q->accepted();

    // The user might have added the item since it didn't exist, so check again
    // to see if we can select that new item.
    if (idx == -1)
        q->setCurrentIndex(q->find(extra.value().editText, Qt::MatchFixedString));
    extra.value().accepting = false;
}

QString QQuickComboBoxPrivate::tryComplete(const QString &input)
{
    Q_Q(QQuickComboBox);
    QString match;

    const int itemCount = q->count();
    for (int idx = 0; idx < itemCount; ++idx) {
        const QString text = q->textAt(idx);
        if (!text.startsWith(input, Qt::CaseInsensitive))
            continue;

        // either the first or the shortest match
        if (match.isEmpty() || text.size() < match.size())
            match = text;
    }

    if (match.isEmpty())
        return input;

    return input + match.mid(input.size());
}

void QQuickComboBoxPrivate::setCurrentIndex(int index, Activation activate)
{
    Q_Q(QQuickComboBox);
    if (currentIndex == index)
        return;

    currentIndex = index;
    emit q->currentIndexChanged();

    if (componentComplete)
        updateCurrentTextAndValue();

    if (activate)
        emit q->activated(index);
}

void QQuickComboBoxPrivate::incrementCurrentIndex()
{
    Q_Q(QQuickComboBox);
    if (extra.isAllocated())
        extra->allowComplete = false;
    if (isPopupVisible()) {
        if (highlightedIndex < q->count() - 1)
            setHighlightedIndex(highlightedIndex + 1, Highlight);
    } else {
        if (currentIndex < q->count() - 1)
            setCurrentIndex(currentIndex + 1, Activate);
    }
    if (extra.isAllocated())
        extra->allowComplete = true;
}

void QQuickComboBoxPrivate::decrementCurrentIndex()
{
    if (extra.isAllocated())
        extra->allowComplete = false;
    if (isPopupVisible()) {
        if (highlightedIndex > 0)
            setHighlightedIndex(highlightedIndex - 1, Highlight);
    } else {
        if (currentIndex > 0)
            setCurrentIndex(currentIndex - 1, Activate);
    }
    if (extra.isAllocated())
        extra->allowComplete = true;
}

void QQuickComboBoxPrivate::updateHighlightedIndex()
{
    setHighlightedIndex(popup->isVisible() ? currentIndex : -1, NoHighlight);
}

void QQuickComboBoxPrivate::setHighlightedIndex(int index, Highlighting highlight)
{
    Q_Q(QQuickComboBox);
    if (highlightedIndex == index)
        return;

    highlightedIndex = index;
    emit q->highlightedIndexChanged();

    if (highlight)
        emit q->highlighted(index);
}

void QQuickComboBoxPrivate::keySearch(const QString &text)
{
    const int startIndex = isPopupVisible() ? highlightedIndex : currentIndex;
    const int index = match(startIndex + 1, text, Qt::MatchStartsWith | Qt::MatchWrap);
    if (index != -1) {
        if (isPopupVisible())
            setHighlightedIndex(index, Highlight);
        else
            setCurrentIndex(index, Activate);
    }
}

int QQuickComboBoxPrivate::match(int start, const QString &text, Qt::MatchFlags flags) const
{
    Q_Q(const QQuickComboBox);
    uint matchType = flags & 0x0F;
    bool wrap = flags & Qt::MatchWrap;
    Qt::CaseSensitivity cs = flags & Qt::MatchCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QRegularExpression::PatternOptions options = flags & Qt::MatchCaseSensitive ? QRegularExpression::NoPatternOption
                                                                                : QRegularExpression::CaseInsensitiveOption;
    int from = start;
    int to = q->count();

    // iterates twice if wrapping
    for (int i = 0; (wrap && i < 2) || (!wrap && i < 1); ++i) {
        for (int idx = from; idx < to; ++idx) {
            QString t = q->textAt(idx);
            switch (matchType) {
            case Qt::MatchExactly:
                if (t == text)
                    return idx;
                break;
            case Qt::MatchRegularExpression: {
                QRegularExpression rx(QRegularExpression::anchoredPattern(text), options);
                if (rx.match(t).hasMatch())
                    return idx;
                break;
            }
            case Qt::MatchWildcard: {
                QRegularExpression rx(QRegularExpression::wildcardToRegularExpression(text),
                                      options);
                if (rx.match(t).hasMatch())
                    return idx;
                break;
            }
            case Qt::MatchStartsWith:
                if (t.startsWith(text, cs))
                    return idx;
                break;
            case Qt::MatchEndsWith:
                if (t.endsWith(text, cs))
                    return idx;
                break;
            case Qt::MatchFixedString:
                if (t.compare(text, cs) == 0)
                    return idx;
                break;
            case Qt::MatchContains:
            default:
                if (t.contains(text, cs))
                    return idx;
                break;
            }
        }
        // prepare for the next iteration
        from = 0;
        to = start;
    }
    return -1;
}

void QQuickComboBoxPrivate::createDelegateModel()
{
    Q_Q(QQuickComboBox);
    bool ownedOldModel = ownModel;
    QQmlInstanceModel* oldModel = delegateModel;
    if (oldModel) {
        disconnect(delegateModel, &QQmlInstanceModel::countChanged, this, &QQuickComboBoxPrivate::countChanged);
        disconnect(delegateModel, &QQmlInstanceModel::modelUpdated, this, &QQuickComboBoxPrivate::modelUpdated);
        disconnect(delegateModel, &QQmlInstanceModel::createdItem, this, &QQuickComboBoxPrivate::createdItem);
    }

    ownModel = false;
    delegateModel = model.value<QQmlInstanceModel *>();

    if (!delegateModel && model.isValid()) {
        QQmlDelegateModel *dataModel = new QQuickComboBoxDelegateModel(q);
        dataModel->setModel(model);
        dataModel->setDelegate(delegate);
        if (q->isComponentComplete())
            dataModel->componentComplete();

        ownModel = true;
        delegateModel = dataModel;
    }

    if (delegateModel) {
        connect(delegateModel, &QQmlInstanceModel::countChanged, this, &QQuickComboBoxPrivate::countChanged);
        connect(delegateModel, &QQmlInstanceModel::modelUpdated, this, &QQuickComboBoxPrivate::modelUpdated);
        connect(delegateModel, &QQmlInstanceModel::createdItem, this, &QQuickComboBoxPrivate::createdItem);
    }

    emit q->delegateModelChanged();

    if (ownedOldModel)
        delete oldModel;
}

bool QQuickComboBoxPrivate::handlePress(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickComboBox);
    QQuickControlPrivate::handlePress(point, timestamp);
    q->setPressed(true);
    return true;
}

bool QQuickComboBoxPrivate::handleMove(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickComboBox);
    QQuickControlPrivate::handleMove(point, timestamp);
    q->setPressed(q->contains(point));
    return true;
}

bool QQuickComboBoxPrivate::handleRelease(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickComboBox);
    QQuickControlPrivate::handleRelease(point, timestamp);
    if (pressed) {
        q->setPressed(false);
        togglePopup(false);
    }
    return true;
}

void QQuickComboBoxPrivate::handleUngrab()
{
    Q_Q(QQuickComboBox);
    QQuickControlPrivate::handleUngrab();
    q->setPressed(false);
}

void QQuickComboBoxPrivate::cancelIndicator()
{
    Q_Q(QQuickComboBox);
    quickCancelDeferred(q, indicatorName());
}

void QQuickComboBoxPrivate::executeIndicator(bool complete)
{
    Q_Q(QQuickComboBox);
    if (indicator.wasExecuted())
        return;

    if (!indicator || complete)
        quickBeginDeferred(q, indicatorName(), indicator);
    if (complete)
        quickCompleteDeferred(q, indicatorName(), indicator);
}

static inline QString popupName() { return QStringLiteral("popup"); }

void QQuickComboBoxPrivate::cancelPopup()
{
    Q_Q(QQuickComboBox);
    quickCancelDeferred(q, popupName());
}

void QQuickComboBoxPrivate::executePopup(bool complete)
{
    Q_Q(QQuickComboBox);
    if (popup.wasExecuted())
        return;

    if (!popup || complete)
        quickBeginDeferred(q, popupName(), popup);
    if (complete)
        quickCompleteDeferred(q, popupName(), popup);
}

void QQuickComboBoxPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    Q_Q(QQuickComboBox);
    QQuickControlPrivate::itemImplicitWidthChanged(item);
    if (item == indicator)
        emit q->implicitIndicatorWidthChanged();
}

void QQuickComboBoxPrivate::setInputMethodHints(Qt::InputMethodHints hints, bool force)
{
    Q_Q(QQuickComboBox);
    if (!force && hints == q->inputMethodHints())
        return;

    extra.value().inputMethodHints = hints;
    emit q->inputMethodHintsChanged();
}

void QQuickComboBoxPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    Q_Q(QQuickComboBox);
    QQuickControlPrivate::itemImplicitHeightChanged(item);
    if (item == indicator)
        emit q->implicitIndicatorHeightChanged();
}

void QQuickComboBoxPrivate::itemDestroyed(QQuickItem *item)
{
    Q_Q(QQuickComboBox);
    QQuickControlPrivate::itemDestroyed(item);
    if (item == indicator) {
        indicator = nullptr;
        emit q->indicatorChanged();
    }
}

qreal QQuickComboBoxPrivate::getContentWidth() const
{
    if (componentComplete) {
        switch (implicitContentWidthPolicy) {
        case QQuickComboBox::WidestText:
            return calculateWidestTextWidth();
        case QQuickComboBox::WidestTextWhenCompleted:
            if (!hasCalculatedWidestText)
                return calculateWidestTextWidth();
            break;
        default:
            break;
        }
    }

    return QQuickControlPrivate::getContentWidth();
}

qreal QQuickComboBoxPrivate::calculateWidestTextWidth() const
{
    Q_Q(const QQuickComboBox);
    if (!componentComplete)
        return 0;

    const int count = q->count();
    if (count == 0)
        return 0;

    auto textInput = qobject_cast<QQuickTextInput*>(contentItem);
    if (!textInput)
        return 0;

    qCDebug(lcCalculateWidestTextWidth) << "calculating widest text from" << count << "items...";

    // Avoid the index check and repeated calls to effectiveTextRole()
    // that would result from calling textAt() in a loop.
    const QString textRole = effectiveTextRole();
    auto textInputPrivate = QQuickTextInputPrivate::get(textInput);
    qreal widest = 0;
    for (int i = 0; i < count; ++i) {
        const QString text = delegateModel->stringValue(i, textRole);
        const qreal textImplicitWidth = textInputPrivate->calculateImplicitWidthForText(text);
        widest = qMax(widest, textImplicitWidth);
    }

    qCDebug(lcCalculateWidestTextWidth) << "... widest text is" << widest;
    return widest;
}

/*!
    \internal

    If the user requested it (and we haven't already done it, depending on the policy),
    update the implicit content width to the largest text in the model.
*/
void QQuickComboBoxPrivate::maybeUpdateImplicitContentWidth()
{
    if (!componentComplete)
        return;

    if (implicitContentWidthPolicy == QQuickComboBox::ContentItemImplicitWidth
        || (implicitContentWidthPolicy == QQuickComboBox::WidestTextWhenCompleted && hasCalculatedWidestText))
        return;

    updateImplicitContentWidth();
    hasCalculatedWidestText = true;
}

void QQuickComboBoxPrivate::hideOldPopup(QQuickPopup *popup)
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

QQuickComboBox::QQuickComboBox(QQuickItem *parent)
    : QQuickControl(*(new QQuickComboBoxPrivate), parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setFlag(QQuickItem::ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::LeftButton);
#if QT_CONFIG(cursor)
    setCursor(Qt::ArrowCursor);
#endif
    Q_D(QQuickComboBox);
    d->setInputMethodHints(Qt::ImhNoPredictiveText, true);
}

QQuickComboBox::~QQuickComboBox()
{
    Q_D(QQuickComboBox);
    d->removeImplicitSizeListener(d->indicator);
    if (d->popup) {
        // Disconnect visibleChanged() to avoid a spurious highlightedIndexChanged() signal
        // emission during the destruction of the (visible) popup. (QTBUG-57650)
        QObjectPrivate::disconnect(d->popup.data(), &QQuickPopup::visibleChanged, d, &QQuickComboBoxPrivate::popupVisibleChanged);
        QQuickComboBoxPrivate::hideOldPopup(d->popup);
        d->popup = nullptr;
    }
}

/*!
    \readonly
    \qmlproperty int QtQuick.Controls::ComboBox::count

    This property holds the number of items in the combo box.
*/
int QQuickComboBox::count() const
{
    Q_D(const QQuickComboBox);
    return d->delegateModel ? d->delegateModel->count() : 0;
}

/*!
    \qmlproperty model QtQuick.Controls::ComboBox::model

    This property holds the model providing data for the combo box.

    \code
    ComboBox {
        textRole: "key"
        model: ListModel {
            ListElement { key: "First"; value: 123 }
            ListElement { key: "Second"; value: 456 }
            ListElement { key: "Third"; value: 789 }
        }
    }
    \endcode

    \sa textRole, {qml-data-models}{Data Models}
*/
QVariant QQuickComboBox::model() const
{
    Q_D(const QQuickComboBox);
    return d->model;
}

void QQuickComboBox::setModel(const QVariant& m)
{
    Q_D(QQuickComboBox);
    QVariant model = m;
    if (model.userType() == qMetaTypeId<QJSValue>())
        model = model.value<QJSValue>().toVariant();

    if (d->model == model)
        return;

    if (QAbstractItemModel* aim = qvariant_cast<QAbstractItemModel *>(d->model)) {
        QObjectPrivate::disconnect(aim, &QAbstractItemModel::dataChanged,
            d, QOverload<>::of(&QQuickComboBoxPrivate::updateCurrentTextAndValue));
    }
    if (QAbstractItemModel* aim = qvariant_cast<QAbstractItemModel *>(model)) {
        QObjectPrivate::connect(aim, &QAbstractItemModel::dataChanged,
            d, QOverload<>::of(&QQuickComboBoxPrivate::updateCurrentTextAndValue));
    }

    d->model = model;
    d->createDelegateModel();
    emit countChanged();
    if (isComponentComplete()) {
        setCurrentIndex(count() > 0 ? 0 : -1);
        d->updateCurrentTextAndValue();
    }
    emit modelChanged();

    d->maybeUpdateImplicitContentWidth();
}

/*!
    \internal
    \qmlproperty model QtQuick.Controls::ComboBox::delegateModel

    This property holds the model providing delegate instances for the combo box.
*/
QQmlInstanceModel *QQuickComboBox::delegateModel() const
{
    Q_D(const QQuickComboBox);
    return d->delegateModel;
}


/*!
    \readonly
    \qmlproperty bool QtQuick.Controls::ComboBox::pressed

    This property holds whether the combo box button is physically pressed.
    A button can be pressed by either touch or key events.

    \sa down
*/
bool QQuickComboBox::isPressed() const
{
    Q_D(const QQuickComboBox);
    return d->pressed;
}

void QQuickComboBox::setPressed(bool pressed)
{
    Q_D(QQuickComboBox);
    if (d->pressed == pressed)
        return;

    d->pressed = pressed;
    emit pressedChanged();

    if (!d->hasDown) {
        setDown(d->pressed || d->isPopupVisible());
        d->hasDown = false;
    }
}

/*!
    \readonly
    \qmlproperty int QtQuick.Controls::ComboBox::highlightedIndex

    This property holds the index of the highlighted item in the combo box popup list.

    When a highlighted item is activated, the popup is closed, \l currentIndex
    is set to \c highlightedIndex, and the value of this property is reset to
    \c -1, as there is no longer a highlighted item.

    \sa highlighted(), currentIndex
*/
int QQuickComboBox::highlightedIndex() const
{
    Q_D(const QQuickComboBox);
    return d->highlightedIndex;
}

/*!
    \qmlproperty int QtQuick.Controls::ComboBox::currentIndex

    This property holds the index of the current item in the combo box.

    The default value is \c -1 when \l count is \c 0, and \c 0 otherwise.

    \sa activated(), currentText, highlightedIndex
*/
int QQuickComboBox::currentIndex() const
{
    Q_D(const QQuickComboBox);
    return d->currentIndex;
}

void QQuickComboBox::setCurrentIndex(int index)
{
    Q_D(QQuickComboBox);
    d->hasCurrentIndex = true;
    d->setCurrentIndex(index, NoActivate);
}

/*!
    \readonly
    \qmlproperty string QtQuick.Controls::ComboBox::currentText

    This property holds the text of the current item in the combo box.

    \sa currentIndex, displayText, textRole, editText
*/
QString QQuickComboBox::currentText() const
{
    Q_D(const QQuickComboBox);
    return d->currentText;
}

/*!
    \qmlproperty string QtQuick.Controls::ComboBox::displayText

    This property holds the text that is displayed on the combo box button.

    By default, the display text presents the current selection. That is,
    it follows the text of the current item. However, the default display
    text can be overridden with a custom value.

    \code
    ComboBox {
        currentIndex: 1
        displayText: "Size: " + currentText
        model: ["S", "M", "L"]
    }
    \endcode

    \sa currentText, textRole
*/
QString QQuickComboBox::displayText() const
{
    Q_D(const QQuickComboBox);
    return d->displayText;
}

void QQuickComboBox::setDisplayText(const QString &text)
{
    Q_D(QQuickComboBox);
    d->hasDisplayText = true;
    if (d->displayText == text)
        return;

    d->displayText = text;
    maybeSetAccessibleName(text);
    emit displayTextChanged();
}

void QQuickComboBox::resetDisplayText()
{
    Q_D(QQuickComboBox);
    if (!d->hasDisplayText)
        return;

    d->hasDisplayText = false;
    d->updateCurrentText();
}


/*!
    \qmlproperty string QtQuick.Controls::ComboBox::textRole

    This property holds the model role used for populating the combo box.

    When the model has multiple roles, \c textRole can be set to determine
    which role should be displayed.

    \sa model, currentText, displayText, {ComboBox Model Roles}
*/
QString QQuickComboBox::textRole() const
{
    Q_D(const QQuickComboBox);
    return d->textRole;
}

void QQuickComboBox::setTextRole(const QString &role)
{
    Q_D(QQuickComboBox);
    if (d->textRole == role)
        return;

    d->textRole = role;
    if (isComponentComplete())
        d->updateCurrentText();
    emit textRoleChanged();
}

/*!
    \since QtQuick.Controls 2.14 (Qt 5.14)
    \qmlproperty string QtQuick.Controls::ComboBox::valueRole

    This property holds the model role used for storing the value associated
    with each item in the model.

    For an example of how to use this property, see \l {ComboBox Model Roles}.

    \sa model, currentValue
*/
QString QQuickComboBox::valueRole() const
{
    Q_D(const QQuickComboBox);
    return d->valueRole;
}

void QQuickComboBox::setValueRole(const QString &role)
{
    Q_D(QQuickComboBox);
    if (d->valueRole == role)
        return;

    d->valueRole = role;
    if (isComponentComplete())
        d->updateCurrentValue();
    emit valueRoleChanged();
}

/*!
    \qmlproperty Component QtQuick.Controls::ComboBox::delegate

    This property holds a delegate that presents an item in the combo box popup.

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
            onClicked: comboBox.popup.close()
        }
    }
    \endcode

    \sa ItemDelegate, {Customizing ComboBox}
*/
QQmlComponent *QQuickComboBox::delegate() const
{
    Q_D(const QQuickComboBox);
    return d->delegate;
}

void QQuickComboBox::setDelegate(QQmlComponent* delegate)
{
    Q_D(QQuickComboBox);
    if (d->delegate == delegate)
        return;

    delete d->delegate;
    d->delegate = delegate;
    QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel*>(d->delegateModel);
    if (delegateModel)
        delegateModel->setDelegate(d->delegate);
    emit delegateChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::ComboBox::indicator

    This property holds the drop indicator item.

    \sa {Customizing ComboBox}
*/
QQuickItem *QQuickComboBox::indicator() const
{
    QQuickComboBoxPrivate *d = const_cast<QQuickComboBoxPrivate *>(d_func());
    if (!d->indicator)
        d->executeIndicator();
    return d->indicator;
}

void QQuickComboBox::setIndicator(QQuickItem *indicator)
{
    Q_D(QQuickComboBox);
    if (d->indicator == indicator)
        return;

    QQuickControlPrivate::warnIfCustomizationNotSupported(this, indicator, QStringLiteral("indicator"));

    if (!d->indicator.isExecuting())
        d->cancelIndicator();

    const qreal oldImplicitIndicatorWidth = implicitIndicatorWidth();
    const qreal oldImplicitIndicatorHeight = implicitIndicatorHeight();

    d->removeImplicitSizeListener(d->indicator);
    QQuickControlPrivate::hideOldItem(d->indicator);
    d->indicator = indicator;
    if (indicator) {
        if (!indicator->parentItem())
            indicator->setParentItem(this);
        d->addImplicitSizeListener(indicator);
    }

    if (!qFuzzyCompare(oldImplicitIndicatorWidth, implicitIndicatorWidth()))
        emit implicitIndicatorWidthChanged();
    if (!qFuzzyCompare(oldImplicitIndicatorHeight, implicitIndicatorHeight()))
        emit implicitIndicatorHeightChanged();
    if (!d->indicator.isExecuting())
        emit indicatorChanged();
}

/*!
    \qmlproperty Popup QtQuick.Controls::ComboBox::popup

    This property holds the popup.

    The popup can be opened or closed manually, if necessary:

    \code
    onSpecialEvent: comboBox.popup.close()
    \endcode

    \sa {Customizing ComboBox}
*/
QQuickPopup *QQuickComboBox::popup() const
{
    QQuickComboBoxPrivate *d = const_cast<QQuickComboBoxPrivate *>(d_func());
    if (!d->popup)
        d->executePopup(isComponentComplete());
    return d->popup;
}

void QQuickComboBox::setPopup(QQuickPopup *popup)
{
    Q_D(QQuickComboBox);
    if (d->popup == popup)
        return;

    if (!d->popup.isExecuting())
        d->cancelPopup();

    if (d->popup) {
        QObjectPrivate::disconnect(d->popup.data(), &QQuickPopup::destroyed, d, &QQuickComboBoxPrivate::popupDestroyed);
        QObjectPrivate::disconnect(d->popup.data(), &QQuickPopup::visibleChanged, d, &QQuickComboBoxPrivate::popupVisibleChanged);
        QQuickComboBoxPrivate::hideOldPopup(d->popup);
    }
    if (popup) {
        QQuickPopupPrivate::get(popup)->allowVerticalFlip = true;
        popup->setClosePolicy(QQuickPopup::CloseOnEscape | QQuickPopup::CloseOnPressOutsideParent);
        QObjectPrivate::connect(popup, &QQuickPopup::visibleChanged, d, &QQuickComboBoxPrivate::popupVisibleChanged);
        // QQuickPopup does not derive from QQuickItemChangeListener, so we cannot use
        // QQuickItemChangeListener::itemDestroyed so we have to use QObject::destroyed
        QObjectPrivate::connect(popup, &QQuickPopup::destroyed, d, &QQuickComboBoxPrivate::popupDestroyed);

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
    \since QtQuick.Controls 2.1 (Qt 5.8)
    \qmlproperty bool QtQuick.Controls::ComboBox::flat

    This property holds whether the combo box button is flat.

    A flat combo box button does not draw a background unless it is interacted
    with. In comparison to normal combo boxes, flat combo boxes provide looks
    that make them stand out less from the rest of the UI. For instance, when
    placing a combo box into a tool bar, it may be desirable to make the combo
    box flat so it matches better with the flat looks of tool buttons.

    The default value is \c false.
*/
bool QQuickComboBox::isFlat() const
{
    Q_D(const QQuickComboBox);
    return d->flat;
}

void QQuickComboBox::setFlat(bool flat)
{
    Q_D(QQuickComboBox);
    if (d->flat == flat)
        return;

    d->flat = flat;
    emit flatChanged();
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty bool QtQuick.Controls::ComboBox::down

    This property holds whether the combo box button is visually down.

    Unless explicitly set, this property is \c true when either \c pressed
    or \c popup.visible is \c true. To return to the default value, set this
    property to \c undefined.

    \sa pressed, popup
*/
bool QQuickComboBox::isDown() const
{
    Q_D(const QQuickComboBox);
    return d->down;
}

void QQuickComboBox::setDown(bool down)
{
    Q_D(QQuickComboBox);
    d->hasDown = true;

    if (d->down == down)
        return;

    d->down = down;
    emit downChanged();
}

void QQuickComboBox::resetDown()
{
    Q_D(QQuickComboBox);
    if (!d->hasDown)
        return;

    setDown(d->pressed || d->isPopupVisible());
    d->hasDown = false;
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty bool QtQuick.Controls::ComboBox::editable

    This property holds whether the combo box is editable.

    The default value is \c false.

    \sa validator
*/
bool QQuickComboBox::isEditable() const
{
    Q_D(const QQuickComboBox);
    return d->extra.isAllocated() && d->extra->editable;
}

void QQuickComboBox::setEditable(bool editable)
{
    Q_D(QQuickComboBox);
    if (editable == isEditable())
        return;

    if (d->contentItem) {
        if (editable) {
            d->contentItem->installEventFilter(this);
            if (QQuickTextInput *input = qobject_cast<QQuickTextInput *>(d->contentItem)) {
                QObjectPrivate::connect(input, &QQuickTextInput::textChanged, d, &QQuickComboBoxPrivate::updateEditText);
                QObjectPrivate::connect(input, &QQuickTextInput::accepted, d, &QQuickComboBoxPrivate::acceptInput);
            }
#if QT_CONFIG(cursor)
            d->contentItem->setCursor(Qt::IBeamCursor);
#endif
        } else {
            d->contentItem->removeEventFilter(this);
            if (QQuickTextInput *input = qobject_cast<QQuickTextInput *>(d->contentItem)) {
                QObjectPrivate::disconnect(input, &QQuickTextInput::textChanged, d, &QQuickComboBoxPrivate::updateEditText);
                QObjectPrivate::disconnect(input, &QQuickTextInput::accepted, d, &QQuickComboBoxPrivate::acceptInput);
            }
#if QT_CONFIG(cursor)
            d->contentItem->unsetCursor();
#endif
        }
    }

    d->extra.value().editable = editable;
    setAccessibleProperty("editable", editable);
    emit editableChanged();
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty string QtQuick.Controls::ComboBox::editText

    This property holds the text in the text field of an editable combo box.

    \sa editable, currentText, displayText
*/
QString QQuickComboBox::editText() const
{
    Q_D(const QQuickComboBox);
    return d->extra.isAllocated() ? d->extra->editText : QString();
}

void QQuickComboBox::setEditText(const QString &text)
{
    Q_D(QQuickComboBox);
    if (text == editText())
        return;

    d->extra.value().editText = text;
    emit editTextChanged();
}

void QQuickComboBox::resetEditText()
{
    setEditText(QString());
}

#if QT_CONFIG(validator)
/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty Validator QtQuick.Controls::ComboBox::validator

    This property holds an input text validator for an editable combo box.

    When a validator is set, the text field will only accept input which
    leaves the text property in an intermediate state. The \l accepted signal
    will only be emitted if the text is in an acceptable state when the
    \uicontrol Return or \uicontrol Enter key is pressed.

    The currently supported validators are \l[QtQuick]{IntValidator},
    \l[QtQuick]{DoubleValidator}, and \l[QtQuick]{RegularExpressionValidator}. An
    example of using validators is shown below, which allows input of
    integers between \c 0 and \c 10 into the text field:

    \code
    ComboBox {
        model: 10
        editable: true
        validator: IntValidator {
            top: 9
            bottom: 0
        }
    }
    \endcode

    \sa acceptableInput, accepted, editable
*/
QValidator *QQuickComboBox::validator() const
{
    Q_D(const QQuickComboBox);
    return d->extra.isAllocated() ? d->extra->validator : nullptr;
}

void QQuickComboBox::setValidator(QValidator *validator)
{
    Q_D(QQuickComboBox);
    if (validator == QQuickComboBox::validator())
        return;

    d->extra.value().validator = validator;
#if QT_CONFIG(validator)
    if (validator)
        validator->setLocale(d->locale);
#endif
    emit validatorChanged();
}
#endif

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty flags QtQuick.Controls::ComboBox::inputMethodHints

    Provides hints to the input method about the expected content of the combo box and how it
    should operate.

    The default value is \c Qt.ImhNoPredictiveText.

    \include inputmethodhints.qdocinc
*/
Qt::InputMethodHints QQuickComboBox::inputMethodHints() const
{
    Q_D(const QQuickComboBox);
    return d->extra.isAllocated() ? d->extra->inputMethodHints : Qt::ImhNoPredictiveText;
}

void QQuickComboBox::setInputMethodHints(Qt::InputMethodHints hints)
{
    Q_D(QQuickComboBox);
    d->setInputMethodHints(hints);
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty bool QtQuick.Controls::ComboBox::inputMethodComposing
    \readonly

    This property holds whether an editable combo box has partial text input from an input method.

    While it is composing, an input method may rely on mouse or key events from the combo box to
    edit or commit the partial text. This property can be used to determine when to disable event
    handlers that may interfere with the correct operation of an input method.
*/
bool QQuickComboBox::isInputMethodComposing() const
{
    Q_D(const QQuickComboBox);
    return d->contentItem && d->contentItem->property("inputMethodComposing").toBool();
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty bool QtQuick.Controls::ComboBox::acceptableInput
    \readonly

    This property holds whether the combo box contains acceptable text in the editable text field.

    If a validator has been set, the value is \c true only if the current text is acceptable
    to the validator as a final string (not as an intermediate string).

    \sa validator, accepted
*/
bool QQuickComboBox::hasAcceptableInput() const
{
    Q_D(const QQuickComboBox);
    return d->m_acceptableInput;
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::ComboBox::implicitIndicatorWidth
    \readonly

    This property holds the implicit indicator width.

    The value is equal to \c {indicator ? indicator.implicitWidth : 0}.

    This is typically used, together with \l {Control::}{implicitContentWidth} and
    \l {Control::}{implicitBackgroundWidth}, to calculate the \l {Item::}{implicitWidth}.

    \sa implicitIndicatorHeight
*/
qreal QQuickComboBox::implicitIndicatorWidth() const
{
    Q_D(const QQuickComboBox);
    if (!d->indicator)
        return 0;
    return d->indicator->implicitWidth();
}

/*!
    \since QtQuick.Controls 2.5 (Qt 5.12)
    \qmlproperty real QtQuick.Controls::ComboBox::implicitIndicatorHeight
    \readonly

    This property holds the implicit indicator height.

    The value is equal to \c {indicator ? indicator.implicitHeight : 0}.

    This is typically used, together with \l {Control::}{implicitContentHeight} and
    \l {Control::}{implicitBackgroundHeight}, to calculate the \l {Item::}{implicitHeight}.

    \sa implicitIndicatorWidth
*/
qreal QQuickComboBox::implicitIndicatorHeight() const
{
    Q_D(const QQuickComboBox);
    if (!d->indicator)
        return 0;
    return d->indicator->implicitHeight();
}

/*!
    \readonly
    \since QtQuick.Controls 2.14 (Qt 5.14)
    \qmlproperty var QtQuick.Controls::ComboBox::currentValue

    This property holds the value of the current item in the combo box.

    For an example of how to use this property, see \l {ComboBox Model Roles}.

    \sa currentIndex, currentText, valueRole
*/
QVariant QQuickComboBox::currentValue() const
{
    Q_D(const QQuickComboBox);
    return d->currentValue;
}

/*!
    \readonly
    \since QtQuick.Controls 2.14 (Qt 5.14)
    \qmlmethod var QtQuick.Controls::ComboBox::valueAt(int index)

    Returns the value at position \a index in the combo box.

    \sa indexOfValue
*/
QVariant QQuickComboBox::valueAt(int index) const
{
    Q_D(const QQuickComboBox);
    if (!d->isValidIndex(index))
        return QVariant();

    const QString effectiveValueRole = d->valueRole.isEmpty() ? QStringLiteral("modelData") : d->valueRole;
    return d->delegateModel->variantValue(index, effectiveValueRole);
}

/*!
    \since QtQuick.Controls 2.14 (Qt 5.14)
    \qmlmethod int QtQuick.Controls::ComboBox::indexOfValue(object value)

    Returns the index of the specified \a value, or \c -1 if no match is found.

    For an example of how to use this method, see \l {ComboBox Model Roles}.

    \include qquickcombobox.qdocinc functions-after-component-completion

    \sa find(), currentValue, currentIndex, valueRole, valueAt
*/
int QQuickComboBox::indexOfValue(const QVariant &value) const
{
    for (int i = 0; i < count(); ++i) {
        const QVariant ourValue = valueAt(i);
        if (value == ourValue)
            return i;
    }
    return -1;
}

/*!
    \since QtQuick.Controls 2.15 (Qt 5.15)
    \qmlproperty bool QtQuick.Controls::ComboBox::selectTextByMouse

    This property holds whether the text field for an editable ComboBox
    can be selected with the mouse.

    The default value is \c false.
*/
bool QQuickComboBox::selectTextByMouse() const
{
    Q_D(const QQuickComboBox);
    return d->extra.isAllocated() ? d->extra->selectTextByMouse : false;
}

void QQuickComboBox::setSelectTextByMouse(bool canSelect)
{
    Q_D(QQuickComboBox);
    if (canSelect == selectTextByMouse())
        return;

    d->extra.value().selectTextByMouse = canSelect;
    emit selectTextByMouseChanged();
}

/*!
    \since QtQuick.Controls 6.0 (Qt 6.0)
    \qmlproperty enumeration QtQuick.Controls::ComboBox::implicitContentWidthPolicy

    This property controls how the \l{Control::}{implicitContentWidth} of the ComboBox is
    calculated.

    When the width of a ComboBox is not large enough to display text, that text
    is elided. Depending on which parts of the text are elided, this can make
    selecting an item difficult for the end user. An efficient way of ensuring
    that a ComboBox is wide enough to avoid text being elided is to set a width
    that is known to be large enough:

    \code
    width: 300
    implicitContentWidthPolicy: ComboBox.ContentItemImplicitWidth
    \endcode

    However, it is often not possible to know whether or not a hard-coded value
    will be large enough, as the size of text depends on many factors, such as
    font family, font size, translations, and so on.

    implicitContentWidthPolicy provides an easy way to control how the
    implicitContentWidth is calculated, which in turn affects the
    \l{Item::}{implicitWidth} of the ComboBox and ensures that text will not be elided.

    The available values are:

    \value ContentItemImplicitWidth
        The implicitContentWidth will default to that of the \l{Control::}{contentItem}.
        This is the most efficient option, as no extra text layout is done.
    \value WidestText
        The implicitContentWidth will be set to the implicit width of the
        the largest text for the given \l textRole every time the model
        changes.
        This option should be used with smaller models, as it can be expensive.
    \value WidestTextWhenCompleted
        The implicitContentWidth will be set to the implicit width of the
        the largest text for the given \l textRole once after
        \l {QQmlParserStatus::componentComplete()}{component completion}.
        This option should be used with smaller models, as it can be expensive.

    The default value is \c ContentItemImplicitWidth.

    As this property only affects the \c implicitWidth of the ComboBox, setting
    an explicit \l{Item::}{width} can still result in eliding.

    \note This feature requires the contentItem to be a type derived from
        \l TextInput.

    \note This feature requires text to be laid out, and can therefore be
        expensive for large models or models whose contents are updated
        frequently.
*/
QQuickComboBox::ImplicitContentWidthPolicy QQuickComboBox::implicitContentWidthPolicy() const
{
    Q_D(const QQuickComboBox);
    return d->implicitContentWidthPolicy;
}

void QQuickComboBox::setImplicitContentWidthPolicy(QQuickComboBox::ImplicitContentWidthPolicy policy)
{
    Q_D(QQuickComboBox);
    if (policy == d->implicitContentWidthPolicy)
        return;

    d->implicitContentWidthPolicy = policy;
    d->maybeUpdateImplicitContentWidth();
    emit implicitContentWidthPolicyChanged();
}
/*!
    \qmlmethod string QtQuick.Controls::ComboBox::textAt(int index)

    Returns the text for the specified \a index, or an empty string
    if the index is out of bounds.

    \include qquickcombobox.qdocinc functions-after-component-completion
    For example:
    \snippet qtquickcontrols-combobox-textat.qml textat

    \sa textRole
*/
QString QQuickComboBox::textAt(int index) const
{
    Q_D(const QQuickComboBox);
    if (!d->isValidIndex(index))
        return QString();

    return d->delegateModel->stringValue(index, d->effectiveTextRole());
}

/*!
    \qmlmethod int QtQuick.Controls::ComboBox::find(string text, enumeration flags)

    Returns the index of the specified \a text, or \c -1 if no match is found.

    The way the search is performed is defined by the specified match \a flags. By default,
    combo box performs case sensitive exact matching (\c Qt.MatchExactly). All other match
    types are case-insensitive unless the \c Qt.MatchCaseSensitive flag is also specified.

    \value Qt.MatchExactly           The search term matches exactly (default).
    \value Qt.MatchRegularExpression The search term matches as a regular expression.
    \value Qt.MatchWildcard          The search term matches using wildcards.
    \value Qt.MatchFixedString       The search term matches as a fixed string.
    \value Qt.MatchStartsWith        The search term matches the start of the item.
    \value Qt.MatchEndsWidth         The search term matches the end of the item.
    \value Qt.MatchContains          The search term is contained in the item.
    \value Qt.MatchCaseSensitive     The search is case sensitive.

    \include qquickcombobox.qdocinc functions-after-component-completion
    For example:
    \snippet qtquickcontrols-combobox-find.qml find

    \sa textRole
*/
int QQuickComboBox::find(const QString &text, Qt::MatchFlags flags) const
{
    Q_D(const QQuickComboBox);
    return d->match(0, text, flags);
}

/*!
    \qmlmethod void QtQuick.Controls::ComboBox::incrementCurrentIndex()

    Increments the current index of the combo box, or the highlighted
    index if the popup list is visible.

    \sa currentIndex, highlightedIndex
*/
void QQuickComboBox::incrementCurrentIndex()
{
    Q_D(QQuickComboBox);
    d->incrementCurrentIndex();
}

/*!
    \qmlmethod void QtQuick.Controls::ComboBox::decrementCurrentIndex()

    Decrements the current index of the combo box, or the highlighted
    index if the popup list is visible.

    \sa currentIndex, highlightedIndex
*/
void QQuickComboBox::decrementCurrentIndex()
{
    Q_D(QQuickComboBox);
    d->decrementCurrentIndex();
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlmethod void QtQuick.Controls::ComboBox::selectAll()

    Selects all the text in the editable text field of the combo box.

    \sa editText
*/
void QQuickComboBox::selectAll()
{
    Q_D(QQuickComboBox);
    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(d->contentItem);
    if (!input)
        return;
    input->selectAll();
}

bool QQuickComboBox::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QQuickComboBox);
    switch (event->type()) {
    case QEvent::MouseButtonRelease:
        if (d->isPopupVisible())
            d->hidePopup(false);
        break;
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (d->filterKeyEvent(ke, false))
            return true;
        event->accept();
        if (d->extra.isAllocated())
            d->extra->allowComplete = ke->key() != Qt::Key_Backspace && ke->key() != Qt::Key_Delete;
        break;
    }
    case QEvent::FocusOut:
        if (qGuiApp->focusObject() != this && (!d->popup || !d->popup->hasActiveFocus())) {
            // Only close the popup if focus was transferred somewhere else
            // than to the popup or the popup button (which normally means that
            // the user clicked on the popup button to open it, not close it).
            d->hidePopup(false);
            setPressed(false);

            // The focus left the text field, so if the edit text matches an item in the model,
            // change our currentIndex to that. This matches widgets' behavior.
            const int indexForEditText = find(d->extra.value().editText, Qt::MatchFixedString);
            if (indexForEditText > -1)
                setCurrentIndex(indexForEditText);
        }
        break;
#if QT_CONFIG(im)
    case QEvent::InputMethod:
        if (d->extra.isAllocated())
            d->extra->allowComplete = !static_cast<QInputMethodEvent*>(event)->commitString().isEmpty();
        break;
#endif
    default:
        break;
    }
    return QQuickControl::eventFilter(object, event);
}

void QQuickComboBox::focusInEvent(QFocusEvent *event)
{
    Q_D(QQuickComboBox);
    QQuickControl::focusInEvent(event);
    // Setting focus on TextField should not be done when drop down indicator was clicked
    // That is why, if focus is not set with key reason, it should not be passed to textEdit by default.
    // Focus on Edit Text should be set only intentionally by user.
    if ((event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason ||
            event->reason() == Qt::ShortcutFocusReason) && d->contentItem && isEditable())
        d->contentItem->forceActiveFocus(event->reason());
}

void QQuickComboBox::focusOutEvent(QFocusEvent *event)
{
    Q_D(QQuickComboBox);
    QQuickControl::focusOutEvent(event);

    if (qGuiApp->focusObject() != d->contentItem && (!d->popup || !d->popup->hasActiveFocus())) {
        // Only close the popup if focus was transferred
        // somewhere else than to the popup or the inner line edit (which is
        // normally done from QQuickComboBox::focusInEvent).
        d->hidePopup(false);
        setPressed(false);
    }
}

#if QT_CONFIG(im)
void QQuickComboBox::inputMethodEvent(QInputMethodEvent *event)
{
    Q_D(QQuickComboBox);
    QQuickControl::inputMethodEvent(event);
    if (!isEditable() && !event->commitString().isEmpty())
        d->keySearch(event->commitString());
    else
        event->ignore();
}
#endif

void QQuickComboBox::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickComboBox);
    QQuickControl::keyPressEvent(event);

    const auto key = event->key();
    if (!isEditable()) {
        const auto buttonPressKeys = QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::ButtonPressKeys).value<QList<Qt::Key>>();
        if (buttonPressKeys.contains(key)) {
            if (!event->isAutoRepeat())
                setPressed(true);
            event->accept();
            return;
        }
    }

    switch (key) {
    case Qt::Key_Escape:
    case Qt::Key_Back:
        if (d->isPopupVisible())
            event->accept();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (d->isPopupVisible())
            setPressed(true);
        event->accept();
        break;
    case Qt::Key_Up:
        d->keyNavigating = true;
        d->decrementCurrentIndex();
        event->accept();
        break;
    case Qt::Key_Down:
        d->keyNavigating = true;
        d->incrementCurrentIndex();
        event->accept();
        break;
    case Qt::Key_Home:
        d->keyNavigating = true;
        if (d->isPopupVisible())
            d->setHighlightedIndex(0, Highlight);
        else
            d->setCurrentIndex(0, Activate);
        event->accept();
        break;
    case Qt::Key_End:
        d->keyNavigating = true;
        if (d->isPopupVisible())
            d->setHighlightedIndex(count() - 1, Highlight);
        else
            d->setCurrentIndex(count() - 1, Activate);
        event->accept();
        break;
    default:
        if (!isEditable() && !event->text().isEmpty())
            d->keySearch(event->text());
        else
            event->ignore();
        break;
    }
}

void QQuickComboBox::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickComboBox);
    QQuickControl::keyReleaseEvent(event);
    d->keyNavigating = false;
    if (event->isAutoRepeat())
        return;

    const auto key = event->key();
    if (!isEditable()) {
        const auto buttonPressKeys = QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::ButtonPressKeys).value<QList<Qt::Key>>();
        if (buttonPressKeys.contains(key)) {
            if (!isEditable() && isPressed())
                d->togglePopup(true);
            setPressed(false);
            event->accept();
            return;
        }
    }

    switch (key) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (!isEditable() || d->isPopupVisible())
            d->hidePopup(d->isPopupVisible());
        setPressed(false);
        event->accept();
        break;
    case Qt::Key_Escape:
    case Qt::Key_Back:
        if (d->isPopupVisible()) {
            d->hidePopup(false);
            setPressed(false);
            event->accept();
        }
        break;
    default:
        break;
    }
}

#if QT_CONFIG(wheelevent)
void QQuickComboBox::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickComboBox);
    QQuickControl::wheelEvent(event);
    if (d->wheelEnabled && !d->isPopupVisible()) {
        if (event->angleDelta().y() > 0)
            d->decrementCurrentIndex();
        else
            d->incrementCurrentIndex();
    }
}
#endif

bool QQuickComboBox::event(QEvent *e)
{
    Q_D(QQuickComboBox);
    if (e->type() == QEvent::LanguageChange)
        d->updateCurrentTextAndValue();
    return QQuickControl::event(e);
}

void QQuickComboBox::componentComplete()
{
    Q_D(QQuickComboBox);
    d->executeIndicator(true);
    QQuickControl::componentComplete();
    if (d->popup)
        d->executePopup(true);

    if (d->delegateModel && d->ownModel)
        static_cast<QQmlDelegateModel *>(d->delegateModel)->componentComplete();

    if (count() > 0) {
        if (!d->hasCurrentIndex && d->currentIndex == -1)
            setCurrentIndex(0);
        else
            d->updateCurrentTextAndValue();

        // If the widest text was already calculated in the call to
        // QQmlDelegateModel::componentComplete() above, then we shouldn't do it here too.
        if (!d->hasCalculatedWidestText)
            d->maybeUpdateImplicitContentWidth();
    }
}

void QQuickComboBox::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    Q_D(QQuickComboBox);
    QQuickControl::itemChange(change, value);
    if (change == ItemVisibleHasChanged && !value.boolValue) {
        d->hidePopup(false);
        setPressed(false);
    }
}

void QQuickComboBox::fontChange(const QFont &newFont, const QFont &oldFont)
{
    Q_D(QQuickComboBox);
    QQuickControl::fontChange(newFont, oldFont);
    d->maybeUpdateImplicitContentWidth();
}

void QQuickComboBox::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickComboBox);
    if (oldItem) {
        oldItem->removeEventFilter(this);
        if (QQuickTextInput *oldInput = qobject_cast<QQuickTextInput *>(oldItem)) {
            QObjectPrivate::disconnect(oldInput, &QQuickTextInput::accepted, d, &QQuickComboBoxPrivate::acceptInput);
            QObjectPrivate::disconnect(oldInput, &QQuickTextInput::textChanged, d, &QQuickComboBoxPrivate::updateEditText);
            disconnect(oldInput, &QQuickTextInput::inputMethodComposingChanged, this, &QQuickComboBox::inputMethodComposingChanged);
            QObjectPrivate::disconnect(oldInput, &QQuickTextInput::acceptableInputChanged, d, &QQuickComboBoxPrivate::updateAcceptableInput);
        }
    }
    if (newItem && isEditable()) {
        newItem->installEventFilter(this);
        if (QQuickTextInput *newInput = qobject_cast<QQuickTextInput *>(newItem)) {
            QObjectPrivate::connect(newInput, &QQuickTextInput::accepted, d, &QQuickComboBoxPrivate::acceptInput);
            QObjectPrivate::connect(newInput, &QQuickTextInput::textChanged, d, &QQuickComboBoxPrivate::updateEditText);
            connect(newInput, &QQuickTextInput::inputMethodComposingChanged, this, &QQuickComboBox::inputMethodComposingChanged);
            QObjectPrivate::connect(newInput, &QQuickTextInput::acceptableInputChanged, d, &QQuickComboBoxPrivate::updateAcceptableInput);
        }
#if QT_CONFIG(cursor)
        newItem->setCursor(Qt::IBeamCursor);
#endif
    }

    d->updateAcceptableInput();
}

void QQuickComboBox::localeChange(const QLocale &newLocale, const QLocale &oldLocale)
{
    QQuickControl::localeChange(newLocale, oldLocale);
#if QT_CONFIG(validator)
    if (QValidator *v = validator())
        v->setLocale(newLocale);
#endif
}

QFont QQuickComboBox::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::ComboBox);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickComboBox::accessibleRole() const
{
    return QAccessible::ComboBox;
}

void QQuickComboBox::accessibilityActiveChanged(bool active)
{
    Q_D(QQuickComboBox);
    QQuickControl::accessibilityActiveChanged(active);

    if (active) {
        maybeSetAccessibleName(d->hasDisplayText ? d->displayText : d->currentText);
        setAccessibleProperty("editable", isEditable());
    }
}
#endif //

QT_END_NAMESPACE

#include "moc_qquickcombobox_p.cpp"
