// Copyright (C) 2021 The Qt Company Ltd.

#ifndef UICALLBACK_H
#define UICALLBACK_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qtreeview.h>
#endif

class UICallback : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    Q_INVOKABLE void showQTreeView(QAbstractItemModel *model)
    {
#ifdef QT_WIDGETS_LIB
        QTreeView *treeViewWidget = new QTreeView(nullptr);
        treeViewWidget->setModel(model);
        treeViewWidget->setExpanded(model->index(0, 0), true);
        treeViewWidget->resize(640, 480);
        treeViewWidget->show();
        treeViewWidget->raise();
#endif
    }
};

#endif // UICALLBACK_H
