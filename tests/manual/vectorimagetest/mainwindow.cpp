// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "vectorimagemanager.h"
#include "svgpainter.h"

#include <QFileDialog>
#include <QSettings>
#include <QQuickWidget>
#include <QQmlEngine>
#include <QSlider>
#include <QCheckBox>

#include <QQuickView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->tbSelectDir, &QToolButton::clicked, this, &MainWindow::selectDirectory);
    connect(ui->tbNext, &QToolButton::clicked, this, &MainWindow::next);
    connect(ui->tbPrev, &QToolButton::clicked, this, &MainWindow::previous);
    m_manager = new VectorImageManager(this);

    m_imageLabel = new QLabel;
    m_imageLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ui->saImage->setWidget(m_imageLabel);
    ui->saImage->setBackgroundRole(QPalette::Base);

    m_svgPainter = new SvgPainter;
    ui->saSvgPainter->setWidget(m_svgPainter);
    ui->saSvgPainter->setBackgroundRole(QPalette::Base);

    m_lottieAnimationWidget = new QQuickWidget;
    m_lottieAnimationWidget->setSource(QUrl(QStringLiteral("qrc:/qt/qml/VectorImageTest/LottieAnimation.qml")));
    m_lottieAnimationWidget->setResizeMode(QQuickWidget::SizeViewToRootObject);
    m_lottieAnimationWidget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    ui->saLottieAnimation->setWidget(m_lottieAnimationWidget);

    m_vectorImageView = new QQuickView(windowHandle());
    connect(m_vectorImageView, &QWindow::widthChanged, this, &MainWindow::vectorImageSizeUpdated);
    connect(m_vectorImageView, &QWindow::heightChanged, this, &MainWindow::vectorImageSizeUpdated);
    m_vectorImageView->setSource(QUrl(QStringLiteral("qrc:/qt/qml/VectorImageTest/VectorImage.qml")));
    m_vectorImageView->setResizeMode(QQuickView::SizeViewToRootObject);

    m_vectorImageWidget = QWidget::createWindowContainer(m_vectorImageView);
    m_vectorImageWidget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    ui->saVectorImage->setWidget(m_vectorImageWidget);

    connect(m_manager, &VectorImageManager::currentSourceChanged, this, &MainWindow::updateSource);
    connect(m_manager, &VectorImageManager::sourcesChanged, this, &MainWindow::updateSources);
    connect(ui->cbFilename, &QComboBox::currentIndexChanged, this, &MainWindow::updateIndex);

    m_settings = new QSettings(QStringLiteral("org.qtproject"), QStringLiteral("svg-test"), this);

    connect(ui->cbCurrentDir, &QComboBox::currentTextChanged, this, &MainWindow::loadDirectory);
    QStringList list = m_settings->value(QStringLiteral("directories")).toString().split(QLatin1Char(','));
    setDirList(list);

    connect(ui->hsScale, &QAbstractSlider::valueChanged, m_manager, &VectorImageManager::setScale);
    connect(ui->hsScale, &QAbstractSlider::valueChanged, m_svgPainter, &SvgPainter::setScale);
    connect(ui->hsScale, &QAbstractSlider::valueChanged, this, &MainWindow::setScale);

    connect(ui->cbLooping, &QCheckBox::toggled, m_manager, &VectorImageManager::setLooping);
    connect(ui->cbLooping, &QCheckBox::toggled, this, &MainWindow::setLooping);
    connect(ui->cbLooping, &QCheckBox::toggled, m_svgPainter, &SvgPainter::setLooping);

    int scale = m_settings->value(QStringLiteral("scale"), 10).toInt();
    ui->hsScale->setValue(scale);

    ui->cbLooping->setChecked(m_settings->value(QStringLiteral("looping")).toBool());

    ui->tbNext->setShortcut(QKeySequence(QKeySequence::MoveToNextChar));
    ui->tbPrev->setShortcut(QKeySequence(QKeySequence::MoveToPreviousChar));
}

void MainWindow::vectorImageSizeUpdated()
{
    if (m_vectorImageView && m_vectorImageWidget) {
        m_vectorImageWidget->setMinimumSize(m_vectorImageView->size());
    }
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

void MainWindow::updateIndex(int newIndex)
{
    if (m_manager->currentIndex() == newIndex)
        return;


    m_manager->setCurrentIndex(newIndex);
}

void MainWindow::updateSources()
{
    ui->cbFilename->clear();

    QList<QUrl> sources = m_manager->sources();
    for (qsizetype i = 0; i < sources.size(); ++i) {
        QFileInfo info(sources.at(i).toLocalFile());
        ui->cbFilename->addItem(info.baseName());
    }
}

void MainWindow::updateSource()
{
    m_svgPainter->setSource(m_manager->currentSource());

    QFileInfo info(m_manager->currentSource().toLocalFile());

    int index = ui->cbFilename->findText(info.baseName());
    if (index < 0)
        return;
    ui->cbFilename->setCurrentIndex(index);

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

    bool isLottie = info.suffix().toLower() == QStringLiteral("json");
    ui->saSvgPainter->setVisible(!isLottie);
    ui->lSvgRenderer->setVisible(!isLottie);
    ui->saLottieAnimation->setVisible(isLottie);
    ui->lLottieAnimation->setVisible(isLottie);
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

void MainWindow::setLooping(bool looping)
{
    m_settings->setValue(QStringLiteral("looping"), looping);
}
