/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVEITEM_P_H
#define QDECLARATIVEITEM_P_H

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

#include "QtQuick1/qdeclarativeitem.h"

#include "QtQuick1/private/qdeclarativeanchors_p.h"
#include "QtQuick1/private/qdeclarativeanchors_p_p.h"
#include "QtQuick1/private/qdeclarativeitemchangelistener_p.h"
#include <QtDeclarative/private/qpodvector_p.h>

#include <QtQuick1/private/qdeclarativestate_p.h>
#include <QtDeclarative/private/qdeclarativenullablevalue_p_p.h>
#include <QtDeclarative/private/qdeclarativenotifier_p.h>
#include <QtDeclarative/private/qdeclarativeglobal_p.h>

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativecontext.h>

#include <QtCore/qlist.h>
#include <QtCore/qrect.h>
#include <QtCore/qdebug.h>
#include <QtGui/qaccessible.h>

#include <QtWidgets/private/qgraphicsitem_p.h>

QT_BEGIN_NAMESPACE

class QNetworkReply;

class QDeclarativeItemKeyFilter;
class QDeclarative1LayoutMirroringAttached;

//### merge into private?
class QDeclarative1Contents : public QObject, public QDeclarativeItemChangeListener
{
    Q_OBJECT
public:
    QDeclarative1Contents(QDeclarativeItem *item);
    ~QDeclarative1Contents();

    QRectF rectF() const;

    void childRemoved(QDeclarativeItem *item);
    void childAdded(QDeclarativeItem *item);

    void calcGeometry() { calcWidth(); calcHeight(); }
    void complete();

Q_SIGNALS:
    void rectChanged(QRectF);

protected:
    void itemGeometryChanged(QDeclarativeItem *item, const QRectF &newGeometry, const QRectF &oldGeometry);
    void itemDestroyed(QDeclarativeItem *item);
    //void itemVisibilityChanged(QDeclarativeItem *item)

private:
    void calcHeight(QDeclarativeItem *changed = 0);
    void calcWidth(QDeclarativeItem *changed = 0);

    QDeclarativeItem *m_item;
    qreal m_x;
    qreal m_y;
    qreal m_width;
    qreal m_height;
};

class Q_QTQUICK1_EXPORT QDeclarativeItemPrivate : public QGraphicsItemPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeItem)

public:
    QDeclarativeItemPrivate()
    : _anchors(0), _contents(0),
      baselineOffset(0),
      _anchorLines(0),
      _stateGroup(0), origin(QDeclarativeItem::Center),
      widthValid(false), heightValid(false),
      componentComplete(true), keepMouse(false),
      smooth(false), transformOriginDirty(true), doneEventPreHandler(false),
      inheritedLayoutMirror(false), effectiveLayoutMirror(false), isMirrorImplicit(true),
      inheritMirrorFromParent(false), inheritMirrorFromItem(false), isAccessible(0), keyHandler(0),
      mWidth(0), mHeight(0), mImplicitWidth(0), mImplicitHeight(0), attachedLayoutDirection(0), hadSubFocusItem(false)
    {
        QGraphicsItemPrivate::acceptedMouseButtons = 0;
        isDeclarativeItem = 1;
        QGraphicsItemPrivate::flags = QGraphicsItem::GraphicsItemFlags(
                                      QGraphicsItem::ItemHasNoContents
                                      | QGraphicsItem::ItemIsFocusable
                                      | QGraphicsItem::ItemNegativeZStacksBehindParent);
    }

    void init(QDeclarativeItem *parent)
    {
        Q_Q(QDeclarativeItem);
        if (parent) {
            QDeclarative_setParent_noEvent(q, parent);
            q->setParentItem(parent);
            QDeclarativeItemPrivate *parentPrivate = QDeclarativeItemPrivate::get(parent);
            setImplicitLayoutMirror(parentPrivate->inheritedLayoutMirror, parentPrivate->inheritMirrorFromParent);
        }
        baselineOffset.invalidate();
        mouseSetsFocus = false;
    }

    bool isMirrored() const {
        return effectiveLayoutMirror;
    }

    // Private Properties
    qreal width() const;
    void setWidth(qreal);
    void resetWidth();

    qreal height() const;
    void setHeight(qreal);
    void resetHeight();

    virtual qreal implicitWidth() const;
    virtual qreal implicitHeight() const;
    virtual void implicitWidthChanged();
    virtual void implicitHeightChanged();

    void resolveLayoutMirror();
    void setImplicitLayoutMirror(bool mirror, bool inherit);
    void setLayoutMirror(bool mirror);

    QDeclarativeListProperty<QObject> data();
    QDeclarativeListProperty<QObject> resources();

    QDeclarativeListProperty<QDeclarative1State> states();
    QDeclarativeListProperty<QDeclarative1Transition> transitions();

    QString state() const;
    void setState(const QString &);

    QDeclarative1AnchorLine left() const;
    QDeclarative1AnchorLine right() const;
    QDeclarative1AnchorLine horizontalCenter() const;
    QDeclarative1AnchorLine top() const;
    QDeclarative1AnchorLine bottom() const;
    QDeclarative1AnchorLine verticalCenter() const;
    QDeclarative1AnchorLine baseline() const;

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

    // transform property
    static int transform_count(QDeclarativeListProperty<QGraphicsTransform> *list);
    static void transform_append(QDeclarativeListProperty<QGraphicsTransform> *list, QGraphicsTransform *);
    static QGraphicsTransform *transform_at(QDeclarativeListProperty<QGraphicsTransform> *list, int);
    static void transform_clear(QDeclarativeListProperty<QGraphicsTransform> *list);

    static QDeclarativeItemPrivate* get(QDeclarativeItem *item)
    {
        return item->d_func();
    }

    QDeclarative1Anchors *anchors() {
        if (!_anchors) {
            Q_Q(QDeclarativeItem);
            _anchors = new QDeclarative1Anchors(q);
            if (!componentComplete)
                _anchors->classBegin();
        }
        return _anchors;
    }
    QDeclarative1Anchors *_anchors;
    QDeclarative1Contents *_contents;

    QDeclarativeNullableValue<qreal> baselineOffset;

    struct AnchorLines {
        AnchorLines(QGraphicsObject *);
        QDeclarative1AnchorLine left;
        QDeclarative1AnchorLine right;
        QDeclarative1AnchorLine hCenter;
        QDeclarative1AnchorLine top;
        QDeclarative1AnchorLine bottom;
        QDeclarative1AnchorLine vCenter;
        QDeclarative1AnchorLine baseline;
    };
    mutable AnchorLines *_anchorLines;
    AnchorLines *anchorLines() const {
        Q_Q(const QDeclarativeItem);
        if (!_anchorLines) _anchorLines =
            new AnchorLines(const_cast<QDeclarativeItem *>(q));
        return _anchorLines;
    }

    enum ChangeType {
        Geometry = 0x01,
        SiblingOrder = 0x02,
        Visibility = 0x04,
        Opacity = 0x08,
        Destroyed = 0x10
    };

    Q_DECLARE_FLAGS(ChangeTypes, ChangeType)

    struct ChangeListener {
        ChangeListener(QDeclarativeItemChangeListener *l, QDeclarativeItemPrivate::ChangeTypes t) : listener(l), types(t) {}
        QDeclarativeItemChangeListener *listener;
        QDeclarativeItemPrivate::ChangeTypes types;
        bool operator==(const ChangeListener &other) const { return listener == other.listener && types == other.types; }
    };

    void addItemChangeListener(QDeclarativeItemChangeListener *listener, ChangeTypes types) {
        changeListeners.append(ChangeListener(listener, types));
    }
    void removeItemChangeListener(QDeclarativeItemChangeListener *, ChangeTypes types);
    QPODVector<ChangeListener,4> changeListeners;

    QDeclarative1StateGroup *_states();
    QDeclarative1StateGroup *_stateGroup;

    QDeclarativeItem::TransformOrigin origin:5;
    bool widthValid:1;
    bool heightValid:1;
    bool componentComplete:1;
    bool keepMouse:1;
    bool smooth:1;
    bool transformOriginDirty : 1;
    bool doneEventPreHandler : 1;
    bool inheritedLayoutMirror:1;
    bool effectiveLayoutMirror:1;
    bool isMirrorImplicit:1;
    bool inheritMirrorFromParent:1;
    bool inheritMirrorFromItem:1;
    bool isAccessible:1;

    QDeclarativeItemKeyFilter *keyHandler;

    qreal mWidth;
    qreal mHeight;
    qreal mImplicitWidth;
    qreal mImplicitHeight;

    QDeclarative1LayoutMirroringAttached* attachedLayoutDirection;

    bool hadSubFocusItem;
    void setAccessibleFlagAndListener();

    QPointF computeTransformOrigin() const;

    virtual void setPosHelper(const QPointF &pos)
    {
        Q_Q(QDeclarativeItem);
        QRectF oldGeometry(this->pos.x(), this->pos.y(), mWidth, mHeight);
        QGraphicsItemPrivate::setPosHelper(pos);
        q->geometryChanged(QRectF(this->pos.x(), this->pos.y(), mWidth, mHeight), oldGeometry);
    }

    // Reimplemented from QGraphicsItemPrivate
    virtual void subFocusItemChange()
    {
        bool hasSubFocusItem = subFocusItem != 0;
        if (((flags & QGraphicsItem::ItemIsFocusScope) || !parent) && hasSubFocusItem != hadSubFocusItem)
            emit q_func()->activeFocusChanged(hasSubFocusItem);
        //see also QDeclarativeItemPrivate::focusChanged
        hadSubFocusItem = hasSubFocusItem;
    }

    // Reimplemented from QGraphicsItemPrivate
    virtual void focusScopeItemChange(bool isSubFocusItem)
    {
        emit q_func()->focusChanged(isSubFocusItem);
    }


    // Reimplemented from QGraphicsItemPrivate
    virtual void siblingOrderChange()
    {
        Q_Q(QDeclarativeItem);
        for(int ii = 0; ii < changeListeners.count(); ++ii) {
            const QDeclarativeItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QDeclarativeItemPrivate::SiblingOrder) {
                change.listener->itemSiblingOrderChanged(q);
            }
        }
    }

    // Reimplemented from QGraphicsItemPrivate
    virtual void transformChanged();

    virtual void focusChanged(bool);

    virtual void mirrorChange() {};

    static qint64 consistentTime;
    static void setConsistentTime(qint64 t);
    static void start(QElapsedTimer &);
    static qint64 elapsed(QElapsedTimer &);
    static qint64 restart(QElapsedTimer &);
};

/*
    Key filters can be installed on a QDeclarativeItem, but not removed.  Currently they
    are only used by attached objects (which are only destroyed on Item
    destruction), so this isn't a problem.  If in future this becomes any form
    of public API, they will have to support removal too.
*/
class QDeclarativeItemKeyFilter
{
public:
    QDeclarativeItemKeyFilter(QDeclarativeItem * = 0);
    virtual ~QDeclarativeItemKeyFilter();

    virtual void keyPressed(QKeyEvent *event, bool post);
    virtual void keyReleased(QKeyEvent *event, bool post);
    virtual void inputMethodEvent(QInputMethodEvent *event, bool post);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;
    virtual void componentComplete();

    bool m_processPost;

private:
    QDeclarativeItemKeyFilter *m_next;
};

class QDeclarative1KeyNavigationAttachedPrivate : public QObjectPrivate
{
public:
    QDeclarative1KeyNavigationAttachedPrivate()
        : QObjectPrivate(),
          left(0), right(0), up(0), down(0), tab(0), backtab(0),
          leftSet(false), rightSet(false), upSet(false), downSet(false),
          tabSet(false), backtabSet(false) {}

    QDeclarativeItem *left;
    QDeclarativeItem *right;
    QDeclarativeItem *up;
    QDeclarativeItem *down;
    QDeclarativeItem *tab;
    QDeclarativeItem *backtab;
    bool leftSet : 1;
    bool rightSet : 1;
    bool upSet : 1;
    bool downSet : 1;
    bool tabSet : 1;
    bool backtabSet : 1;
};

class QDeclarative1KeyNavigationAttached : public QObject, public QDeclarativeItemKeyFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1KeyNavigationAttached)

    Q_PROPERTY(QDeclarativeItem *left READ left WRITE setLeft NOTIFY leftChanged)
    Q_PROPERTY(QDeclarativeItem *right READ right WRITE setRight NOTIFY rightChanged)
    Q_PROPERTY(QDeclarativeItem *up READ up WRITE setUp NOTIFY upChanged)
    Q_PROPERTY(QDeclarativeItem *down READ down WRITE setDown NOTIFY downChanged)
    Q_PROPERTY(QDeclarativeItem *tab READ tab WRITE setTab NOTIFY tabChanged)
    Q_PROPERTY(QDeclarativeItem *backtab READ backtab WRITE setBacktab NOTIFY backtabChanged)
    Q_PROPERTY(Priority priority READ priority WRITE setPriority NOTIFY priorityChanged)

    Q_ENUMS(Priority)

public:
    QDeclarative1KeyNavigationAttached(QObject * = 0);

    QDeclarativeItem *left() const;
    void setLeft(QDeclarativeItem *);
    QDeclarativeItem *right() const;
    void setRight(QDeclarativeItem *);
    QDeclarativeItem *up() const;
    void setUp(QDeclarativeItem *);
    QDeclarativeItem *down() const;
    void setDown(QDeclarativeItem *);
    QDeclarativeItem *tab() const;
    void setTab(QDeclarativeItem *);
    QDeclarativeItem *backtab() const;
    void setBacktab(QDeclarativeItem *);

    enum Priority { BeforeItem, AfterItem };
    Priority priority() const;
    void setPriority(Priority);

    static QDeclarative1KeyNavigationAttached *qmlAttachedProperties(QObject *);

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
    void setFocusNavigation(QDeclarativeItem *currentItem, const char *dir);
};

class QDeclarative1LayoutMirroringAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled RESET resetEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool childrenInherit READ childrenInherit WRITE setChildrenInherit NOTIFY childrenInheritChanged)

public:
    explicit QDeclarative1LayoutMirroringAttached(QObject *parent = 0);

    bool enabled() const;
    void setEnabled(bool);
    void resetEnabled();

    bool childrenInherit() const;
    void setChildrenInherit(bool);

    static QDeclarative1LayoutMirroringAttached *qmlAttachedProperties(QObject *);
Q_SIGNALS:
    void enabledChanged();
    void childrenInheritChanged();
private:
    friend class QDeclarativeItemPrivate;
    QDeclarativeItemPrivate *itemPrivate;
};

class QDeclarative1KeysAttachedPrivate : public QObjectPrivate
{
public:
    QDeclarative1KeysAttachedPrivate()
        : QObjectPrivate(), inPress(false), inRelease(false)
        , inIM(false), enabled(true), imeItem(0), item(0)
    {}

    bool isConnected(const char *signalName);

    QGraphicsItem *finalFocusProxy(QGraphicsItem *item) const
    {
        QGraphicsItem *fp;
        while ((fp = item->focusProxy()))
            item = fp;
        return item;
    }

    //loop detection
    bool inPress:1;
    bool inRelease:1;
    bool inIM:1;

    bool enabled : 1;

    QGraphicsItem *imeItem;
    QList<QDeclarativeItem *> targets;
    QDeclarativeItem *item;
};

class QDeclarative1KeysAttached : public QObject, public QDeclarativeItemKeyFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1KeysAttached)

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QDeclarativeListProperty<QDeclarativeItem> forwardTo READ forwardTo)
    Q_PROPERTY(Priority priority READ priority WRITE setPriority NOTIFY priorityChanged)

    Q_ENUMS(Priority)

public:
    QDeclarative1KeysAttached(QObject *parent=0);
    ~QDeclarative1KeysAttached();

    bool enabled() const { Q_D(const QDeclarative1KeysAttached); return d->enabled; }
    void setEnabled(bool enabled) {
        Q_D(QDeclarative1KeysAttached);
        if (enabled != d->enabled) {
            d->enabled = enabled;
            emit enabledChanged();
        }
    }

    enum Priority { BeforeItem, AfterItem};
    Priority priority() const;
    void setPriority(Priority);

    QDeclarativeListProperty<QDeclarativeItem> forwardTo() {
        Q_D(QDeclarative1KeysAttached);
        return QDeclarativeListProperty<QDeclarativeItem>(this, d->targets);
    }

    virtual void componentComplete();

    static QDeclarative1KeysAttached *qmlAttachedProperties(QObject *);

Q_SIGNALS:
    void enabledChanged();
    void priorityChanged();
    void pressed(QDeclarative1KeyEvent *event);
    void released(QDeclarative1KeyEvent *event);
    void digit0Pressed(QDeclarative1KeyEvent *event);
    void digit1Pressed(QDeclarative1KeyEvent *event);
    void digit2Pressed(QDeclarative1KeyEvent *event);
    void digit3Pressed(QDeclarative1KeyEvent *event);
    void digit4Pressed(QDeclarative1KeyEvent *event);
    void digit5Pressed(QDeclarative1KeyEvent *event);
    void digit6Pressed(QDeclarative1KeyEvent *event);
    void digit7Pressed(QDeclarative1KeyEvent *event);
    void digit8Pressed(QDeclarative1KeyEvent *event);
    void digit9Pressed(QDeclarative1KeyEvent *event);

    void leftPressed(QDeclarative1KeyEvent *event);
    void rightPressed(QDeclarative1KeyEvent *event);
    void upPressed(QDeclarative1KeyEvent *event);
    void downPressed(QDeclarative1KeyEvent *event);
    void tabPressed(QDeclarative1KeyEvent *event);
    void backtabPressed(QDeclarative1KeyEvent *event);

    void asteriskPressed(QDeclarative1KeyEvent *event);
    void numberSignPressed(QDeclarative1KeyEvent *event);
    void escapePressed(QDeclarative1KeyEvent *event);
    void returnPressed(QDeclarative1KeyEvent *event);
    void enterPressed(QDeclarative1KeyEvent *event);
    void deletePressed(QDeclarative1KeyEvent *event);
    void spacePressed(QDeclarative1KeyEvent *event);
    void backPressed(QDeclarative1KeyEvent *event);
    void cancelPressed(QDeclarative1KeyEvent *event);
    void selectPressed(QDeclarative1KeyEvent *event);
    void yesPressed(QDeclarative1KeyEvent *event);
    void noPressed(QDeclarative1KeyEvent *event);
    void context1Pressed(QDeclarative1KeyEvent *event);
    void context2Pressed(QDeclarative1KeyEvent *event);
    void context3Pressed(QDeclarative1KeyEvent *event);
    void context4Pressed(QDeclarative1KeyEvent *event);
    void callPressed(QDeclarative1KeyEvent *event);
    void hangupPressed(QDeclarative1KeyEvent *event);
    void flipPressed(QDeclarative1KeyEvent *event);
    void menuPressed(QDeclarative1KeyEvent *event);
    void volumeUpPressed(QDeclarative1KeyEvent *event);
    void volumeDownPressed(QDeclarative1KeyEvent *event);

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


Q_DECLARE_OPERATORS_FOR_FLAGS(QDeclarativeItemPrivate::ChangeTypes);

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarative1KeysAttached)
QML_DECLARE_TYPEINFO(QDeclarative1KeysAttached, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPE(QDeclarative1KeyNavigationAttached)
QML_DECLARE_TYPEINFO(QDeclarative1KeyNavigationAttached, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPE(QDeclarative1LayoutMirroringAttached)
QML_DECLARE_TYPEINFO(QDeclarative1LayoutMirroringAttached, QML_HAS_ATTACHED_PROPERTIES)

#endif // QDECLARATIVEITEM_P_H
