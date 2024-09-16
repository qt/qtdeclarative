// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "previewwidget.h"
#include "../states/statecontroller.h"

#include <QListView>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QSplitter>
#include <QVBoxLayout>

PreviewWidget::PreviewWidget(QWidget *parent)
    : QWidget{parent}
    , m_errorListModel{this}
    , m_errorListView{new QListView}
    , m_quickWidget{new QQuickWidget}
{
    initUI();
    setupConnections();
}

QString PreviewWidget::sourcePath() const
{
    return m_quickWidget->source().toString();
}

void PreviewWidget::setSourcePath(const QString &path)
{
    m_quickWidget->engine()->clearComponentCache();
    m_quickWidget->setSource(QUrl::fromLocalFile(path));
}

void PreviewWidget::initUI()
{
    m_errorListView->setModel(&m_errorListModel);
    m_errorListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_errorListView->setWordWrap(true);
    m_errorListView->setSpacing(4);

    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    QSizePolicy sizePolicy = m_quickWidget->sizePolicy();
    sizePolicy.setVerticalStretch(1);
    m_quickWidget->setSizePolicy(sizePolicy);

    QSplitter *splitter = new QSplitter{this};
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(m_quickWidget);
    splitter->addWidget(m_errorListView);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);
    setLayout(layout);
}

void PreviewWidget::setupConnections()
{
    connect(StateController::instance(), &StateController::stateChanged, this,
            &PreviewWidget::onAppStateChanged);
    connect(m_quickWidget, &QQuickWidget::statusChanged, this,
            &PreviewWidget::onQuickWidetStatusChanged);
    connect(m_errorListView, &QAbstractItemView::doubleClicked, &m_errorListModel,
            &ErrorListModel::selectIndex);
    connect(&m_errorListModel, &ErrorListModel::errorPositionSelected, this,
            &PreviewWidget::errorPositionSelected);
}

void PreviewWidget::onAppStateChanged(int oldState, int newState)
{
    Q_UNUSED(oldState);

    switch (newState) {
    case StateController::InitState:
        m_errorListModel.setErrorList({});
        break;

    default:
        break;
    }
}

void PreviewWidget::onQuickWidetStatusChanged(int status)
{
    switch (status) {
    case QQuickWidget::Error:
        m_errorListModel.setErrorList(m_quickWidget->errors());
        break;
    default:
        m_errorListModel.setErrorList({});
        break;
    }
}
