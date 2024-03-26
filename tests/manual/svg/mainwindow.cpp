// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "svgmanager.h"
#include "svgpainter.h"

#include <QFileDialog>
#include <QSettings>
#include <QQuickWidget>
#include <QQmlEngine>
#include <QSlider>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->tbSelectDir, &QToolButton::clicked, this, &MainWindow::selectDirectory);
    connect(ui->tbNext, &QToolButton::clicked, this, &MainWindow::next);
    connect(ui->tbPrev, &QToolButton::clicked, this, &MainWindow::previous);
    m_manager = new SvgManager(this);

    m_imageLabel = new QLabel;
    m_imageLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ui->saImage->setWidget(m_imageLabel);
    ui->saImage->setBackgroundRole(QPalette::Base);

    m_svgPainter = new SvgPainter;
    ui->saSvgPainter->setWidget(m_svgPainter);
    ui->saSvgPainter->setBackgroundRole(QPalette::Base);

    m_svgImageWidget = new QQuickWidget;
    m_svgImageWidget->setSource(QUrl(QStringLiteral("qrc:/SvgImageTest/SvgImage.qml")));
    m_svgImageWidget->setResizeMode(QQuickWidget::SizeViewToRootObject);
    m_svgImageWidget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    ui->saSvgImage->setWidget(m_svgImageWidget);

    m_qmlGeneratorWidget = new QQuickWidget;
    m_qmlGeneratorWidget->setSource(QUrl(QStringLiteral("qrc:/SvgImageTest/QmlGenerator.qml")));
    m_qmlGeneratorWidget->setResizeMode(QQuickWidget::SizeViewToRootObject);
    m_qmlGeneratorWidget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    ui->saQmlGenerator->setWidget(m_qmlGeneratorWidget);

    connect(m_manager, &SvgManager::currentSourceChanged, this, &MainWindow::updateSources);

    m_settings = new QSettings(QStringLiteral("org.qtproject"), QStringLiteral("svg-test"), this);

    connect(ui->cbCurrentDir, &QComboBox::currentTextChanged, this, &MainWindow::loadDirectory);
    QStringList list = m_settings->value(QStringLiteral("directories")).toString().split(QLatin1Char(','));
    setDirList(list);

    connect(ui->hsScale, &QAbstractSlider::valueChanged, m_manager, &SvgManager::setScale);
    connect(ui->hsScale, &QAbstractSlider::valueChanged, m_svgPainter, &SvgPainter::setScale);
    connect(ui->hsScale, &QAbstractSlider::valueChanged, this, &MainWindow::setScale);
    int scale = m_settings->value(QStringLiteral("scale"), 10).toInt();
    ui->hsScale->setValue(scale);
}

void MainWindow::loadDirectory(const QString &newDir)
{
    m_manager->setCurrentDirectory(newDir);
}

void MainWindow::setDirList(const QStringList &list)
{
    ui->cbCurrentDir->clear();
    for (QString dirName : list)
        ui->cbCurrentDir->addItem(dirName);

    m_settings->setValue(QStringLiteral("directories"), list.join(QLatin1Char(',')));
}

void MainWindow::setScale(const int scale)
{
    m_settings->setValue(QStringLiteral("scale"), scale);
}

void MainWindow::updateCurrentDir(const QString &s)
{
    QStringList list;
    for (int i = 0; i < ui->cbCurrentDir->count(); ++i) {
        if (ui->cbCurrentDir->itemText(i) != s)
            list.append(ui->cbCurrentDir->itemText(i));
    }

    list.prepend(s);

    while (list.size() > 10)
        list.removeLast();

    setDirList(list);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateSources()
{
    m_svgPainter->setSource(m_manager->currentSource());

    QFileInfo info(m_manager->currentSource().toLocalFile());
    ui->lCurrentFileName->setText(info.baseName());

    QFileInfo pngInfo(info.absolutePath() + QLatin1Char('/') + info.baseName() + QStringLiteral(".png"));
    if (pngInfo.exists()) {
        ui->lRefPng->setVisible(true);
        ui->saImage->setVisible(true);
        QPixmap pixmap = QPixmap::fromImage(QImage(pngInfo.absoluteFilePath()));
        m_imageLabel->setPixmap(pixmap);
    } else {
        ui->lRefPng->setVisible(false);
        ui->saImage->setVisible(false);
        m_imageLabel->clear();
    }
}

void MainWindow::selectDirectory()
{
    QString s = QFileDialog::getExistingDirectory(this, tr("Select directory"), m_manager->currentDirectory());
    if (!s.isEmpty()) {
        updateCurrentDir(s);
    }
}

void MainWindow::next()
{
    if (m_manager->currentIndex() + 1 < m_manager->sourceCount())
        m_manager->setCurrentIndex(m_manager->currentIndex() + 1);
}

void MainWindow::previous()
{
    if (m_manager->currentIndex() > 0)
        m_manager->setCurrentIndex(m_manager->currentIndex() - 1);

}
