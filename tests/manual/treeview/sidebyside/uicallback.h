/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of TreeView.
**
** $QT_BEGIN_LICENSE:GPL-MARKETPLACE-QT$
**
** Marketplace License Usage
** Users, who have licensed the Software under the Qt Marketplace license
** agreement, may use this file in accordance with the Qt Marketplace license
** agreement provided with the Software or, alternatively, in accordance with
** the terms contained in a written agreement between the licensee and The Qt
** Company. For licensing terms and conditions see
** https://www.qt.io/terms-conditions/#marketplace and
** https://www.qt.io/terms-conditions. For further information use the contact
** form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
