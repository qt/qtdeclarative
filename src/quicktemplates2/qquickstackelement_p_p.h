/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QQUICKSTACKELEMENT_P_P_H
#define QQUICKSTACKELEMENT_P_P_H

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
struct QQuickStackTransition;
class RequiredProperties;

class QQuickStackElement : public QQuickItemViewTransitionableItem, public QQuickItemChangeListener
{
    QQuickStackElement();

public:
    ~QQuickStackElement();

    static QQuickStackElement *fromString(const QString &str, QQuickStackView *view, QString *error);
    static QQuickStackElement *fromObject(QObject *object, QQuickStackView *view, QString *error);

    bool load(QQuickStackView *parent);
    void incubate(QObject *object, RequiredProperties &requiredProperties);
    void initialize(RequiredProperties &requiredProperties);

    void setIndex(int index);
    void setView(QQuickStackView *view);
    void setStatus(QQuickStackView::Status status);
    void setVisible(bool visible);

    void transitionNextReposition(QQuickItemViewTransitioner *transitioner, QQuickItemViewTransitioner::TransitionType type, bool asTarget);
    bool prepareTransition(QQuickItemViewTransitioner *transitioner, const QRectF &viewBounds);
    void startTransition(QQuickItemViewTransitioner *transitioner, QQuickStackView::Status status);
    void completeTransition(QQuickTransition *quickTransition);

    void itemDestroyed(QQuickItem *item) override;

    int index = -1;
    bool init = false;
    bool removal = false;
    bool ownItem = false;
    bool ownComponent = false;
    bool widthValid = false;
    bool heightValid = false;
    QQmlContext *context = nullptr;
    QQmlComponent *component = nullptr;
    QQuickStackView *view = nullptr;
    QPointer<QQuickItem> originalParent;
    QQuickStackView::Status status = QQuickStackView::Inactive;
    QV4::PersistentValue properties;
    QV4::PersistentValue qmlCallingContext;
};

QT_END_NAMESPACE

#endif // QQUICKSTACKELEMENT_P_P_H
