/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLBUILTINFUNCTIONS_P_H
#define QQMLBUILTINFUNCTIONS_P_H

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

#include <QtCore/qglobal.h>
#include <private/qv4object_p.h>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QV8Engine;

namespace QV4 {

struct QtObject : Object
{
    Q_MANAGED
    QtObject(ExecutionEngine *v4, QQmlEngine *qmlEngine);

    static ReturnedValue method_isQtObject(SimpleCallContext *ctx);
    static ReturnedValue method_rgba(SimpleCallContext *ctx);
    static ReturnedValue method_hsla(SimpleCallContext *ctx);
    static ReturnedValue method_colorEqual(SimpleCallContext *ctx);
    static ReturnedValue method_font(SimpleCallContext *ctx);
    static ReturnedValue method_rect(SimpleCallContext *ctx);
    static ReturnedValue method_point(SimpleCallContext *ctx);
    static ReturnedValue method_size(SimpleCallContext *ctx);
    static ReturnedValue method_vector2d(SimpleCallContext *ctx);
    static ReturnedValue method_vector3d(SimpleCallContext *ctx);
    static ReturnedValue method_vector4d(SimpleCallContext *ctx);
    static ReturnedValue method_quaternion(SimpleCallContext *ctx);
    static ReturnedValue method_matrix4x4(SimpleCallContext *ctx);
    static ReturnedValue method_lighter(SimpleCallContext *ctx);
    static ReturnedValue method_darker(SimpleCallContext *ctx);
    static ReturnedValue method_tint(SimpleCallContext *ctx);
    static ReturnedValue method_formatDate(SimpleCallContext *ctx);
    static ReturnedValue method_formatTime(SimpleCallContext *ctx);
    static ReturnedValue method_formatDateTime(SimpleCallContext *ctx);
    static ReturnedValue method_openUrlExternally(SimpleCallContext *ctx);
    static ReturnedValue method_fontFamilies(SimpleCallContext *ctx);
    static ReturnedValue method_md5(SimpleCallContext *ctx);
    static ReturnedValue method_btoa(SimpleCallContext *ctx);
    static ReturnedValue method_atob(SimpleCallContext *ctx);
    static ReturnedValue method_quit(SimpleCallContext *ctx);
    static ReturnedValue method_resolvedUrl(SimpleCallContext *ctx);
    static ReturnedValue method_createQmlObject(SimpleCallContext *ctx);
    static ReturnedValue method_createComponent(SimpleCallContext *ctx);
    static ReturnedValue method_locale(SimpleCallContext *ctx);
    static ReturnedValue method_binding(SimpleCallContext *ctx);

    static ReturnedValue method_get_platform(SimpleCallContext *ctx);
    static ReturnedValue method_get_application(SimpleCallContext *ctx);
#ifndef QT_NO_IM
    static ReturnedValue method_get_inputMethod(SimpleCallContext *ctx);
#endif

    QObject *m_platform;
    QObject *m_application;
};

struct ConsoleObject : Object
{
    ConsoleObject(ExecutionEngine *v4);

    static ReturnedValue method_error(SimpleCallContext *ctx);
    static ReturnedValue method_log(SimpleCallContext *ctx);
    static ReturnedValue method_profile(SimpleCallContext *ctx);
    static ReturnedValue method_profileEnd(SimpleCallContext *ctx);
    static ReturnedValue method_time(SimpleCallContext *ctx);
    static ReturnedValue method_timeEnd(SimpleCallContext *ctx);
    static ReturnedValue method_count(SimpleCallContext *ctx);
    static ReturnedValue method_trace(SimpleCallContext *ctx);
    static ReturnedValue method_warn(SimpleCallContext *ctx);
    static ReturnedValue method_assert(SimpleCallContext *ctx);
    static ReturnedValue method_exception(SimpleCallContext *ctx);

};

struct GlobalExtensions {
    static void init(QQmlEngine *qmlEngine, Object *globalObject);

#ifndef QT_NO_TRANSLATION
    static ReturnedValue method_qsTranslate(SimpleCallContext *ctx);
    static ReturnedValue method_qsTranslateNoOp(SimpleCallContext *ctx);
    static ReturnedValue method_qsTr(SimpleCallContext *ctx);
    static ReturnedValue method_qsTrNoOp(SimpleCallContext *ctx);
    static ReturnedValue method_qsTrId(SimpleCallContext *ctx);
    static ReturnedValue method_qsTrIdNoOp(SimpleCallContext *ctx);
#endif
    static ReturnedValue method_gc(SimpleCallContext *ctx);

    // on String:prototype
    static ReturnedValue method_string_arg(SimpleCallContext *ctx);

};


}

QT_END_NAMESPACE

#endif // QQMLBUILTINFUNCTIONS_P_H
