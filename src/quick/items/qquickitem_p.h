// Commit: 5c783d0a9a912816813945387903857a314040b5
/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKITEM_P_H
#define QQUICKITEM_P_H

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

#include "qquickitem.h"

#include "qquickanchors_p.h"
#include "qquickanchors_p_p.h"
#include "qquickitemchangelistener_p.h"

#include "qquickcanvas_p.h"

#include <QtQuick/qsgnode.h>
#include "qquickclipnode_p.h"

#include <private/qpodvector_p.h>
#include <QtQuick/private/qdeclarativestate_p.h>
#include <private/qdeclarativenullablevalue_p_p.h>
#include <private/qdeclarativenotifier_p.h>
#include <private/qdeclarativeglobal_p.h>

#include <qdeclarative.h>
#include <qdeclarativecontext.h>

#include <QtCore/qlist.h>
#include <QtCore/qdebug.h>
#include <QtCore/qelapsedtimer.h>

QT_BEGIN_NAMESPACE

class QNetworkReply;
class QQuickItemKeyFilter;
class QQuickLayoutMirroringAttached;

class QQuickContents : public QQuickItemChangeListener
{
public:
    QQuickContents(QQuickItem *item);
    ~QQuickContents();

    QRectF rectF() const { return QRectF(m_x, m_y, m_width, m_height); }

    inline void calcGeometry(QQuickItem *changed = 0);
    void complete();

protected:
    void itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry);
    void itemDestroyed(QQuickItem *item);
    void itemChildAdded(QQuickItem *, QQuickItem *);
    void itemChildRemoved(QQuickItem *, QQuickItem *);
    //void itemVisibilityChanged(QQuickItem *item)

private:
    bool calcHeight(QQuickItem *changed = 0);
    bool calcWidth(QQuickItem *changed = 0);
    void updateRect();

    QQuickItem *m_item;
    qreal m_x;
    qreal m_y;
    qreal m_width;
    qreal m_height;
};

void QQuickContents::calcGeometry(QQuickItem *changed)
{
    bool wChanged = calcWidth(changed);
    bool hChanged = calcHeight(changed);
    if (wChanged || hChanged)
        updateRect();
}

class QQuickTransformPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickTransform);
public:
    static QQuickTransformPrivate* get(QQuickTransform *transform) { return transform->d_func(); }

    QQuickTransformPrivate();

    QList<QQuickItem *> items;
};

class Q_QUICK_EXPORT QQuickItemPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickItem)

public:
    static QQuickItemPrivate* get(QQuickItem *item) { return item->d_func(); }
    static const QQuickItemPrivate* get(const QQuickItem *item) { return item->d_func(); }

    static void registerAccessorProperties();

    QQuickItemPrivate();
    ~QQuickItemPrivate();
    void init(QQuickItem *parent);

    QDeclarativeListProperty<QObject> data();
    QDeclarativeListProperty<QObject> resources();
    QDeclarativeListProperty<QQuickItem> children();

    QDeclarativeListProperty<QDeclarativeState> states();
    QDeclarativeListProperty<QDeclarativeTransition> transitions();

    QString state() const;
    void setState(const QString &);

    QQuickAnchorLine left() const;
    QQuickAnchorLine right() const;
    QQuickAnchorLine horizontalCenter() const;
    QQuickAnchorLine top() const;
    QQuickAnchorLine bottom() const;
    QQuickAnchorLine verticalCenter() const;
    QQuickAnchorLine baseline() const;

    // data property
    static void data_append(QDeclarativeListProperty<QObject> *, QObject *);
    static int data_count(QDeclarativeListProperty<QObject> *);
    static QObject *data_at(QDeclarativeListProperty<QObject> *, int);
    static void data_clear(QDeclarativeListProperty<QObject> *);

    // resources property
    static QObject *resources_at(QDeclarativeListProperty<QObject> *, int);
    static void resources_append(QDeclarativeListProperty<QObject> *, QObject *);
    static int resources_count(QDeclarativeListProperty<QObject> *);
    static void resources_clear(QDeclarativeListProperty<QObject> *);

    // children property
    static void children_append(QDeclarativeListProperty<QQuickItem> *, QQuickItem *);
    static int children_count(QDeclarativeListProperty<QQuickItem> *);
    static QQuickItem *children_at(QDeclarativeListProperty<QQuickItem> *, int);
    static void children_clear(QDeclarativeListProperty<QQuickItem> *);

    // transform property
    static int transform_count(QDeclarativeListProperty<QQuickTransform> *list);
    static void transform_append(QDeclarativeListProperty<QQuickTransform> *list, QQuickTransform *);
    static QQuickTransform *transform_at(QDeclarativeListProperty<QQuickTransform> *list, int);
    static void transform_clear(QDeclarativeListProperty<QQuickTransform> *list);

    QQuickAnchors *anchors() const;
    mutable QQuickAnchors *_anchors;
    QQuickContents *_contents;

    QDeclarativeNullableValue<qreal> baselineOffset;

    struct AnchorLines {
        AnchorLines(QQuickItem *);
        QQuickAnchorLine left;
        QQuickAnchorLine right;
        QQuickAnchorLine hCenter;
        QQuickAnchorLine top;
        QQuickAnchorLine bottom;
        QQuickAnchorLine vCenter;
        QQuickAnchorLine baseline;
    };
    mutable AnchorLines *_anchorLines;
    AnchorLines *anchorLines() const;

    enum ChangeType {
        Geometry = 0x01,
        SiblingOrder = 0x02,
        Visibility = 0x04,
        Opacity = 0x08,
        Destroyed = 0x10,
        Parent = 0x20,
        Children = 0x40,
        Rotation = 0x80,
    };

    Q_DECLARE_FLAGS(ChangeTypes, ChangeType)

    enum GeometryChangeType {
        NoChange = 0,
        XChange = 0x01,
        YChange = 0x02,
        WidthChange = 0x04,
        HeightChange = 0x08,
        SizeChange = WidthChange | HeightChange,
        GeometryChange = XChange | YChange | SizeChange
    };

    Q_DECLARE_FLAGS(GeometryChangeTypes, GeometryChangeType)

    struct ChangeListener {
        ChangeListener(QQuickItemChangeListener *l, QQuickItemPrivate::ChangeTypes t) : listener(l), types(t), gTypes(GeometryChange) {}
        ChangeListener(QQuickItemChangeListener *l, QQuickItemPrivate::GeometryChangeTypes gt) : listener(l), types(Geometry), gTypes(gt) {}
        QQuickItemChangeListener *listener;
        QQuickItemPrivate::ChangeTypes types;
        QQuickItemPrivate::GeometryChangeTypes gTypes;  //NOTE: not used for ==
        bool operator==(const ChangeListener &other) const { return listener == other.listener && types == other.types; }
    };

    void addItemChangeListener(QQuickItemChangeListener *listener, ChangeTypes types) {
        changeListeners.append(ChangeListener(listener, types));
    }
    void removeItemChangeListener(QQuickItemChangeListener *, ChangeTypes types);
    void updateOrAddGeometryChangeListener(QQuickItemChangeListener *listener, GeometryChangeTypes types);
    void updateOrRemoveGeometryChangeListener(QQuickItemChangeListener *listener, GeometryChangeTypes types);
    QPODVector<ChangeListener,4> changeListeners;

    QDeclarativeStateGroup *_states();
    QDeclarativeStateGroup *_stateGroup;

    QQuickItem::TransformOrigin origin:5;
    quint32 flags:5;
    bool widthValid:1;
    bool heightValid:1;
    bool componentComplete:1;
    bool keepMouse:1;
    bool keepTouch:1;
    bool hoverEnabled:1;
    bool smooth:1;
    bool focus:1;
    bool activeFocus:1;
    bool notifiedFocus:1;
    bool notifiedActiveFocus:1;
    bool filtersChildMouseEvents:1;
    bool explicitVisible:1;
    bool effectiveVisible:1;
    bool explicitEnable:1;
    bool effectiveEnable:1;
    bool polishScheduled:1;
    bool inheritedLayoutMirror:1;
    bool effectiveLayoutMirror:1;
    bool isMirrorImplicit:1;
    bool inheritMirrorFromParent:1;
    bool inheritMirrorFromItem:1;
    bool childrenDoNotOverlap:1;

    QQuickCanvas *canvas;
    QSGContext *sceneGraphContext() const { Q_ASSERT(canvas); return static_cast<QQuickCanvasPrivate *>(QObjectPrivate::get(canvas))->context; }

    QQuickItem *parentItem;
    QDeclarativeNotifier parentNotifier;

    QList<QQuickItem *> childItems;
    mutable QList<QQuickItem *> *sortedChildItems;
    QList<QQuickItem *> paintOrderChildItems() const;
    void addChild(QQuickItem *);
    void removeChild(QQuickItem *);
    void siblingOrderChanged();

    inline void markSortedChildrenDirty(QQuickItem *child) {
        // If sortedChildItems == &childItems then all in childItems have z == 0
        // and we don't need to invalidate if the changed item also has z == 0.
        if (child->z() != 0. || sortedChildItems != &childItems) {
            if (sortedChildItems != &childItems)
                delete sortedChildItems;
            sortedChildItems = 0;
        }
    }

    class InitializationState {
    public:
        QQuickItem *getFocusScope(QQuickItem *item);
        void clear();
        void clear(QQuickItem *focusScope);
    private:
        QQuickItem *focusScope;
    };
    void initCanvas(InitializationState *, QQuickCanvas *);

    QQuickItem *subFocusItem;

    QTransform canvasToItemTransform() const;
    QTransform itemToCanvasTransform() const;
    void itemToParentTransform(QTransform &) const;

    qreal x;
    qreal y;
    qreal width;
    qreal height;
    qreal implicitWidth;
    qreal implicitHeight;

    qreal z;
    qreal scale;
    qreal rotation;
    qreal opacity;

    QQuickLayoutMirroringAttached* attachedLayoutDirection;

    Qt::MouseButtons acceptedMouseButtons;
    Qt::InputMethodHints imHints;

    QPointF transformOriginPoint;

    virtual qreal getImplicitWidth() const;
    virtual qreal getImplicitHeight() const;
    virtual void implicitWidthChanged();
    virtual void implicitHeightChanged();

    void resolveLayoutMirror();
    void setImplicitLayoutMirror(bool mirror, bool inherit);
    void setLayoutMirror(bool mirror);
    bool isMirrored() const {
        return effectiveLayoutMirror;
    }

    void emitChildrenRectChanged(const QRectF &rect) {
        Q_Q(QQuickItem);
        emit q->childrenRectChanged(rect);
    }

    QPointF computeTransformOrigin() const;
    QList<QQuickTransform *> transforms;
    virtual void transformChanged();

    QQuickItemKeyFilter *keyHandler;
    void deliverKeyEvent(QKeyEvent *);
    void deliverInputMethodEvent(QInputMethodEvent *);
    void deliverFocusEvent(QFocusEvent *);
    void deliverMouseEvent(QMouseEvent *);
    void deliverWheelEvent(QWheelEvent *);
    void deliverTouchEvent(QTouchEvent *);
    void deliverHoverEvent(QHoverEvent *);
    void deliverDragEvent(QEvent *);

    bool calcEffectiveVisible() const;
    void setEffectiveVisibleRecur(bool);
    bool calcEffectiveEnable() const;
    void setEffectiveEnableRecur(bool);

    // XXX todo
    enum DirtyType {
        TransformOrigin         = 0x00000001,
        Transform               = 0x00000002,
        BasicTransform          = 0x00000004,
        Position                = 0x00000008,
        Size                    = 0x00000010,

        ZValue                  = 0x00000020,
        Content                 = 0x00000040,
        Smooth                  = 0x00000080,
        OpacityValue            = 0x00000100,
        ChildrenChanged         = 0x00000200,
        ChildrenStackingChanged = 0x00000400,
        ParentChanged           = 0x00000800,

        Clip                    = 0x00001000,
        Canvas                  = 0x00002000,

        EffectReference         = 0x00008000,
        Visible                 = 0x00010000,
        HideReference           = 0x00020000,
        // When you add an attribute here, don't forget to update
        // dirtyToString()

        TransformUpdateMask     = TransformOrigin | Transform | BasicTransform | Position | Size | Canvas,
        ComplexTransformUpdateMask     = Transform | Canvas,
        ContentUpdateMask       = Size | Content | Smooth | Canvas,
        ChildrenUpdateMask      = ChildrenChanged | ChildrenStackingChanged | EffectReference | Canvas,

    };
    quint32 dirtyAttributes;
    QString dirtyToString() const;
    void dirty(DirtyType);
    void addToDirtyList();
    void removeFromDirtyList();
    QQuickItem *nextDirtyItem;
    QQuickItem**prevDirtyItem;

    inline QSGTransformNode *itemNode();
    inline QSGNode *childContainerNode();

    /*
      QSGNode order is:
         - itemNode
         - (opacityNode)
         - (clipNode)
         - (effectNode)
         - groupNode
     */

    QSGTransformNode *itemNodeInstance;
    QSGOpacityNode *opacityNode;
    QQuickDefaultClipNode *clipNode;
    QSGRootNode *rootNode;
    QSGNode *groupNode;
    QSGNode *paintNode;
    QSGNode *beforePaintNode;

    virtual QSGTransformNode *createTransformNode();

    // A reference from an effect item means that this item is used by the effect, so
    // it should insert a root node.
    void refFromEffectItem(bool hide);
    void derefFromEffectItem(bool unhide);
    int effectRefCount;
    int hideRefCount;

    void itemChange(QQuickItem::ItemChange, const QQuickItem::ItemChangeData &);

    virtual void mirrorChange() {}

    static qint64 consistentTime;
    static void setConsistentTime(qint64 t);
    static void start(QElapsedTimer &);
    static qint64 elapsed(QElapsedTimer &);
    static qint64 restart(QElapsedTimer &);
};

/*
    Key filters can be installed on a QQuickItem, but not removed.  Currently they
    are only used by attached objects (which are only destroyed on Item
    destruction), so this isn't a problem.  If in future this becomes any form
    of public API, they will have to support removal too.
*/
class QQuickItemKeyFilter
{
public:
    QQuickItemKeyFilter(QQuickItem * = 0);
    virtual ~QQuickItemKeyFilter();

    virtual void keyPressed(QKeyEvent *event, bool post);
    virtual void keyReleased(QKeyEvent *event, bool post);
    virtual void inputMethodEvent(QInputMethodEvent *event, bool post);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;
    virtual void componentComplete();

    bool m_processPost;

private:
    QQuickItemKeyFilter *m_next;
};

class QQuickKeyNavigationAttachedPrivate : public QObjectPrivate
{
public:
    QQuickKeyNavigationAttachedPrivate()
        : QObjectPrivate(),
          left(0), right(0), up(0), down(0), tab(0), backtab(0),
          leftSet(false), rightSet(false), upSet(false), downSet(false),
          tabSet(false), backtabSet(false) {}

    QQuickItem *left;
    QQuickItem *right;
    QQuickItem *up;
    QQuickItem *down;
    QQuickItem *tab;
    QQuickItem *backtab;
    bool leftSet : 1;
    bool rightSet : 1;
    bool upSet : 1;
    bool downSet : 1;
    bool tabSet : 1;
    bool backtabSet : 1;
};

class QQuickKeyNavigationAttached : public QObject, public QQuickItemKeyFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickKeyNavigationAttached)

    Q_PROPERTY(QQuickItem *left READ left WRITE setLeft NOTIFY leftChanged)
    Q_PROPERTY(QQuickItem *right READ right WRITE setRight NOTIFY rightChanged)
    Q_PROPERTY(QQuickItem *up READ up WRITE setUp NOTIFY upChanged)
    Q_PROPERTY(QQuickItem *down READ down WRITE setDown NOTIFY downChanged)
    Q_PROPERTY(QQuickItem *tab READ tab WRITE setTab NOTIFY tabChanged)
    Q_PROPERTY(QQuickItem *backtab READ backtab WRITE setBacktab NOTIFY backtabChanged)
    Q_PROPERTY(Priority priority READ priority WRITE setPriority NOTIFY priorityChanged)

    Q_ENUMS(Priority)

public:
    QQuickKeyNavigationAttached(QObject * = 0);

    QQuickItem *left() const;
    void setLeft(QQuickItem *);
    QQuickItem *right() const;
    void setRight(QQuickItem *);
    QQuickItem *up() const;
    void setUp(QQuickItem *);
    QQuickItem *down() const;
    void setDown(QQuickItem *);
    QQuickItem *tab() const;
    void setTab(QQuickItem *);
    QQuickItem *backtab() const;
    void setBacktab(QQuickItem *);

    enum Priority { BeforeItem, AfterItem };
    Priority priority() const;
    void setPriority(Priority);

    static QQuickKeyNavigationAttached *qmlAttachedProperties(QObject *);

Q_SIGNALS:
    void leftChanged();
    void rightChanged();
    void upChanged();
    void downChanged();
    void tabChanged();
    void backtabChanged();
    void priorityChanged();

private:
    virtual void keyPressed(QKeyEvent *event, bool post);
    virtual void keyReleased(QKeyEvent *event, bool post);
    void setFocusNavigation(QQuickItem *currentItem, const char *dir);
};

class QQuickLayoutMirroringAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled RESET resetEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool childrenInherit READ childrenInherit WRITE setChildrenInherit NOTIFY childrenInheritChanged)

public:
    explicit QQuickLayoutMirroringAttached(QObject *parent = 0);

    bool enabled() const;
    void setEnabled(bool);
    void resetEnabled();

    bool childrenInherit() const;
    void setChildrenInherit(bool);

    static QQuickLayoutMirroringAttached *qmlAttachedProperties(QObject *);
Q_SIGNALS:
    void enabledChanged();
    void childrenInheritChanged();
private:
    friend class QQuickItemPrivate;
    QQuickItemPrivate *itemPrivate;
};

class QQuickKeysAttachedPrivate : public QObjectPrivate
{
public:
    QQuickKeysAttachedPrivate()
        : QObjectPrivate(), inPress(false), inRelease(false)
        , inIM(false), enabled(true), imeItem(0), item(0)
    {}

    bool isConnected(const char *signalName);

    //loop detection
    bool inPress:1;
    bool inRelease:1;
    bool inIM:1;

    bool enabled : 1;

    QQuickItem *imeItem;
    QList<QQuickItem *> targets;
    QQuickItem *item;
};

class QQuickKeysAttached : public QObject, public QQuickItemKeyFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickKeysAttached)

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QDeclarativeListProperty<QQuickItem> forwardTo READ forwardTo)
    Q_PROPERTY(Priority priority READ priority WRITE setPriority NOTIFY priorityChanged)

    Q_ENUMS(Priority)

public:
    QQuickKeysAttached(QObject *parent=0);
    ~QQuickKeysAttached();

    bool enabled() const { Q_D(const QQuickKeysAttached); return d->enabled; }
    void setEnabled(bool enabled) {
        Q_D(QQuickKeysAttached);
        if (enabled != d->enabled) {
            d->enabled = enabled;
            emit enabledChanged();
        }
    }

    enum Priority { BeforeItem, AfterItem};
    Priority priority() const;
    void setPriority(Priority);

    QDeclarativeListProperty<QQuickItem> forwardTo() {
        Q_D(QQuickKeysAttached);
        return QDeclarativeListProperty<QQuickItem>(this, d->targets);
    }

    virtual void componentComplete();

    static QQuickKeysAttached *qmlAttachedProperties(QObject *);

Q_SIGNALS:
    void enabledChanged();
    void priorityChanged();
    void pressed(QQuickKeyEvent *event);
    void released(QQuickKeyEvent *event);
    void digit0Pressed(QQuickKeyEvent *event);
    void digit1Pressed(QQuickKeyEvent *event);
    void digit2Pressed(QQuickKeyEvent *event);
    void digit3Pressed(QQuickKeyEvent *event);
    void digit4Pressed(QQuickKeyEvent *event);
    void digit5Pressed(QQuickKeyEvent *event);
    void digit6Pressed(QQuickKeyEvent *event);
    void digit7Pressed(QQuickKeyEvent *event);
    void digit8Pressed(QQuickKeyEvent *event);
    void digit9Pressed(QQuickKeyEvent *event);

    void leftPressed(QQuickKeyEvent *event);
    void rightPressed(QQuickKeyEvent *event);
    void upPressed(QQuickKeyEvent *event);
    void downPressed(QQuickKeyEvent *event);
    void tabPressed(QQuickKeyEvent *event);
    void backtabPressed(QQuickKeyEvent *event);

    void asteriskPressed(QQuickKeyEvent *event);
    void numberSignPressed(QQuickKeyEvent *event);
    void escapePressed(QQuickKeyEvent *event);
    void returnPressed(QQuickKeyEvent *event);
    void enterPressed(QQuickKeyEvent *event);
    void deletePressed(QQuickKeyEvent *event);
    void spacePressed(QQuickKeyEvent *event);
    void backPressed(QQuickKeyEvent *event);
    void cancelPressed(QQuickKeyEvent *event);
    void selectPressed(QQuickKeyEvent *event);
    void yesPressed(QQuickKeyEvent *event);
    void noPressed(QQuickKeyEvent *event);
    void context1Pressed(QQuickKeyEvent *event);
    void context2Pressed(QQuickKeyEvent *event);
    void context3Pressed(QQuickKeyEvent *event);
    void context4Pressed(QQuickKeyEvent *event);
    void callPressed(QQuickKeyEvent *event);
    void hangupPressed(QQuickKeyEvent *event);
    void flipPressed(QQuickKeyEvent *event);
    void menuPressed(QQuickKeyEvent *event);
    void volumeUpPressed(QQuickKeyEvent *event);
    void volumeDownPressed(QQuickKeyEvent *event);

private:
    virtual void keyPressed(QKeyEvent *event, bool post);
    virtual void keyReleased(QKeyEvent *event, bool post);
    virtual void inputMethodEvent(QInputMethodEvent *, bool post);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;

    const QByteArray keyToSignal(int key) {
        QByteArray keySignal;
        if (key >= Qt::Key_0 && key <= Qt::Key_9) {
            keySignal = "digit0Pressed";
            keySignal[5] = '0' + (key - Qt::Key_0);
        } else {
            int i = 0;
            while (sigMap[i].key && sigMap[i].key != key)
                ++i;
            keySignal = sigMap[i].sig;
        }
        return keySignal;
    }

    struct SigMap {
        int key;
        const char *sig;
    };

    static const SigMap sigMap[];
};

QSGTransformNode *QQuickItemPrivate::itemNode()
{
    if (!itemNodeInstance) {
        itemNodeInstance = createTransformNode();
        itemNodeInstance->setFlag(QSGNode::OwnedByParent, false);
#ifdef QML_RUNTIME_TESTING
        Q_Q(QQuickItem);
        itemNodeInstance->description = QString::fromLatin1("QQuickItem(%1)").arg(QString::fromLatin1(q->metaObject()->className()));
#endif
    }
    return itemNodeInstance;
}

QSGNode *QQuickItemPrivate::childContainerNode()
{
    if (!groupNode) {
        groupNode = new QSGNode();
        if (rootNode)
            rootNode->appendChildNode(groupNode);
        else if (clipNode)
            clipNode->appendChildNode(groupNode);
        else if (opacityNode)
            opacityNode->appendChildNode(groupNode);
        else
            itemNode()->appendChildNode(groupNode);
        groupNode->setFlag(QSGNode::ChildrenDoNotOverlap, childrenDoNotOverlap);
#ifdef QML_RUNTIME_TESTING
        groupNode->description = QLatin1String("group");
#endif
    }
    return groupNode;
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickItemPrivate::ChangeTypes);

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickKeysAttached)
QML_DECLARE_TYPEINFO(QQuickKeysAttached, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPE(QQuickKeyNavigationAttached)
QML_DECLARE_TYPEINFO(QQuickKeyNavigationAttached, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPE(QQuickLayoutMirroringAttached)
QML_DECLARE_TYPEINFO(QQuickLayoutMirroringAttached, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKITEM_P_H
