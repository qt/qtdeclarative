#ifndef OPAQUE_H
#define OPAQUE_H

#include "qqmlintegration.h"
#include <memory>
#include <QtCore/qobject.h>
#include <QtQmlIntegration/qqmlintegration.h>

class Secretive : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(std::shared_ptr<int> opaque_prop MEMBER m_opaque_prop)

public:
    Q_INVOKABLE std::shared_ptr<int> returnsOpaque() const { return std::make_shared<int>(42); }
    Q_INVOKABLE void takesOpaque(std::shared_ptr<int> ptr) { m_opaque_prop = ptr; ++takesOpaqueCallCount; }

    std::shared_ptr<int> m_opaque_prop {};
    int takesOpaqueCallCount = 0;
};

#endif
