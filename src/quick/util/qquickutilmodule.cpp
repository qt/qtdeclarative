/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickutilmodule_p.h"
#include "qquickanimation_p.h"
#include "qquickanimation_p_p.h"
#include "qquickapplication_p.h"
#include "qquickbehavior_p.h"
#include "qquicksmoothedanimation_p.h"
#include "qquickfontloader_p.h"
#include "qquickfontmetrics_p.h"
#include "qquickpropertychanges_p.h"
#include "qquickspringanimation_p.h"
#include "qquickstategroup_p.h"
#include "qquickstatechangescript_p.h"
#include "qquickstate_p.h"
#include "qquickstate_p_p.h"
#include "qquicksystempalette_p.h"
#include "qquicktextmetrics_p.h"
#include "qquicktransition_p.h"
#include "qquickanimator_p.h"
#if QT_CONFIG(shortcut)
#include "qquickshortcut_p.h"
#endif
#include "qquickvalidator_p.h"
#include "qquickforeignutils_p.h"
#include <qqmlinfo.h>
#include <private/qqmltypenotavailable_p.h>
#include <private/qquickanimationcontroller_p.h>
#include <QtCore/qcoreapplication.h>

#if QT_CONFIG(shortcut)
Q_DECLARE_METATYPE(QKeySequence::StandardKey)
#endif

void QQuickUtilModule::defineModule()
{
#if QT_CONFIG(shortcut)
    qRegisterMetaType<QKeySequence::StandardKey>();
#endif

    qmlRegisterTypesAndRevisions<
#if QT_CONFIG(validator)
            QValidatorForeign,
            QQuickIntValidator,
            QQuickDoubleValidator,
            QRegExpValidatorForeign,
#if QT_CONFIG(regularexpression)
            QRegularExpressionValidatorForeign,
#endif // QT_CONFIG(regularexpression)
#endif // QT_CONFIG(validator)
#if QT_CONFIG(quick_shadereffect) && QT_CONFIG(opengl)
            QQuickUniformAnimator,
#endif
#if QT_CONFIG(shortcut)
            QQuickShortcut,
            QKeySequenceForeign,
#endif
#if QT_CONFIG(im)
            QInputMethodForeign,
#endif
            QQuickAbstractAnimation,
            QQuickBehavior,
            QQuickColorAnimation,
            QQuickSmoothedAnimation,
            QQuickFontLoader,
            QQuickNumberAnimation,
            QQuickParallelAnimation,
            QQuickPauseAnimation,
            QQuickPropertyAction,
            QQuickPropertyAnimation,
            QQuickRotationAnimation,
            QQuickScriptAction,
            QQuickSequentialAnimation,
            QQuickSpringAnimation,
            QQuickAnimationController,
            QQuickStateChangeScript,
            QQuickStateGroup,
            QQuickState,
            QQuickSystemPalette,
            QQuickTransition,
            QQuickVector3dAnimation,
            QQuickAnimator,
            QQuickXAnimator,
            QQuickYAnimator,
            QQuickScaleAnimator,
            QQuickRotationAnimator,
            QQuickOpacityAnimator,
            QQuickStateOperation,
            QQuickPropertyChanges,
            QQuickFontMetrics,
            QQuickTextMetrics,
            QQuickApplication
    >("QtQuick", 2);
}
