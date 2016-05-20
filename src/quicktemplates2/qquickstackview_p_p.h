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

#ifndef QQUICKSTACKVIEW_P_P_H
#define QQUICKSTACKVIEW_P_P_H

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

#include <QtQuickTemplates2/private/qquickstackview_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <QtQuick/private/qquickitemviewtransition_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQml/private/qv4persistent_p.h>

QT_BEGIN_NAMESPACE

class QQmlContext;
class QQmlComponent;
class QQmlIncubator;

class QQuickStackElement : public QQuickItemViewTransitionableItem, public QQuickItemChangeListener
{
    QQuickStackElement();

public:
    ~QQuickStackElement();

    static QQuickStackElement *fromString(const QString &str, QQuickStackView *view);
    static QQuickStackElement *fromObject(QObject *object, QQuickStackView *view);

    bool load(QQuickStackView *parent);
    void incubate(QObject *object);
    void initialize();

    void setIndex(int index);
    void setView(QQuickStackView *view);
    void setStatus(QQuickStackView::Status status);

    void transitionNextReposition(QQuickItemViewTransitioner *transitioner, QQuickItemViewTransitioner::TransitionType type, bool asTarget);
    bool prepareTransition(QQuickItemViewTransitioner *transitioner, const QRectF &viewBounds);
    void startTransition(QQuickItemViewTransitioner *transitioner);

    void itemDestroyed(QQuickItem *item) override;

    int index;
    bool init;
    bool removal;
    bool ownItem;
    bool ownComponent;
    bool widthValid;
    bool heightValid;
    QQmlContext *context;
    QQmlComponent *component;
    QQmlIncubator *incubator;
    QQuickStackView *view;
    QPointer<QQuickItem> originalParent;
    QQuickStackView::Status status;
    QV4::PersistentValue properties;
    QV4::PersistentValue qmlCallingContext;
};

class QQuickStackViewPrivate : public QQuickControlPrivate, public QQuickItemViewTransitionChangeListener
{
    Q_DECLARE_PUBLIC(QQuickStackView)

public:
    QQuickStackViewPrivate();

    static QQuickStackViewPrivate *get(QQuickStackView *view)
    {
        return view->d_func();
    }

    void setCurrentItem(QQuickItem *item);

    QList<QQuickStackElement *> parseElements(QQmlV4Function *args, int from = 0);
    QQuickStackElement *findElement(QQuickItem *item) const;
    QQuickStackElement *findElement(const QV4::Value &value) const;
    QQuickStackElement *createElement(const QV4::Value &value);
    bool pushElements(const QList<QQuickStackElement *> &elements);
    bool pushElement(QQuickStackElement *element);
    bool popElements(QQuickStackElement *element);
    bool replaceElements(QQuickStackElement *element, const QList<QQuickStackElement *> &elements);

    void ensureTransitioner();
    void popTransition(QQuickStackElement *enter, QQuickStackElement *exit, const QRectF &viewBounds, bool immediate);
    void pushTransition(QQuickStackElement *enter, QQuickStackElement *exit, const QRectF &viewBounds, bool immediate);
    void replaceTransition(QQuickStackElement *enter, QQuickStackElement *exit, const QRectF &viewBounds, bool immediate);
    void completeTransition(QQuickStackElement *element, QQuickTransition *transition);

    void viewItemTransitionFinished(QQuickItemViewTransitionableItem *item) override;
    void setBusy(bool busy);

    bool busy;
    QVariant initialItem;
    QQuickItem *currentItem;
    QList<QQuickStackElement*> removals;
    QStack<QQuickStackElement *> elements;
    QQuickItemViewTransitioner *transitioner;
};

class QQuickStackAttachedPrivate : public QObjectPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickStackAttached)

public:
    QQuickStackAttachedPrivate() : element(nullptr) { }

    static QQuickStackAttachedPrivate *get(QQuickStackAttached *attached)
    {
        return attached->d_func();
    }

    void itemParentChanged(QQuickItem *item, QQuickItem *parent);

    QQuickStackElement *element;
};

QT_END_NAMESPACE

#endif // QQUICKSTACKVIEW_P_P_H
