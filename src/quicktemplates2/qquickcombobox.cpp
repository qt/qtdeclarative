/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickcombobox_p.h"
#include "qquickcontrol_p_p.h"
#include "qquickabstractbutton_p.h"
#include "qquickpopup_p_p.h"

#include <QtCore/qregexp.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/private/qqmldelegatemodel_p.h>
#include <QtQuick/private/qquickevents_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ComboBox
    \inherits Control
    \instantiates QQuickComboBox
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-input
    \brief A combined button and popup list taking minimal space.

    \image qtquickcontrols2-combobox.png

    ComboBox is a combined button and popup list. It provides a means of
    presenting a list of options to the user in a way that takes up the
    minimum amount of screen space.

    ComboBox is populated with a data model. The data model is commonly
    a JavaScript array, a \l ListModel or an integer, but also other types
    of \l {qml-data-models}{data models} are supported.

    \code
    ComboBox {
        model: ["First", "Second", "Third"]
    }
    \endcode

    ComboBox is able to visualize standard \l {qml-data-models}{data models}
    that provide the \c modelData role:
    \list
    \li models that have only one role
    \li models that do not have named roles (JavaScript array, integer)
    \endlist

    When using models that have multiple named roles, ComboBox must be configured
    to use a specific \l {textRole}{text role} for its \l {displayText}{display text}
    and \l delegate instances.

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

    \note If ComboBox is assigned a data model that has multiple named roles, but
    \l textRole is not defined, ComboBox is unable to visualize it and throws a
    \c {ReferenceError: modelData is not defined}.

    \sa {Customizing ComboBox}, {Input Controls}
*/

/*!
    \qmlsignal void QtQuick.Controls::ComboBox::activated(int index)

    This signal is emitted when the item at \a index is activated by the user.

    \sa currentIndex
*/

/*!
    \qmlsignal void QtQuick.Controls::ComboBox::highlighted(int index)

    This signal is emitted when the item at \a index in the popup list is highlighted by the user.

    \sa highlightedIndex
*/

class QQuickComboBoxDelegateModel : public QQmlDelegateModel
{
public:
    explicit QQuickComboBoxDelegateModel(QQuickComboBox *combo);
    QString stringValue(int index, const QString &role) override;

private:
    QQuickComboBox *combo;
};

QQuickComboBoxDelegateModel::QQuickComboBoxDelegateModel(QQuickComboBox *combo) :
    QQmlDelegateModel(qmlContext(combo), combo), combo(combo)
{
}

QString QQuickComboBoxDelegateModel::stringValue(int index, const QString &role)
{
    QVariant model = combo->model();
    if (model.userType() == QMetaType::QVariantList) {
        QVariant object = model.toList().value(index);
        if (object.userType() == QMetaType::QVariantMap) {
            const QVariantMap data = object.toMap();
            if (data.count() == 1 && role == QLatin1String("modelData"))
                return data.first().toString();
            return data.value(role).toString();
        }
    }
    return QQmlDelegateModel::stringValue(index, role);
}

class QQuickComboBoxPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickComboBox)

public:
    QQuickComboBoxPrivate() : pressed(false), ownModel(false), hasDisplayText(false), hasCurrentIndex(false),
        highlightedIndex(-1), currentIndex(-1), delegateModel(nullptr),
        delegate(nullptr), indicator(nullptr), popup(nullptr) { }

    bool isPopupVisible() const;
    void showPopup();
    void hidePopup(bool accept);
    void togglePopup(bool accept);

    void itemClicked();

    void createdItem(int index, QObject *object);
    void countChanged();
    void updateCurrentText();
    void incrementCurrentIndex();
    void decrementCurrentIndex();
    void setHighlightedIndex(int index);

    void createDelegateModel();

    bool pressed;
    bool ownModel;
    bool hasDisplayText;
    bool hasCurrentIndex;
    int highlightedIndex;
    int currentIndex;
    QVariant model;
    QString textRole;
    QString currentText;
    QString displayText;
    QQuickItem *pressedItem;
    QQmlInstanceModel *delegateModel;
    QQmlComponent *delegate;
    QQuickItem *indicator;
    QQuickPopup *popup;
};

bool QQuickComboBoxPrivate::isPopupVisible() const
{
    return popup && popup->isVisible();
}

void QQuickComboBoxPrivate::showPopup()
{
    if (popup && !popup->isVisible())
        popup->open();
    setHighlightedIndex(currentIndex);
}

void QQuickComboBoxPrivate::hidePopup(bool accept)
{
    Q_Q(QQuickComboBox);
    if (popup && popup->isVisible())
        popup->close();
    if (accept) {
        q->setCurrentIndex(highlightedIndex);
        emit q->activated(currentIndex);
    }
    setHighlightedIndex(-1);
}

void QQuickComboBoxPrivate::togglePopup(bool accept)
{
    if (!popup)
        return;

    if (popup->isVisible())
        hidePopup(accept);
    else
        showPopup();
}

void QQuickComboBoxPrivate::itemClicked()
{
    Q_Q(QQuickComboBox);
    int index = delegateModel->indexOf(q->sender(), nullptr);
    if (index != -1) {
        setHighlightedIndex(index);
        emit q->highlighted(index);
        hidePopup(true);
    }
}

void QQuickComboBoxPrivate::createdItem(int index, QObject *object)
{
    QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(object);
    if (button) {
        button->setFocusPolicy(Qt::NoFocus);
        connect(button, &QQuickAbstractButton::clicked, this, &QQuickComboBoxPrivate::itemClicked);
    }

    if (index == currentIndex)
        updateCurrentText();
}

void QQuickComboBoxPrivate::countChanged()
{
    Q_Q(QQuickComboBox);
    if (q->count() == 0)
        q->setCurrentIndex(-1);
    emit q->countChanged();
}

void QQuickComboBoxPrivate::updateCurrentText()
{
    Q_Q(QQuickComboBox);
    QString text = q->textAt(currentIndex);
    if (currentText != text) {
        currentText = text;
        emit q->currentTextChanged();
    }
    if (!hasDisplayText && displayText != text) {
        displayText = text;
        emit q->displayTextChanged();
    }
}

void QQuickComboBoxPrivate::incrementCurrentIndex()
{
    Q_Q(QQuickComboBox);
    if (isPopupVisible()) {
        if (highlightedIndex < q->count() - 1) {
            setHighlightedIndex(highlightedIndex + 1);
            emit q->highlighted(highlightedIndex);
        }
    } else {
        if (currentIndex < q->count() - 1) {
            q->setCurrentIndex(currentIndex + 1);
            emit q->activated(currentIndex);
        }
    }
}

void QQuickComboBoxPrivate::decrementCurrentIndex()
{
    Q_Q(QQuickComboBox);
    if (isPopupVisible()) {
        if (highlightedIndex > 0) {
            setHighlightedIndex(highlightedIndex - 1);
            emit q->highlighted(highlightedIndex);
        }
    } else {
        if (currentIndex > 0) {
            q->setCurrentIndex(currentIndex - 1);
            emit q->activated(currentIndex);
        }
    }
}

void QQuickComboBoxPrivate::setHighlightedIndex(int index)
{
    Q_Q(QQuickComboBox);
    if (highlightedIndex == index)
        return;

    highlightedIndex = index;
    emit q->highlightedIndexChanged();
}

void QQuickComboBoxPrivate::createDelegateModel()
{
    Q_Q(QQuickComboBox);
    bool ownedOldModel = ownModel;
    QQmlInstanceModel* oldModel = delegateModel;
    if (oldModel) {
        disconnect(delegateModel, &QQmlInstanceModel::countChanged, this, &QQuickComboBoxPrivate::countChanged);
        disconnect(delegateModel, &QQmlInstanceModel::modelUpdated, this, &QQuickComboBoxPrivate::updateCurrentText);
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
        connect(delegateModel, &QQmlInstanceModel::modelUpdated, this, &QQuickComboBoxPrivate::updateCurrentText);
        connect(delegateModel, &QQmlInstanceModel::createdItem, this, &QQuickComboBoxPrivate::createdItem);
    }

    emit q->delegateModelChanged();

    if (ownedOldModel)
        delete oldModel;
}

QQuickComboBox::QQuickComboBox(QQuickItem *parent) :
    QQuickControl(*(new QQuickComboBoxPrivate), parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setFlag(QQuickItem::ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::LeftButton);
}

QQuickComboBox::~QQuickComboBox()
{
    Q_D(QQuickComboBox);
    delete d->popup;
    d->popup = nullptr;
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

    d->model = model;
    d->createDelegateModel();
    if (isComponentComplete()) {
        setCurrentIndex(count() > 0 ? 0 : -1);
        d->updateCurrentText();
    }
    emit modelChanged();
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
    \qmlproperty bool QtQuick.Controls::ComboBox::pressed

    This property holds whether the combo box button is pressed.
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
}

/*!
    \readonly
    \qmlproperty int QtQuick.Controls::ComboBox::highlightedIndex

    This property holds the index of the highlighted item in the combo box popup list.

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

    \sa activated(), currentText
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
    if (d->currentIndex == index)
        return;

    d->currentIndex = index;
    emit currentIndexChanged();
    if (isComponentComplete())
        d->updateCurrentText();
}

/*!
    \readonly
    \qmlproperty string QtQuick.Controls::ComboBox::currentText

    This property holds the text of the current item in the combo box.

    \sa currentIndex, displayText, textRole
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

    \sa model, currentText, displayText
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
    \qmlproperty Component QtQuick.Controls::ComboBox::delegate

    This property holds a delegate that presents an item in the combo box popup.

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
*/
QQuickItem *QQuickComboBox::indicator() const
{
    Q_D(const QQuickComboBox);
    return d->indicator;
}

void QQuickComboBox::setIndicator(QQuickItem *indicator)
{
    Q_D(QQuickComboBox);
    if (d->indicator == indicator)
        return;

    delete d->indicator;
    d->indicator = indicator;
    if (indicator) {
        if (!indicator->parentItem())
            indicator->setParentItem(this);
    }
    emit indicatorChanged();
}

/*!
    \qmlproperty Popup QtQuick.Controls::ComboBox::popup

    This property holds the popup.

    \sa {Customizing ComboBox}
*/
QQuickPopup *QQuickComboBox::popup() const
{
    Q_D(const QQuickComboBox);
    return d->popup;
}

void QQuickComboBox::setPopup(QQuickPopup *popup)
{
    Q_D(QQuickComboBox);
    if (d->popup == popup)
        return;

    delete d->popup;
    if (popup) {
        QQuickPopupPrivate::get(popup)->allowVerticalFlip = true;
        popup->setClosePolicy(QQuickPopup::CloseOnEscape | QQuickPopup::CloseOnPressOutsideParent);
    }
    d->popup = popup;
    emit popupChanged();
}

/*!
    \qmlmethod string QtQuick.Controls::ComboBox::textAt(int index)

    Returns the text for the specified \a index, or an empty string
    if the index is out of bounds.

    \sa textRole
*/
QString QQuickComboBox::textAt(int index) const
{
    Q_D(const QQuickComboBox);
    if (!d->delegateModel || index < 0 || index >= d->delegateModel->count() || !d->delegateModel->object(index))
        return QString();
    return d->delegateModel->stringValue(index, d->textRole.isEmpty() ? QStringLiteral("modelData") : d->textRole);
}

/*!
    \qmlmethod int QtQuick.Controls::ComboBox::find(string text, flags = Qt.MatchExactly)

    Returns the index of the specified \a text, or \c -1 if no match is found.

    The way the search is performed is defined by the specified match \a flags. By default,
    combo box performs case sensitive exact matching (\c Qt.MatchExactly). All other match
    types are case-insensitive unless the \c Qt.MatchCaseSensitive flag is also specified.

    \value Qt.MatchExactly          The search term matches exactly (default).
    \value Qt.MatchRegExp           The search term matches as a regular expression.
    \value Qt.MatchWildcard         The search term matches using wildcards.
    \value Qt.MatchFixedString      The search term matches as a fixed string.
    \value Qt.MatchStartsWith       The search term matches the start of the item.
    \value Qt.MatchEndsWidth        The search term matches the end of the item.
    \value Qt.MatchContains         The search term is contained in the item.
    \value Qt.MatchCaseSensitive    The search is case sensitive.

    \sa textRole
*/
int QQuickComboBox::find(const QString &text, Qt::MatchFlags flags) const
{
    int itemCount = count();
    uint matchType = flags & 0x0F;
    Qt::CaseSensitivity cs = flags & Qt::MatchCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    for (int idx = 0; idx < itemCount; ++idx) {
        QString t = textAt(idx);
        switch (matchType) {
        case Qt::MatchExactly:
            if (t == text)
                return idx;
            break;
        case Qt::MatchRegExp:
            if (QRegExp(text, cs).exactMatch(t))
                return idx;
            break;
        case Qt::MatchWildcard:
            if (QRegExp(text, cs, QRegExp::Wildcard).exactMatch(t))
                return idx;
            break;
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
    return -1;
}

/*!
    \qmlmethod void QtQuick.Controls::ComboBox::incrementCurrentIndex()

    Increments the current index of the combo box, or the highlighted
    index if the popup list when it is visible.

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
    index if the popup list when it is visible.

    \sa currentIndex, highlightedIndex
*/
void QQuickComboBox::decrementCurrentIndex()
{
    Q_D(QQuickComboBox);
    d->decrementCurrentIndex();
}

void QQuickComboBox::focusOutEvent(QFocusEvent *event)
{
    Q_D(QQuickComboBox);
    QQuickControl::focusOutEvent(event);
    d->hidePopup(false);
    setPressed(false);
}

void QQuickComboBox::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickComboBox);
    QQuickControl::keyPressEvent(event);
    if (!d->popup)
        return;

    switch (event->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Back:
        if (d->isPopupVisible())
            event->accept();
        break;
    case Qt::Key_Space:
        if (!event->isAutoRepeat())
            setPressed(true);
        event->accept();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (d->isPopupVisible())
            setPressed(true);
        event->accept();
        break;
    case Qt::Key_Up:
        d->decrementCurrentIndex();
        event->accept();
        break;
    case Qt::Key_Down:
        d->incrementCurrentIndex();
        event->accept();
        break;
    default:
        break;
    }
}

void QQuickComboBox::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickComboBox);
    QQuickControl::keyReleaseEvent(event);
    if (!d->popup || event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_Space:
        d->togglePopup(true);
        setPressed(false);
        event->accept();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        d->hidePopup(d->isPopupVisible());
        setPressed(false);
        event->accept();
        break;
    case Qt::Key_Escape:
    case Qt::Key_Back:
        d->hidePopup(false);
        setPressed(false);
        event->accept();
        break;
    default:
        break;
    }
}

void QQuickComboBox::mousePressEvent(QMouseEvent *event)
{
    QQuickControl::mousePressEvent(event);
    setPressed(true);
}

void QQuickComboBox::mouseMoveEvent(QMouseEvent* event)
{
    QQuickControl::mouseMoveEvent(event);
    setPressed(contains(event->pos()));
}

void QQuickComboBox::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickComboBox);
    QQuickControl::mouseReleaseEvent(event);
    if (d->pressed) {
        setPressed(false);
        d->togglePopup(false);
    }
}

void QQuickComboBox::mouseUngrabEvent()
{
    QQuickControl::mouseUngrabEvent();
    setPressed(false);
}

void QQuickComboBox::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickComboBox);
    QQuickControl::wheelEvent(event);
    if (d->wheelEnabled && !d->isPopupVisible()) {
        const int oldIndex = d->currentIndex;
        if (event->angleDelta().y() > 0)
            d->decrementCurrentIndex();
        else
            d->incrementCurrentIndex();
        event->setAccepted(d->currentIndex != oldIndex);
    }
}

void QQuickComboBox::componentComplete()
{
    Q_D(QQuickComboBox);
    QQuickControl::componentComplete();

    if (d->delegateModel && d->ownModel)
        static_cast<QQmlDelegateModel *>(d->delegateModel)->componentComplete();

    if (count() > 0) {
        if (!d->hasCurrentIndex && d->currentIndex == -1)
            setCurrentIndex(0);
        else
            d->updateCurrentText();
    }
}

QFont QQuickComboBox::defaultFont() const
{
    return QQuickControlPrivate::themeFont(QPlatformTheme::ComboMenuItemFont);
}

QT_END_NAMESPACE
