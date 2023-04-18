#ifndef CLICKABLE_COLOR_Q_FRAME_H
#define CLICKABLE_COLOR_Q_FRAME_H

#include <QFrame>
#include <QMouseEvent>

class ClickableColorQFrame : public QFrame
{
    Q_OBJECT

public:
    ClickableColorQFrame(QWidget* parent) : QFrame(parent) {}

signals:
    void clicked();

public:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            emit clicked();
        }
    }
};

#endif 
