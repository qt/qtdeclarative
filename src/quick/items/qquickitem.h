/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKITEM_H
#define QQUICKITEM_H

#include <QtQuick/qtquickglobal.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlcomponent.h>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtGui/qevent.h>
#include <QtGui/qfont.h>
#include <QtGui/qaccessible.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickTransformPrivate;
class QQuickTransform : public QObject
{
    Q_OBJECT
public:
    QQuickTransform(QObject *parent = 0);
    ~QQuickTransform();

    void appendToItem(QQuickItem *);
    void prependToItem(QQuickItem *);

    virtual void applyTo(QMatrix4x4 *matrix) const = 0;

protected Q_SLOTS:
    void update();

protected:
    QQuickTransform(QQuickTransformPrivate &dd, QObject *parent);

private:
    Q_DECLARE_PRIVATE(QQuickTransform)
};

class QQuickItemLayer;
class QQmlV8Function;
class QQuickState;
class QQuickAnchorLine;
class QQuickTransition;
class QQuickKeyEvent;
class QQuickAnchors;
class QQuickItemPrivate;
class QQuickCanvas;
class QSGEngine;
class QTouchEvent;
class QSGNode;
class QSGTransformNode;
class QSGTextureProvider;

class Q_QUICK_EXPORT QQuickItem : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QQuickItem *parent READ parentItem WRITE setParentItem NOTIFY parentChanged DESIGNABLE false FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQmlListProperty<QObject> data READ data DESIGNABLE false)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQmlListProperty<QObject> resources READ resources DESIGNABLE false)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQmlListProperty<QQuickItem> children READ children NOTIFY childrenChanged DESIGNABLE false)

    Q_PROPERTY(QPointF pos READ pos FINAL)
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged FINAL)
    Q_PROPERTY(qreal z READ z WRITE setZ NOTIFY zChanged FINAL)
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged RESET resetWidth FINAL)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged RESET resetHeight FINAL)

    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged FINAL)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQmlListProperty<QQuickItem> visibleChildren READ visibleChildren NOTIFY visibleChildrenChanged DESIGNABLE false)

    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQmlListProperty<QQuickState> states READ states DESIGNABLE false)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQmlListProperty<QQuickTransition> transitions READ transitions DESIGNABLE false)
    Q_PROPERTY(QString state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(QRectF childrenRect READ childrenRect NOTIFY childrenRectChanged DESIGNABLE false FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchors * anchors READ anchors DESIGNABLE false CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine left READ left CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine right READ right CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine horizontalCenter READ horizontalCenter CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine top READ top CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine bottom READ bottom CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine verticalCenter READ verticalCenter CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine baseline READ baseline CONSTANT FINAL)
    Q_PROPERTY(qreal baselineOffset READ baselineOffset WRITE setBaselineOffset NOTIFY baselineOffsetChanged)

    Q_PROPERTY(bool clip READ clip WRITE setClip NOTIFY clipChanged)

    Q_PROPERTY(bool focus READ hasFocus WRITE setFocus NOTIFY focusChanged FINAL)
    Q_PROPERTY(bool activeFocus READ hasActiveFocus NOTIFY activeFocusChanged FINAL)

    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(TransformOrigin transformOrigin READ transformOrigin WRITE setTransformOrigin NOTIFY transformOriginChanged)
    Q_PROPERTY(QPointF transformOriginPoint READ transformOriginPoint)  // XXX todo - notify?
    Q_PROPERTY(QQmlListProperty<QQuickTransform> transform READ transform DESIGNABLE false FINAL)

    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY smoothChanged)
    Q_PROPERTY(qreal implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY implicitWidthChanged)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight WRITE setImplicitHeight NOTIFY implicitHeightChanged)

    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickItemLayer *layer READ layer DESIGNABLE false CONSTANT FINAL)

    Q_ENUMS(TransformOrigin)
    Q_CLASSINFO("DefaultProperty", "data")
    Q_CLASSINFO("qt_HasQmlAccessors", "true")

public:
    enum Flag {
        ItemClipsChildrenToShape  = 0x01,
        ItemAcceptsInputMethod    = 0x02,
        ItemIsFocusScope          = 0x04,
        ItemHasContents           = 0x08,
        ItemAcceptsDrops          = 0x10
        // Remember to increment the size of QQuickItemPrivate::flags
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum ItemChange {
        ItemChildAddedChange,      // value.item
        ItemChildRemovedChange,    // value.item
        ItemSceneChange,           // value.canvas
        ItemVisibleHasChanged,     // value.boolValue
        ItemParentHasChanged,      // value.item
        ItemOpacityHasChanged,     // value.realValue
        ItemActiveFocusHasChanged, // value.boolValue
        ItemRotationHasChanged     // value.realValue
    };

    union ItemChangeData {
        ItemChangeData(QQuickItem *v) : item(v) {}
        ItemChangeData(QQuickCanvas *v) : canvas(v) {}
        ItemChangeData(qreal v) : realValue(v) {}
        ItemChangeData(bool v) : boolValue(v) {}

        QQuickItem *item;
        QQuickCanvas *canvas;
        qreal realValue;
        bool boolValue;
    };

    enum TransformOrigin {
        TopLeft, Top, TopRight,
        Left, Center, Right,
        BottomLeft, Bottom, BottomRight
    };

    QQuickItem(QQuickItem *parent = 0);
    virtual ~QQuickItem();

    QSGEngine *sceneGraphEngine() const;

    QQuickCanvas *canvas() const;
    QQuickItem *parentItem() const;
    void setParentItem(QQuickItem *parent);
    void stackBefore(const QQuickItem *);
    void stackAfter(const QQuickItem *);

    QRectF childrenRect();
    QList<QQuickItem *> childItems() const;

    bool clip() const;
    void setClip(bool);

    QString state() const;
    void setState(const QString &);

    qreal baselineOffset() const;
    void setBaselineOffset(qreal);

    QQmlListProperty<QQuickTransform> transform();

    qreal x() const;
    qreal y() const;
    QPointF pos() const;
    void setX(qreal);
    void setY(qreal);
    void setPos(const QPointF &);

    qreal width() const;
    void setWidth(qreal);
    void resetWidth();
    qreal implicitWidth() const;

    qreal height() const;
    void setHeight(qreal);
    void resetHeight();
    qreal implicitHeight() const;

    void setSize(const QSizeF &size);

    TransformOrigin transformOrigin() const;
    void setTransformOrigin(TransformOrigin);
    QPointF transformOriginPoint() const;
    void setTransformOriginPoint(const QPointF &);

    qreal z() const;
    void setZ(qreal);

    qreal rotation() const;
    void setRotation(qreal);
    qreal scale() const;
    void setScale(qreal);

    qreal opacity() const;
    void setOpacity(qreal);

    bool isVisible() const;
    void setVisible(bool);

    bool isEnabled() const;
    void setEnabled(bool);

    bool smooth() const;
    void setSmooth(bool);

    Flags flags() const;
    void setFlag(Flag flag, bool enabled = true);
    void setFlags(Flags flags);

    virtual QRectF boundingRect() const;
    virtual QRectF clipRect() const;

    bool hasActiveFocus() const;
    bool hasFocus() const;
    void setFocus(bool);
    bool isFocusScope() const;
    QQuickItem *scopedFocusItem() const;

    Qt::MouseButtons acceptedMouseButtons() const;
    void setAcceptedMouseButtons(Qt::MouseButtons buttons);
    bool acceptHoverEvents() const;
    void setAcceptHoverEvents(bool enabled);

    bool isUnderMouse() const;
    void grabMouse();
    void ungrabMouse();
    bool keepMouseGrab() const;
    void setKeepMouseGrab(bool);
    bool filtersChildMouseEvents() const;
    void setFiltersChildMouseEvents(bool filter);

    void grabTouchPoints(const QList<int> &ids);
    void ungrabTouchPoints();
    bool keepTouchGrab() const;
    void setKeepTouchGrab(bool);

    QTransform itemTransform(QQuickItem *, bool *) const;
    QPointF mapToItem(const QQuickItem *item, const QPointF &point) const;
    QPointF mapToScene(const QPointF &point) const;
    QRectF mapRectToItem(const QQuickItem *item, const QRectF &rect) const;
    QRectF mapRectToScene(const QRectF &rect) const;
    QPointF mapFromItem(const QQuickItem *item, const QPointF &point) const;
    QPointF mapFromScene(const QPointF &point) const;
    QRectF mapRectFromItem(const QQuickItem *item, const QRectF &rect) const;
    QRectF mapRectFromScene(const QRectF &rect) const;

    void polish();

    Q_INVOKABLE void mapFromItem(QQmlV8Function*) const;
    Q_INVOKABLE void mapToItem(QQmlV8Function*) const;
    Q_INVOKABLE void forceActiveFocus();
    Q_INVOKABLE QQuickItem *childAt(qreal x, qreal y) const;

    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;

    struct UpdatePaintNodeData {
       QSGTransformNode *transformNode;
    private:
       friend class QQuickCanvasPrivate;
       UpdatePaintNodeData();
    };

    virtual bool isTextureProvider() const;
    virtual QSGTextureProvider *textureProvider() const;

public Q_SLOTS:
    void update();

Q_SIGNALS:
    void childrenRectChanged(const QRectF &);
    void baselineOffsetChanged(qreal);
    void stateChanged(const QString &);
    void focusChanged(bool);
    void activeFocusChanged(bool);
    void parentChanged(QQuickItem *);
    void transformOriginChanged(TransformOrigin);
    void smoothChanged(bool);
    void clipChanged(bool);

    // XXX todo
    void childrenChanged();
    void opacityChanged();
    void enabledChanged();
    void visibleChanged();
    void visibleChildrenChanged();
    void rotationChanged();
    void scaleChanged();

    void xChanged();
    void yChanged();
    void widthChanged();
    void heightChanged();
    void zChanged();
    void implicitWidthChanged();
    void implicitHeightChanged();

protected:
    virtual bool event(QEvent *);

    bool isComponentComplete() const;
    virtual void itemChange(ItemChange, const ItemChangeData &);

    void updateInputMethod(Qt::InputMethodQueries queries = Qt::ImQueryInput);

    void setImplicitWidth(qreal);
    bool widthValid() const; // ### better name?
    void setImplicitHeight(qreal);
    bool heightValid() const; // ### better name?
    void setImplicitSize(qreal, qreal);

    virtual void classBegin();
    virtual void componentComplete();

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    virtual void inputMethodEvent(QInputMethodEvent *);
    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void mouseUngrabEvent(); // XXX todo - params?
    virtual void touchUngrabEvent();
    virtual void wheelEvent(QWheelEvent *event);
    virtual void touchEvent(QTouchEvent *event);
    virtual void hoverEnterEvent(QHoverEvent *event);
    virtual void hoverMoveEvent(QHoverEvent *event);
    virtual void hoverLeaveEvent(QHoverEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *);
    virtual void dragMoveEvent(QDragMoveEvent *);
    virtual void dragLeaveEvent(QDragLeaveEvent *);
    virtual void dropEvent(QDropEvent *);
    virtual bool childMouseEventFilter(QQuickItem *, QEvent *);
    virtual void windowDeactivateEvent();

    virtual void geometryChanged(const QRectF &newGeometry,
                                 const QRectF &oldGeometry);

    virtual QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);
    virtual void updatePolish();

protected Q_SLOTS:
    void sendAccessibilityUpdate();

protected:
    QQuickItem(QQuickItemPrivate &dd, QQuickItem *parent = 0);

private:
    friend class QQuickCanvas;
    friend class QQuickCanvasPrivate;
    friend class QSGRenderer;
    friend class QAccessibleQuickItem;
    friend class QQuickAccessibleAttached;
    Q_DISABLE_COPY(QQuickItem)
    Q_DECLARE_PRIVATE(QQuickItem)
};

// XXX todo
Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickItem::Flags)

#ifndef QT_NO_DEBUG_STREAM
QDebug Q_QUICK_EXPORT operator<<(QDebug debug, QQuickItem *item);
#endif

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickItem)
QML_DECLARE_TYPE(QQuickTransform)

QT_END_HEADER

#endif // QQUICKITEM_H
