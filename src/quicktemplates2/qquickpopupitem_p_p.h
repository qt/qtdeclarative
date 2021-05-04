/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKPOPUPITEM_P_P_H
#define QQUICKPOPUPITEM_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuickTemplates2/private/qquickpopupitem_p.h>

QT_BEGIN_NAMESPACE

class QQuickPopup;

class QQuickPopupItemPrivate : public QQuickPagePrivate
{
    Q_DECLARE_PUBLIC(QQuickPopupItem)

public:
    QQuickPopupItemPrivate(QQuickPopup *popup);

    void init();

    void implicitWidthChanged() override;
    void implicitHeightChanged() override;

    void resolveFont() override;

    QQuickItem *getContentItem() override;

    void cancelContentItem() override;
    void executeContentItem(bool complete = false) override;

    void cancelBackground() override;
    void executeBackground(bool complete = false) override;

    QQuickPalette *palette() const override;
    void setPalette(QQuickPalette* p) override;
    void resetPalette() override;

    QPalette defaultPalette() const override;
    bool providesPalette() const override;

    QPalette parentPalette() const override;

    int backId = 0;
    int escapeId = 0;
    QQuickPopup *popup = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKPOPUPITEM_P_P_H
