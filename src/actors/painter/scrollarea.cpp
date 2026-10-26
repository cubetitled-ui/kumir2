#include "scrollarea.h"

#include <QWheelEvent>
#include <QScrollBar>

namespace ActorPainter {

ScrollArea::ScrollArea(QWidget *parent) :
    QScrollArea(parent)
{
}

void ScrollArea::wheelEvent(QWheelEvent *evt)
{
    const int delta = wheelEventDelta(evt);
    const int degrees = delta / 8;
    const int steps = -1 * degrees / 15;
    if (! (evt->modifiers() & Qt::ControlModifier) ) {
        // Scroll contents
        QAbstractSlider* slider = (evt->angleDelta().y() != 0)
                ? verticalScrollBar()
                : horizontalScrollBar();
        if ((evt->angleDelta().y() != 0) && evt->modifiers() & Qt::ShiftModifier) {
            slider = horizontalScrollBar();
        }
        if (slider->isVisible()) {
            int value = slider->value() + steps * slider->singleStep();
            value = qMin(value, slider->maximum());
            slider->setValue(value);
            evt->accept();
        }
        else {
            evt->ignore();
        }
    }
    else {
        // Zoom
        emit changeZoomRequest(steps);
        evt->accept();
    }
}

} // namespace ActorPainter
