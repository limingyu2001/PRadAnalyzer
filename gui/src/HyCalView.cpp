//============================================================================//
// A class to contain all the graphic items, including spectrum and modules   //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include "HyCalView.h"
#include "HyCalModule.h"
#include <cmath>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QMouseEvent>

HyCalView::HyCalView(QWidget *parent)
: QGraphicsView(parent)
{
    // enable tracking mode
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
}

void HyCalView::wheelEvent(QWheelEvent *event)
{
    // zoom in / zoom out on mouse wheel scrolling
    double numDegrees = -event->delta() / 8.0;
    double numSteps = numDegrees / 15.0;
    double factor = std::pow(1.125, numSteps);
    scale(factor, factor);
}

void HyCalView::keyPressEvent(QKeyEvent *event)
{
    // press ctrl to enable drag mode
    if(event->key() == Qt::Key_Control) {
        setDragMode(ScrollHandDrag);
    }
}

void HyCalView::keyReleaseEvent(QKeyEvent *event)
{
    // release ctrl to disable drag mode
    if(event->key() == Qt::Key_Control) {
        setDragMode(NoDrag);
    }
}

void HyCalView::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
    QPointF scenePos = mapToScene(event->pos());
    QGraphicsItem* item = scene()->itemAt(scenePos, transform());
    if (!item) {
        QToolTip::hideText();
        return;
    }

    HyCalModule* module = dynamic_cast<HyCalModule*>(item);
    if (!module) {
        QToolTip::hideText();
        return;
    }

    std::string moduleName = module->GetName();
    if (moduleName.empty()) {
        QToolTip::hideText();
        return;
    }

    HyCalScene* hycalScene = dynamic_cast<HyCalScene*>(scene());
    if (!hycalScene) {
        QToolTip::hideText();
        return;
    }

    auto abnormalModules = hycalScene->getAbnormalModules();
    if (abnormalModules.count(moduleName)) {
        double changeRatio = abnormalModules[moduleName];
        QString tip = QString(
            "Ref PMT %1\n"
            "Module: %2\n"
            "Gain Change: %3%"
        ).arg(QString::fromStdString(moduleName))
         .arg(changeRatio * 100, 0, 'f', 1);

        QToolTip::showText(event->globalPos(), tip, this);
    } else {
        QToolTip::hideText();
    }
}

