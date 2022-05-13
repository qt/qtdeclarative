// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "fbitem.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QLabel>
#include <QQuickItem>

MainWindow::MainWindow(bool transparency, bool noRenderAlpha)
    : m_currentView(nullptr),
      m_currentRootObject(nullptr),
      m_transparent(transparency),
      m_noRenderAlpha(noRenderAlpha)
{
    auto *layout = new QVBoxLayout;

    QGroupBox *groupBox = new QGroupBox(tr("Type"));
    auto *vbox = new QVBoxLayout;
    m_radioView = new QRadioButton(tr("QQuickView in a window container (direct)"));
    m_radioWidget = new QRadioButton(tr("QQuickWidget (indirect through framebuffer objects)"));
    vbox->addWidget(m_radioWidget);
    vbox->addWidget(m_radioView);
    m_radioWidget->setChecked(true);
    m_state = Unknown;
    connect(m_radioWidget, &QRadioButton::toggled, this, &MainWindow::updateView);
    connect(m_radioView, &QRadioButton::toggled, this, &MainWindow::updateView);
    groupBox->setLayout(vbox);

    layout->addWidget(groupBox);

    m_checkboxMultiSample = new QCheckBox(tr("Multisample (4x)"));
    connect(m_checkboxMultiSample, &QCheckBox::toggled, this, &MainWindow::updateView);
    layout->addWidget(m_checkboxMultiSample);

    m_labelStatus = new QLabel;
    layout->addWidget(m_labelStatus);

    QWidget *quickContainer = new QWidget;
    layout->addWidget(quickContainer);
    layout->setStretchFactor(quickContainer, 8);
    m_containerLayout = new QVBoxLayout;
    quickContainer->setLayout(m_containerLayout);

    // Add an overlay widget to demonstrate that it will _not_ work with
    // QQuickView, whereas it is perfectly fine with QQuickWidget.
    QPalette semiTransparent(QColor(255,0,0,128));
    semiTransparent.setBrush(QPalette::Text, Qt::white);
    semiTransparent.setBrush(QPalette::WindowText, Qt::white);

    m_overlayLabel = new QLabel("This is a\nsemi-transparent\n overlay widget\nwhich is placed\non top\n of the Quick\ncontent.", this);
    m_overlayLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_overlayLabel->setAutoFillBackground(true);
    m_overlayLabel->setPalette(semiTransparent);
    QFont f = font();
    f.setPixelSize(QFontInfo(f).pixelSize()*2);
    f.setWeight(QFont::Bold);
    m_overlayLabel->setFont(f);
    m_overlayLabel->hide();

    m_checkboxOverlayVisible = new QCheckBox(tr("Show widget overlay"));
    connect(m_checkboxOverlayVisible, &QCheckBox::toggled, m_overlayLabel, &QWidget::setVisible);
    layout->addWidget(m_checkboxOverlayVisible);

    setLayout(layout);

    updateView();
}

void MainWindow::resizeEvent(QResizeEvent*)
{
    int margin = width() / 10;
    int top = m_checkboxMultiSample->y();
    int bottom = m_checkboxOverlayVisible->geometry().bottom();
    m_overlayLabel->setGeometry(margin, top, width() - 2 * margin, bottom - top);
}

void MainWindow::switchTo(QWidget *view)
{
    if (m_containerLayout->count())
        m_containerLayout->takeAt(0);

    delete m_currentView;
    m_currentView = view;
    m_containerLayout->addWidget(m_currentView);
    m_currentView->setFocus();
}

void MainWindow::updateView()
{
    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    if (m_transparent)
        format.setAlphaBufferSize(8);
    if (m_checkboxMultiSample->isChecked())
        format.setSamples(4);

    State state = m_radioView->isChecked() ? UseWindow : UseWidget;

    if (m_format == format && m_state == state)
        return;

    m_format = format;
    m_state = state;

    QString text = m_currentRootObject
            ? m_currentRootObject->property("currentText").toString()
            : QStringLiteral("Hello Qt");

    QUrl source("qrc:qquickwidgetversuswindow_opengl/test.qml");

    if (m_state == UseWindow) {
        auto *quickView = new QQuickView;
        // m_transparent is not supported here since many systems have problems with semi-transparent child windows
        quickView->setFormat(m_format);
        quickView->setResizeMode(QQuickView::SizeRootObjectToView);
        connect(quickView, &QQuickView::statusChanged, this, &MainWindow::onStatusChangedView);
        connect(quickView, &QQuickView::sceneGraphError, this, &MainWindow::onSceneGraphError);
        quickView->setSource(source);
        m_currentRootObject = quickView->rootObject();
        switchTo(QWidget::createWindowContainer(quickView));
    } else if (m_state == UseWidget) {
        auto *quickWidget = new QQuickWidget;
        if (m_transparent)
            quickWidget->setClearColor(Qt::transparent);
        quickWidget->setFormat(m_format);
        quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
        connect(quickWidget, &QQuickWidget::statusChanged, this, &MainWindow::onStatusChangedWidget);
        connect(quickWidget, &QQuickWidget::sceneGraphError, this, &MainWindow::onSceneGraphError);
        quickWidget->setSource(source);
        switchTo(quickWidget);
        m_currentRootObject = quickWidget->rootObject();
    }

    if (m_currentRootObject) {
        m_currentRootObject->setProperty("currentText", text);
        m_currentRootObject->setProperty("multisample", m_checkboxMultiSample->isChecked());
        if (!m_noRenderAlpha)
            m_currentRootObject->setProperty("translucency", m_transparent);
    }

    m_overlayLabel->raise();
}

void MainWindow::onStatusChangedView(QQuickView::Status status)
{
    QString s;
    switch (status) {
    case QQuickView::Null:
        s = tr("Null");
        break;
    case QQuickView::Ready:
        s = tr("Ready");
        break;
    case QQuickView::Loading:
        s = tr("Loading");
        break;
    case QQuickView::Error:
        s = tr("Error");
        break;
    default:
        s = tr("Unknown");
        break;
    }
    m_labelStatus->setText(tr("QQuickView status: %1").arg(s));
}

void MainWindow::onStatusChangedWidget(QQuickWidget::Status status)
{
    QString s;
    switch (status) {
    case QQuickWidget::Null:
        s = tr("Null");
        break;
    case QQuickWidget::Ready:
        s = tr("Ready");
        break;
    case QQuickWidget::Loading:
        s = tr("Loading");
        break;
    case QQuickWidget::Error:
        s = tr("Error");
        break;
    default:
        s = tr("Unknown");
        break;
    }
    m_labelStatus->setText(tr("QQuickWidget status: %1").arg(s));
}

void MainWindow::onSceneGraphError(QQuickWindow::SceneGraphError error, const QString &message)
{
    m_labelStatus->setText(tr("Scenegraph error %1: %2").arg(error).arg(message));
}
