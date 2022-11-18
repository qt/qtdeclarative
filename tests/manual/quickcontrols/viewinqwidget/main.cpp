// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>
#include <QDir>
#include <QGroupBox>
#include <QScreen>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QQuickView>
#include <QQuickWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget widget;
    widget.setWindowTitle(QT_VERSION_STR);

    QHBoxLayout *hLayout = new QHBoxLayout(&widget);
    QGroupBox *groupBox = new QGroupBox("QuickWidget", &widget);
    QVBoxLayout *vLayout = new QVBoxLayout(groupBox);
    QQuickWidget *quickWidget = new QQuickWidget(groupBox);
    quickWidget->setMinimumSize(360, 520);
    vLayout->addWidget(quickWidget);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    quickWidget->setSource(QUrl(QLatin1String("qrc:/main.qml")));
    if (quickWidget->status() == QQuickWidget::Error) {
        qWarning() << quickWidget->errors();
        return 1;
    }
    hLayout->addWidget(groupBox);

    const QString gallerySource =
        QDir::cleanPath(QLatin1String(SRCDIR"../../../examples/quickcontrols/gallery/gallery.qml"));
    QQmlApplicationEngine engine(QUrl::fromLocalFile(gallerySource));
    QObject *root = engine.rootObjects().value(0, nullptr);
    if (!root || !root->isWindowType()) {
        qWarning() << "Load error" << gallerySource;
        return 1;
    }
    groupBox = new QGroupBox("QQuickView/createWindowContainer", &widget);
    vLayout = new QVBoxLayout(groupBox);
    QWidget *container = QWidget::createWindowContainer(qobject_cast<QWindow *>(root), groupBox);
    container->setMinimumSize(360, 520);
    vLayout->addWidget(container);
    hLayout->addWidget(groupBox);

    const QRect availableGeometry = widget.screen()->availableGeometry();
    widget.move(availableGeometry.center() - QPoint(widget.sizeHint().width() / 2, widget.sizeHint().height() / 2));

    widget.show();

    return app.exec();
}
