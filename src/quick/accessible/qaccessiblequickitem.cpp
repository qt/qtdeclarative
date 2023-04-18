// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblequickitem_p.h"

#include <QtGui/qtextdocument.h>

#include "QtQuick/private/qquickitem_p.h"
#include "QtQuick/private/qquicktext_p.h"
#include <private/qquicktext_p_p.h>

#include "QtQuick/private/qquicktextinput_p.h"
#include "QtQuick/private/qquickaccessibleattached_p.h"
#include "QtQuick/qquicktextdocument.h"
#include "QtQuick/qquickrendercontrol.h"
QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

class QAccessibleHyperlink : public QAccessibleInterface, public QAccessibleHyperlinkInterface {
public:
    QAccessibleHyperlink(QQuickItem *parentTextItem, int linkIndex);

    // check for valid pointers
    bool isValid() const override;
    QObject *object() const override;
    QWindow *window() const override;

    // navigation, hierarchy
    QAccessibleInterface *parent() const override;
    QAccessibleInterface *child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *iface) const override;
    QAccessibleInterface *childAt(int x, int y) const override;

    // properties and state
    QString text(QAccessible::Text) const override;
    void setText(QAccessible::Text, const QString &text) override;
    QRect rect() const override;
    QAccessible::Role role() const override;
    QAccessible::State state() const override;

    void *interface_cast(QAccessible::InterfaceType t) override;

    // QAccessibleHyperlinkInterface
    QString anchor() const override
    {
        const QVector<QQuickTextPrivate::LinkDesc> links = QQuickTextPrivate::get(textItem())->getLinks();
        if (linkIndex < links.size())
            return links.at(linkIndex).m_anchor;
        return QString();
    }

    QString anchorTarget() const override
    {
        const QVector<QQuickTextPrivate::LinkDesc> links = QQuickTextPrivate::get(textItem())->getLinks();
        if (linkIndex < links.size())
            return links.at(linkIndex).m_anchorTarget;
        return QString();
    }

    int startIndex() const override
    {
        const QVector<QQuickTextPrivate::LinkDesc> links = QQuickTextPrivate::get(textItem())->getLinks();
        if (linkIndex < links.size())
            return links.at(linkIndex).m_startIndex;
        return -1;
    }

    int endIndex() const override
    {
        const QVector<QQuickTextPrivate::LinkDesc> links = QQuickTextPrivate::get(textItem())->getLinks();
        if (linkIndex < links.size())
            return links.at(linkIndex).m_endIndex;
        return -1;
    }

private:
    QQuickText *textItem() const { return qobject_cast<QQuickText*>(parentTextItem); }
    QQuickItem *parentTextItem;
    const int linkIndex;

    friend class QAccessibleQuickItem;
};


QAccessibleHyperlink::QAccessibleHyperlink(QQuickItem *parentTextItem, int linkIndex)
    : parentTextItem(parentTextItem),
      linkIndex(linkIndex)
{
}


bool QAccessibleHyperlink::isValid() const
{
    return textItem();
}


QObject *QAccessibleHyperlink::object() const
{
    return nullptr;
}


QWindow *QAccessibleHyperlink::window() const
{
    return textItem()->window();
}


/* \reimp */
QRect QAccessibleHyperlink::rect() const
{
    const QVector<QQuickTextPrivate::LinkDesc> links = QQuickTextPrivate::get(textItem())->getLinks();
    if (linkIndex < links.size()) {
        const QPoint tl = itemScreenRect(textItem()).topLeft();
        return links.at(linkIndex).rect.translated(tl);
    }
    return QRect();
}

/* \reimp */
QAccessibleInterface *QAccessibleHyperlink::childAt(int, int) const
{
    return nullptr;
}

/* \reimp */
QAccessibleInterface *QAccessibleHyperlink::parent() const
{
    return QAccessible::queryAccessibleInterface(textItem());
}

/* \reimp */
QAccessibleInterface *QAccessibleHyperlink::child(int) const
{
    return nullptr;
}

/* \reimp */
int QAccessibleHyperlink::childCount() const
{
    return 0;
}

/* \reimp */
int QAccessibleHyperlink::indexOfChild(const QAccessibleInterface *) const
{
    return -1;
}

/* \reimp */
QAccessible::State QAccessibleHyperlink::state() const
{
    QAccessible::State s;
    s.selectable = true;
    s.focusable = true;
    s.selectableText = true;
    s.selected = false;
    return s;
}

/* \reimp */
QAccessible::Role QAccessibleHyperlink::role() const
{
    return QAccessible::Link;
}

/* \reimp */
QString QAccessibleHyperlink::text(QAccessible::Text t) const
{
    // AT servers have different behaviors:
    // Wordpad on windows have this behavior:
    //   * Name returns the anchor target (URL)
    //   * Value returns the anchor target (URL)

    // Other AT servers (e.g. MS Edge on Windows) does what seems to be more sensible:
    //   * Name returns the anchor name
    //   * Value returns the anchor target (URL)
    if (t == QAccessible::Name)
        return anchor();
    if (t == QAccessible::Value)
        return anchorTarget();
    return QString();
}

/* \reimp */
void QAccessibleHyperlink::setText(QAccessible::Text, const QString &)
{

}

/* \reimp */
void *QAccessibleHyperlink::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::HyperlinkInterface)
       return static_cast<QAccessibleHyperlinkInterface*>(this);
    return nullptr;
}


/*!
 * \internal
 * \brief QAccessibleQuickItem::QAccessibleQuickItem
 * \param item
 */
QAccessibleQuickItem::QAccessibleQuickItem(QQuickItem *item)
    : QAccessibleObject(item), m_doc(textDocument())
{
}

QWindow *QAccessibleQuickItem::window() const
{
    QQuickWindow *window = item()->window();

    // For QQuickWidget the above window will be the offscreen QQuickWindow,
    // which is not a part of the accessibility tree. Detect this case and
    // return the window for the QQuickWidget instead.
    if (window && !window->handle()) {
        if (QQuickRenderControl *renderControl = QQuickWindowPrivate::get(window)->renderControl) {
            if (QWindow *renderWindow = renderControl->renderWindow(nullptr))
                return renderWindow;
        }
    }

    return window;
}

int QAccessibleQuickItem::childCount() const
{
    // see comment in QAccessibleQuickItem::child() as to why we do this
    int cc = 0;
    if (QQuickText *textItem = qobject_cast<QQuickText*>(item())) {
        cc = QQuickTextPrivate::get(textItem)->getLinks().size();
    }
    cc += childItems().size();
    return cc;
}

QRect QAccessibleQuickItem::rect() const
{
    const QRect r = itemScreenRect(item());
    return r;
}

QRect QAccessibleQuickItem::viewRect() const
{
    // ### no window in some cases.
    if (!item()->window()) {
        return QRect();
    }

    QQuickWindow *window = item()->window();
    QPoint screenPos = window->mapToGlobal(QPoint(0,0));
    return QRect(screenPos, window->size());
}


bool QAccessibleQuickItem::clipsChildren() const
{
    return static_cast<QQuickItem *>(item())->clip();
}

QAccessibleInterface *QAccessibleQuickItem::childAt(int x, int y) const
{
    if (item()->clip()) {
        if (!rect().contains(x, y))
            return nullptr;
    }

    // special case for text interfaces
    if (QQuickText *textItem = qobject_cast<QQuickText*>(item())) {
        const auto hyperLinkChildCount = QQuickTextPrivate::get(textItem)->getLinks().size();
        for (auto i = 0; i < hyperLinkChildCount; i++) {
            QAccessibleInterface *iface = child(i);
            if (iface->rect().contains(x,y)) {
                return iface;
            }
        }
    }

    // general item hit test
    const QList<QQuickItem*> kids = accessibleUnignoredChildren(item(), true);
    for (int i = kids.size() - 1; i >= 0; --i) {
        QAccessibleInterface *childIface = QAccessible::queryAccessibleInterface(kids.at(i));
        if (QAccessibleInterface *childChild = childIface->childAt(x, y))
            return childChild;
        if (childIface && !childIface->state().invisible) {
            if (childIface->rect().contains(x, y))
                return childIface;
        }
    }

    return nullptr;
}

QAccessibleInterface *QAccessibleQuickItem::parent() const
{
    QQuickItem *parent = item()->parentItem();
    QQuickWindow *itemWindow = item()->window();
    QQuickItem *ci = itemWindow ? itemWindow->contentItem() : nullptr;
    while (parent && !QQuickItemPrivate::get(parent)->isAccessible && parent != ci)
        parent = parent->parentItem();

    if (parent) {
        if (parent == ci) {
            // Jump out to the window if the parent is the root item
            return QAccessible::queryAccessibleInterface(window());
        } else {
            while (parent && !parent->d_func()->isAccessible)
                parent = parent->parentItem();
            return QAccessible::queryAccessibleInterface(parent);
        }
    }
    return nullptr;
}

QAccessibleInterface *QAccessibleQuickItem::child(int index) const
{
    /*  Text with hyperlinks will have dedicated children interfaces representing each hyperlink.

        For the pathological case when a Text node has hyperlinks in its text *and* accessible
        quick items as children, we put the hyperlink a11y interfaces as the first children, then
        the other interfaces follows the hyperlink children (as siblings).

        For example, suppose you have two links in the text and an image as a child of the text,
        it will have the following a11y hierarchy:

       [a11y:TextInterface]
        |
        +- [a11y:HyperlinkInterface]
        +- [a11y:HyperlinkInterface]
        +- [a11y:ImageInterface]

        Having this order (as opposed to having hyperlink interfaces last) will at least
        ensure that the child id of hyperlink children is not altered when child is added/removed
        to the text item and marked accessible.
        In addition, hyperlink interfaces as children should be the common case, so it is preferred
        to explore those first when iterating.
    */
    if (index < 0)
        return nullptr;


    if (QQuickText *textItem = qobject_cast<QQuickText*>(item())) {
        const int hyperLinkChildCount = QQuickTextPrivate::get(textItem)->getLinks().size();
        if (index < hyperLinkChildCount) {
            auto it = m_childToId.constFind(index);
            if (it != m_childToId.constEnd())
                return QAccessible::accessibleInterface(it.value());

            QAccessibleHyperlink *iface = new QAccessibleHyperlink(item(), index);
            QAccessible::Id id = QAccessible::registerAccessibleInterface(iface);
            m_childToId.insert(index, id);
            return iface;
        }
        index -= hyperLinkChildCount;
    }

    QList<QQuickItem *> children = childItems();
    if (index < children.size()) {
        QQuickItem *child = children.at(index);
        return QAccessible::queryAccessibleInterface(child);
    }
    return nullptr;
}

int QAccessibleQuickItem::indexOfChild(const QAccessibleInterface *iface) const
{
    int hyperLinkChildCount = 0;
    if (QQuickText *textItem = qobject_cast<QQuickText*>(item())) {
        hyperLinkChildCount = QQuickTextPrivate::get(textItem)->getLinks().size();
        if (QAccessibleHyperlinkInterface *hyperLinkIface = const_cast<QAccessibleInterface *>(iface)->hyperlinkInterface()) {
            // ### assumes that there is only one subclass implementing QAccessibleHyperlinkInterface
            // Alternatively, we could simply iterate with child() and do a linear search for it
            QAccessibleHyperlink *hyperLink = static_cast<QAccessibleHyperlink*>(hyperLinkIface);
            if (hyperLink->textItem() == static_cast<QQuickText*>(item())) {
                return hyperLink->linkIndex;
            }
        }
    }
    QList<QQuickItem*> kids = childItems();
    int idx = kids.indexOf(static_cast<QQuickItem*>(iface->object()));
    if (idx >= 0)
        idx += hyperLinkChildCount;
    return idx;
}

static void unignoredChildren(QQuickItem *item, QList<QQuickItem *> *items, bool paintOrder)
{
    const QList<QQuickItem*> childItems = paintOrder ? QQuickItemPrivate::get(item)->paintOrderChildItems()
                                               : item->childItems();
    for (QQuickItem *child : childItems) {
        if (QQuickItemPrivate::get(child)->isAccessible) {
            items->append(child);
        } else {
            unignoredChildren(child, items, paintOrder);
        }
    }
}

QList<QQuickItem *> accessibleUnignoredChildren(QQuickItem *item, bool paintOrder)
{
    QList<QQuickItem *> items;
    unignoredChildren(item, &items, paintOrder);
    return items;
}

QList<QQuickItem *> QAccessibleQuickItem::childItems() const
{
    return accessibleUnignoredChildren(item());
}

static bool isTextRole(QAccessible::Role role)
{
    return role == QAccessible::EditableText || role == QAccessible::StaticText;
}

QAccessible::State QAccessibleQuickItem::state() const
{
    QQuickAccessibleAttached *attached = QQuickAccessibleAttached::attachedProperties(item());
    if (!attached)
        return QAccessible::State();

    QAccessible::State state = attached->state();

    QRect viewRect_ = viewRect();
    QRect itemRect = rect();

    if (viewRect_.isNull() || itemRect.isNull() || !window() || !window()->isVisible() ||!item()->isVisible() || qFuzzyIsNull(item()->opacity()))
        state.invisible = true;
    if (!viewRect_.intersects(itemRect))
        state.offscreen = true;
    if ((role() == QAccessible::CheckBox || role() == QAccessible::RadioButton) && object()->property("checked").toBool())
        state.checked = true;
    if (item()->activeFocusOnTab() || isTextRole(role()))
        state.focusable = true;
    if (item()->hasActiveFocus())
        state.focused = true;
    if (role() == QAccessible::EditableText)
        if (auto ti = qobject_cast<QQuickTextInput *>(item()))
            state.passwordEdit = ti->echoMode() != QQuickTextInput::Normal;
    if (!item()->isEnabled()) {
        state.focusable = false;
        state.disabled = true;
    }
    return state;
}

QAccessible::Role QAccessibleQuickItem::role() const
{
    // Workaround for setAccessibleRole() not working for
    // Text items. Text items are special since they are defined
    // entirely from C++ (setting the role from QML works.)

    QAccessible::Role role = QAccessible::NoRole;
    if (item())
        role = QQuickItemPrivate::get(item())->effectiveAccessibleRole();
    if (role == QAccessible::NoRole) {
        if (qobject_cast<QQuickText*>(const_cast<QQuickItem *>(item())))
            role = QAccessible::StaticText;
        else if (qobject_cast<QQuickTextInput*>(const_cast<QQuickItem *>(item())))
            role = QAccessible::EditableText;
        else
            role = QAccessible::Client;
    }

    return role;
}

bool QAccessibleQuickItem::isAccessible() const
{
    return item()->d_func()->isAccessible;
}

QStringList QAccessibleQuickItem::actionNames() const
{
    QStringList actions;
    switch (role()) {
    case QAccessible::Link:
    case QAccessible::PushButton:
        actions << QAccessibleActionInterface::pressAction();
        break;
    case QAccessible::RadioButton:
    case QAccessible::CheckBox:
        actions << QAccessibleActionInterface::toggleAction()
                << QAccessibleActionInterface::pressAction();
        break;
    case QAccessible::Slider:
    case QAccessible::SpinBox:
    case QAccessible::ScrollBar:
        actions << QAccessibleActionInterface::increaseAction()
                << QAccessibleActionInterface::decreaseAction();
        break;
    default:
        break;
    }
    if (state().focusable)
        actions.append(QAccessibleActionInterface::setFocusAction());

    // ### The following can lead to duplicate action names.
    if (QQuickAccessibleAttached *attached = QQuickAccessibleAttached::attachedProperties(item()))
        attached->availableActions(&actions);
    return actions;
}

void QAccessibleQuickItem::doAction(const QString &actionName)
{
    bool accepted = false;
    if (actionName == QAccessibleActionInterface::setFocusAction()) {
        item()->forceActiveFocus();
        accepted = true;
    }
    if (QQuickAccessibleAttached *attached = QQuickAccessibleAttached::attachedProperties(item()))
        accepted = attached->doAction(actionName);

    if (accepted)
        return;
    // Look for and call the accessible[actionName]Action() function on the item.
    // This allows for overriding the default action handling.
    const QByteArray functionName = "accessible" + actionName.toLatin1() + "Action";
    if (object()->metaObject()->indexOfMethod(QByteArray(functionName + "()")) != -1) {
        QMetaObject::invokeMethod(object(), functionName);
        return;
    }

    // Role-specific default action handling follows. Items are expected to provide
    // properties according to role conventions. These will then be read and/or updated
    // by the accessibility system.
    //   Checkable roles   : checked
    //   Value-based roles : (via the value interface: value, minimumValue, maximumValue), stepSize
    switch (role()) {
    case QAccessible::RadioButton:
    case QAccessible::CheckBox: {
        QVariant checked = object()->property("checked");
        if (checked.isValid()) {
            if (actionName == QAccessibleActionInterface::toggleAction() ||
                    actionName == QAccessibleActionInterface::pressAction()) {

                object()->setProperty("checked",  QVariant(!checked.toBool()));
            }
        }
        break;
    }
    case QAccessible::Slider:
    case QAccessible::SpinBox:
    case QAccessible::Dial:
    case QAccessible::ScrollBar: {
        if (actionName != QAccessibleActionInterface::increaseAction() &&
                actionName != QAccessibleActionInterface::decreaseAction())
            break;

        // Update the value using QAccessibleValueInterface, respecting
        // the minimum and maximum value (if set). Also check for and
        // use the "stepSize" property on the item
        if (QAccessibleValueInterface *valueIface = valueInterface()) {
            QVariant valueV = valueIface->currentValue();
            qreal newValue = valueV.toReal();

            QVariant stepSizeV = object()->property("stepSize");
            qreal stepSize = stepSizeV.isValid() ? stepSizeV.toReal() : qreal(1.0);
            if (actionName == QAccessibleActionInterface::increaseAction()) {
                newValue += stepSize;
            } else {
                newValue -= stepSize;
            }

            QVariant minimumValueV = valueIface->minimumValue();
            if (minimumValueV.isValid()) {
                newValue = qMax(newValue, minimumValueV.toReal());
            }
            QVariant maximumValueV = valueIface->maximumValue();
            if (maximumValueV.isValid()) {
                newValue = qMin(newValue, maximumValueV.toReal());
            }

            valueIface->setCurrentValue(QVariant(newValue));
        }
        break;
    }
    default:
        break;
    }
}

QStringList QAccessibleQuickItem::keyBindingsForAction(const QString &actionName) const
{
    Q_UNUSED(actionName);
    return QStringList();
}

QString QAccessibleQuickItem::text(QAccessible::Text textType) const
{
    // handles generic behavior not specific to an item
    switch (textType) {
    case QAccessible::Name: {
        QVariant accessibleName = QQuickAccessibleAttached::property(object(), "name");
        if (!accessibleName.isNull())
            return accessibleName.toString();
        break;}
    case QAccessible::Description: {
        QVariant accessibleDecription = QQuickAccessibleAttached::property(object(), "description");
        if (!accessibleDecription.isNull())
            return accessibleDecription.toString();
        break;}
#ifdef Q_ACCESSIBLE_QUICK_ITEM_ENABLE_DEBUG_DESCRIPTION
    case QAccessible::DebugDescription: {
        QString debugString;
        debugString = QString::fromLatin1(object()->metaObject()->className()) + QLatin1Char(' ');
        debugString += isAccessible() ? QLatin1String("enabled") : QLatin1String("disabled");
        return debugString;
        break; }
#endif
    case QAccessible::Value:
    case QAccessible::Help:
    case QAccessible::Accelerator:
    default:
        break;
    }

    // the following block handles item-specific behavior
    if (role() == QAccessible::EditableText) {
        if (textType == QAccessible::Value) {
            if (QTextDocument *doc = textDocument()) {
                return doc->toPlainText();
            }
            QVariant text = object()->property("text");
            return text.toString();
        }
    }

    return QString();
}

void QAccessibleQuickItem::setText(QAccessible::Text textType, const QString &text)
{
    if (role() != QAccessible::EditableText)
        return;
    if (textType != QAccessible::Value)
        return;

    if (QTextDocument *doc = textDocument()) {
        doc->setPlainText(text);
        return;
    }
    auto textPropertyName = "text";
    if (object()->metaObject()->indexOfProperty(textPropertyName) >= 0)
        object()->setProperty(textPropertyName, text);
}

void *QAccessibleQuickItem::interface_cast(QAccessible::InterfaceType t)
{
    QAccessible::Role r = role();
    if (t == QAccessible::ActionInterface)
        return static_cast<QAccessibleActionInterface*>(this);
    if (t == QAccessible::ValueInterface &&
           (r == QAccessible::Slider ||
            r == QAccessible::SpinBox ||
            r == QAccessible::Dial ||
            r == QAccessible::ScrollBar))
       return static_cast<QAccessibleValueInterface*>(this);

    if (t == QAccessible::TextInterface) {
        if (r == QAccessible::EditableText ||
            r == QAccessible::StaticText)
        return static_cast<QAccessibleTextInterface*>(this);
    }

    return QAccessibleObject::interface_cast(t);
}

QVariant QAccessibleQuickItem::currentValue() const
{
    return item()->property("value");
}

void QAccessibleQuickItem::setCurrentValue(const QVariant &value)
{
    item()->setProperty("value", value);
}

QVariant QAccessibleQuickItem::maximumValue() const
{
    return item()->property("maximumValue");
}

QVariant QAccessibleQuickItem::minimumValue() const
{
    return item()->property("minimumValue");
}

QVariant QAccessibleQuickItem::minimumStepSize() const
{
    return item()->property("stepSize");
}

/*!
  \internal
  Shared between QAccessibleQuickItem and QAccessibleQuickView
*/
QRect itemScreenRect(QQuickItem *item)
{
    // ### no window in some cases.
    // ### Should we really check for 0 opacity?
    if (!item->window() ||!item->isVisible() || qFuzzyIsNull(item->opacity())) {
        return QRect();
    }

    QSize itemSize((int)item->width(), (int)item->height());
    // ### If the bounding rect fails, we first try the implicit size, then we go for the
    // parent size. WE MIGHT HAVE TO REVISIT THESE FALLBACKS.
    if (itemSize.isEmpty()) {
        itemSize = QSize((int)item->implicitWidth(), (int)item->implicitHeight());
        if (itemSize.isEmpty() && item->parentItem())
            // ### Seems that the above fallback is not enough, fallback to use the parent size...
            itemSize = QSize((int)item->parentItem()->width(), (int)item->parentItem()->height());
    }

    QPointF scenePoint = item->mapToScene(QPointF(0, 0));
    QPoint screenPos = item->window()->mapToGlobal(scenePoint.toPoint());
    return QRect(screenPos, itemSize);
}

QTextDocument *QAccessibleQuickItem::textDocument() const
{
    QVariant docVariant = item()->property("textDocument");
    if (docVariant.canConvert<QQuickTextDocument*>()) {
        QQuickTextDocument *qqdoc = docVariant.value<QQuickTextDocument*>();
        return qqdoc->textDocument();
    }
    return nullptr;
}

int QAccessibleQuickItem::characterCount() const
{
    if (m_doc) {
        QTextCursor cursor = QTextCursor(m_doc);
        cursor.movePosition(QTextCursor::End);
        return cursor.position();
    }
    return text(QAccessible::Value).size();
}

int QAccessibleQuickItem::cursorPosition() const
{
    QVariant pos = item()->property("cursorPosition");
    return pos.toInt();
}

void QAccessibleQuickItem::setCursorPosition(int position)
{
    item()->setProperty("cursorPosition", position);
}

QString QAccessibleQuickItem::text(int startOffset, int endOffset) const
{
    if (m_doc) {
        QTextCursor cursor = QTextCursor(m_doc);
        cursor.setPosition(startOffset);
        cursor.setPosition(endOffset, QTextCursor::KeepAnchor);
        return cursor.selectedText();
    }
    return text(QAccessible::Value).mid(startOffset, endOffset - startOffset);
}

QString QAccessibleQuickItem::textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                 int *startOffset, int *endOffset) const
{
    Q_ASSERT(startOffset);
    Q_ASSERT(endOffset);

    if (m_doc) {
        QTextCursor cursor = QTextCursor(m_doc);
        cursor.setPosition(offset);
        QPair<int, int> boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);
        cursor.setPosition(boundaries.first - 1);
        boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);

        *startOffset = boundaries.first;
        *endOffset = boundaries.second;

        return text(boundaries.first, boundaries.second);
    } else {
        return QAccessibleTextInterface::textBeforeOffset(offset, boundaryType, startOffset, endOffset);
    }
}

QString QAccessibleQuickItem::textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                int *startOffset, int *endOffset) const
{
    Q_ASSERT(startOffset);
    Q_ASSERT(endOffset);

    if (m_doc) {
        QTextCursor cursor = QTextCursor(m_doc);
        cursor.setPosition(offset);
        QPair<int, int> boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);
        cursor.setPosition(boundaries.second);
        boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);

        *startOffset = boundaries.first;
        *endOffset = boundaries.second;

        return text(boundaries.first, boundaries.second);
    } else {
        return QAccessibleTextInterface::textAfterOffset(offset, boundaryType, startOffset, endOffset);
    }
}

QString QAccessibleQuickItem::textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                             int *startOffset, int *endOffset) const
{
    Q_ASSERT(startOffset);
    Q_ASSERT(endOffset);

    if (m_doc) {
        QTextCursor cursor = QTextCursor(m_doc);
        cursor.setPosition(offset);
        QPair<int, int> boundaries = QAccessible::qAccessibleTextBoundaryHelper(cursor, boundaryType);

        *startOffset = boundaries.first;
        *endOffset = boundaries.second;
        return text(boundaries.first, boundaries.second);
    } else {
        return QAccessibleTextInterface::textAtOffset(offset, boundaryType, startOffset, endOffset);
    }
}

void QAccessibleQuickItem::selection(int selectionIndex, int *startOffset, int *endOffset) const
{
    if (selectionIndex == 0) {
        *startOffset = item()->property("selectionStart").toInt();
        *endOffset = item()->property("selectionEnd").toInt();
    } else {
        *startOffset = 0;
        *endOffset = 0;
    }
}

int QAccessibleQuickItem::selectionCount() const
{
    if (item()->property("selectionStart").toInt() != item()->property("selectionEnd").toInt())
        return 1;
    return 0;
}

void QAccessibleQuickItem::addSelection(int /* startOffset */, int /* endOffset */)
{

}
void QAccessibleQuickItem::removeSelection(int /* selectionIndex */)
{

}
void QAccessibleQuickItem::setSelection(int /* selectionIndex */, int /* startOffset */, int /* endOffset */)
{

}


#endif // accessibility

QT_END_NAMESPACE
