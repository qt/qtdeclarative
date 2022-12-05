// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstacktransition_p_p.h"
#include "qquickstackelement_p_p.h"
#include "qquickstackview_p_p.h"

QT_BEGIN_NAMESPACE

static QQuickStackTransition exitTransition(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view)
{
    QQuickStackTransition st;
    st.status = QQuickStackView::Deactivating;
    st.element = element;

    const QQuickItemViewTransitioner *transitioner = QQuickStackViewPrivate::get(view)->transitioner;

    switch (operation) {
    case QQuickStackView::PushTransition:
        st.type = QQuickItemViewTransitioner::AddTransition;
        if (transitioner)
            st.transition = transitioner->addDisplacedTransition;
        break;
    case QQuickStackView::ReplaceTransition:
        st.type = QQuickItemViewTransitioner::MoveTransition;
        if (transitioner)
            st.transition = transitioner->moveDisplacedTransition;
        break;
    case QQuickStackView::PopTransition:
        st.target = true;
        st.type = QQuickItemViewTransitioner::RemoveTransition;
        st.viewBounds = view->boundingRect();
        if (transitioner)
            st.transition = transitioner->removeTransition;
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    return st;
}

static QQuickStackTransition enterTransition(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view)
{
    QQuickStackTransition st;
    st.status = QQuickStackView::Activating;
    st.element = element;

    const QQuickItemViewTransitioner *transitioner = QQuickStackViewPrivate::get(view)->transitioner;

    switch (operation) {
    case QQuickStackView::PushTransition:
        st.target = true;
        st.type = QQuickItemViewTransitioner::AddTransition;
        st.viewBounds = view->boundingRect();
        if (transitioner)
            st.transition = transitioner->addTransition;
        break;
    case QQuickStackView::ReplaceTransition:
        st.target = true;
        st.type = QQuickItemViewTransitioner::MoveTransition;
        st.viewBounds = view->boundingRect();
        if (transitioner)
            st.transition = transitioner->moveTransition;
        break;
    case QQuickStackView::PopTransition:
        st.type = QQuickItemViewTransitioner::RemoveTransition;
        if (transitioner)
            st.transition = transitioner->removeDisplacedTransition;
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    return st;
}

static QQuickStackView::Operation operationTransition(QQuickStackView::Operation operation, QQuickStackView::Operation transition)
{
    if (operation == QQuickStackView::Immediate || operation == QQuickStackView::Transition)
        return transition;
    return operation;
}

QQuickStackTransition QQuickStackTransition::popExit(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view)
{
    return exitTransition(operationTransition(operation, QQuickStackView::PopTransition), element, view);
}

QQuickStackTransition QQuickStackTransition::popEnter(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view)
{
    return enterTransition(operationTransition(operation, QQuickStackView::PopTransition), element, view);
}

QQuickStackTransition QQuickStackTransition::pushExit(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view)
{
    return exitTransition(operationTransition(operation, QQuickStackView::PushTransition), element, view);
}

QQuickStackTransition QQuickStackTransition::pushEnter(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view)
{
    return enterTransition(operationTransition(operation, QQuickStackView::PushTransition), element, view);
}

QQuickStackTransition QQuickStackTransition::replaceExit(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view)
{
    return exitTransition(operationTransition(operation, QQuickStackView::ReplaceTransition), element, view);
}

QQuickStackTransition QQuickStackTransition::replaceEnter(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view)
{
    return enterTransition(operationTransition(operation, QQuickStackView::ReplaceTransition), element, view);
}

QT_END_NAMESPACE
