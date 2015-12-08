/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#ifndef QQUICKPOPUP_P_P_H
#define QQUICKPOPUP_P_P_H

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

#include <QtCore/private/qobject_p.h>
#include <QtQuick/private/qquicktransitionmanager_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickTransition;
class QQuickTransitionManager;
class QQuickPopup;
class QQuickPopupPrivate;
class QQuickOverlay;

class QQuickPopupTransitionManager : public QQuickTransitionManager
{
public:
    QQuickPopupTransitionManager(QQuickPopupPrivate *);
    void transitionShow();
    void transitionHide();

protected:
    void finished() Q_DECL_OVERRIDE;

private:
    enum TransitionState {
        Off, Show, Hide
    };

    TransitionState state;
    QQuickPopupPrivate *pp;
};

class QQuickPopupPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickPopup)

public:
    QQuickPopupPrivate();
    ~QQuickPopupPrivate();

    void finalizeShowTransition();
    void finalizeHideTransition();

    QQuickItem *contentItem;
    QQuickOverlay *overlay;
    bool focus;
    bool modal;
    QQuickTransition *showTransition;
    QQuickTransition *hideTransition;
    QQuickPopupTransitionManager transitionManager;
};

QT_END_NAMESPACE

#endif // QQUICKPOPUP_P_P_H

