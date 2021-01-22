/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQMLPROPERTYCACHEVECTOR_P_H
#define QQMLPROPERTYCACHEVECTOR_P_H

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

#include <private/qflagpointer_p.h>
#include <private/qqmlpropertycache_p.h>

QT_BEGIN_NAMESPACE

class QQmlPropertyCacheVector
{
public:
    QQmlPropertyCacheVector() {}
    QQmlPropertyCacheVector(QQmlPropertyCacheVector &&other)
        : data(std::move(other.data)) {}
    QQmlPropertyCacheVector &operator=(QQmlPropertyCacheVector &&other) {
        QVector<QFlagPointer<QQmlPropertyCache>> moved(std::move(other.data));
        data.swap(moved);
        return *this;
    }

    ~QQmlPropertyCacheVector() { clear(); }
    void resize(int size) { return data.resize(size); }
    int count() const { return data.count(); }
    void clear()
    {
        for (int i = 0; i < data.count(); ++i) {
            if (QQmlPropertyCache *cache = data.at(i).data())
                cache->release();
        }
        data.clear();
    }

    void append(QQmlPropertyCache *cache) { cache->addref(); data.append(cache); }
    QQmlPropertyCache *at(int index) const { return data.at(index).data(); }
    void set(int index, const QQmlRefPointer<QQmlPropertyCache> &replacement) {
        if (QQmlPropertyCache *oldCache = data.at(index).data()) {
            if (replacement.data() == oldCache)
                return;
            oldCache->release();
        }
        data[index] = replacement.data();
        replacement->addref();
    }

    void setNeedsVMEMetaObject(int index) { data[index].setFlag(); }
    bool needsVMEMetaObject(int index) const { return data.at(index).flag(); }
private:
    Q_DISABLE_COPY(QQmlPropertyCacheVector)
    QVector<QFlagPointer<QQmlPropertyCache>> data;
};

QT_END_NAMESPACE

#endif // QQMLPROPERTYCACHEVECTOR_P_H
