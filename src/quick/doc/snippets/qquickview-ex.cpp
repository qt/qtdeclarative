// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQuickView *view = new QQuickView;
    view->setSource(QUrl::fromLocalFile("myqmlfile.qml"));
    view->show();
    return app.exec();
}
//![0]

void makeDocTeamHappyByKeepingExampleCompilable() {
//![1]
    QScopedPointer<QQuickView> view { new QQuickView };
    view->setInitialProperties({"x, 100"}, {"width", 50});
    view->setSource(QUrl::fromLocalFile("myqmlfile.qml"));
    view->show();
//![1]
}
