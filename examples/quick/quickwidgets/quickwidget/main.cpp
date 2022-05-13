// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QCommandLineParser>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlError>
#include <QMdiArea>
#include <QLCDNumber>
#include <QFileDialog>
#include <QStatusBar>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();

private slots:
    void quickWidgetStatusChanged(QQuickWidget::Status);
    void sceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);
    void grabFramebuffer();
    void renderToPixmap();
    void grabToImage();
    void createQuickWidgetsInTabs(QMdiArea *mdiArea);

private:
    QQuickWidget *m_quickWidget;
};

static bool optMultipleSample = false;

MainWindow::MainWindow()
   : m_quickWidget(new QQuickWidget)
{
    QSurfaceFormat format;
    if (optMultipleSample)
        format.setSamples(4);
    m_quickWidget->setFormat(format);

    QMdiArea *centralWidget = new QMdiArea;

    QLCDNumber *lcd = new QLCDNumber;
    lcd->display(1337);
    lcd->setMinimumSize(250,100);
    centralWidget->addSubWindow(lcd);

    QUrl source("qrc:quickwidget/rotatingsquare.qml");

    connect(m_quickWidget, &QQuickWidget::statusChanged,
            this, &MainWindow::quickWidgetStatusChanged);
    connect(m_quickWidget, &QQuickWidget::sceneGraphError,
            this, &MainWindow::sceneGraphError);
    m_quickWidget->resize(300,300);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView );
    m_quickWidget->setSource(source);

    centralWidget->addSubWindow(m_quickWidget);

    setCentralWidget(centralWidget);

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("Grab framebuffer"), this, &MainWindow::grabFramebuffer);
    fileMenu->addAction(tr("Render to pixmap"), this, &MainWindow::renderToPixmap);
    fileMenu->addAction(tr("Grab via grabToImage"), this, &MainWindow::grabToImage);
    fileMenu->addAction(tr("Quit"), qApp, &QCoreApplication::quit);

    QMenu *windowMenu = menuBar()->addMenu(tr("&Window"));
    windowMenu->addAction(tr("Add tab widget"), this,
                          [this, centralWidget] { createQuickWidgetsInTabs(centralWidget); });
}

void MainWindow::createQuickWidgetsInTabs(QMdiArea *mdiArea)
{
    // A QQuickWidget should work like any other widget when it comes to being
    // in layouts, in tab widgets, MDI areas, etc. It can also be freely
    // reparented and made top-level.

    QTabWidget *tabWidget = new QTabWidget;
    const QSize size(400, 400);
    const QString msgToTopLevel = QLatin1String("Break out to top-level window");
    const QString msgFromTopLevel = QLatin1String("Move back under tab widget");

    static const int N = 4;
    static const QColor colorTab[N] = { Qt::green, Qt::blue, Qt::yellow, Qt::magenta };
    for (int i = 0; i < N; ++i) {
        QQuickWidget *widget = new QQuickWidget;
        widget->resize(size);
        widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
        QObject::connect(widget, &QQuickWidget::statusChanged, widget, [widget, i] {
            if (widget->status() == QQuickWidget::Ready) {
                if (QQuickItem *rootItem = widget->rootObject()) {
                    rootItem->setProperty("rectColor", colorTab[i]);
                }
            }
        });
        widget->setSource(QUrl("qrc:quickwidget/rotatingsquare.qml"));
        widget->setWindowTitle(QString::asprintf("Tab %d", i + 1));
        QPushButton *btn = new QPushButton(msgToTopLevel, widget);
        connect(btn, &QPushButton::clicked, widget, [=] {
            if (widget->parent()) {
                widget->setAttribute(Qt::WA_DeleteOnClose, true);
                widget->setParent(nullptr);
                widget->show();
                btn->setText(msgFromTopLevel);
            } else {
                widget->setAttribute(Qt::WA_DeleteOnClose, false);
                tabWidget->addTab(widget, widget->windowTitle());
                btn->setText(msgToTopLevel);
            }
        });
        tabWidget->addTab(widget, QString::asprintf("Tab %d", i + 1));
    }

    mdiArea->addSubWindow(tabWidget);
    tabWidget->show();
}

void MainWindow::quickWidgetStatusChanged(QQuickWidget::Status status)
{
    if (status == QQuickWidget::Error) {
        QStringList errors;
        const auto widgetErrors = m_quickWidget->errors();
        for (const QQmlError &error : widgetErrors)
            errors.append(error.toString());
        statusBar()->showMessage(errors.join(QStringLiteral(", ")));
    }
}

void MainWindow::sceneGraphError(QQuickWindow::SceneGraphError, const QString &message)
{
     statusBar()->showMessage(message);
}

template<class T> void saveToFile(QWidget *parent, T *saveable)
{
    QFileDialog fd(parent);
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setDefaultSuffix("png");
    fd.selectFile("test.png");
    if (fd.exec() == QDialog::Accepted)
        saveable->save(fd.selectedFiles().first());
}

void MainWindow::grabFramebuffer()
{
    QImage image = m_quickWidget->grabFramebuffer();
    saveToFile(this, &image);
}

void MainWindow::renderToPixmap()
{
    QPixmap pixmap(m_quickWidget->size());
    m_quickWidget->render(&pixmap);
    saveToFile(this, &pixmap);
}

void MainWindow::grabToImage()
{
    QFileDialog fd(this);
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setDefaultSuffix("png");
    fd.selectFile("test_grabToImage.png");
    if (fd.exec() == QDialog::Accepted) {
        QMetaObject::invokeMethod(m_quickWidget->rootObject(), "performLayerBasedGrab",
                                  Q_ARG(QVariant, fd.selectedFiles().first()));
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("Qt QQuickWidget Example");
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption multipleSampleOption("multisample", "Multisampling");
    parser.addOption(multipleSampleOption);

    parser.process(app);

    optMultipleSample = parser.isSet(multipleSampleOption);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}

#include "main.moc"
