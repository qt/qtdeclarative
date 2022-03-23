#ifndef STATE_H
#define STATE_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace model {
namespace Window {
Q_NAMESPACE
QML_NAMED_ELEMENT(WindowState)
enum State {
    OPEN,
    TILT,
    UNCALIBRATED,
    CLOSE
};
Q_ENUM_NS(State)
}

class WindowInstance : public QObject
{
    Q_OBJECT
    QML_ELEMENT
};

}

#endif // STATE_H
