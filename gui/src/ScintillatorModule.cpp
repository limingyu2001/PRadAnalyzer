#include "ScintillatorModule.h"
#include <QPainter>
#include <QToolTip>
#include <QGraphicsSceneHoverEvent>

ScintillatorModule::ScintillatorModule(const QString& name, const QRectF& rect)
    : name(name),
      rect(0,0,rect.width(),rect.height()),
      adc(0),
      tdc(0),
      hit(false),
      rate(0),
      hover(false)
{
    setPos(rect.x(), rect.y());

    setAcceptHoverEvents(true);

    setToolTip(QString("Module: %1").arg(name));
}

void ScintillatorModule::UpdateTooltip()
{
    QString tip = QString(
        "Module: %1\n"
        "ADC: %2\n"
        "TDC: %3\n"
        "Rate: %4")
            .arg(name)
            .arg(adc)
            .arg(tdc)
            .arg(rate);

    setToolTip(tip);
}

QRectF ScintillatorModule::boundingRect() const
{
    return QRectF(0,0,rect.width(),rect.height());
}

void ScintillatorModule::SetADC(double val)
{
    adc = val;
    UpdateTooltip();
    update();
}

void ScintillatorModule::SetTDC(double val)
{
    tdc = val;
    UpdateTooltip();
    update();
}

void ScintillatorModule::SetHit(bool val)
{
    hit = val;
    UpdateTooltip();
    update();
}

void ScintillatorModule::SetRate(double val)
{
    rate = val;
    UpdateTooltip();
    update();
}

void ScintillatorModule::paint(QPainter *painter,
                               const QStyleOptionGraphicsItem *,
                               QWidget *)
{
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF r = boundingRect();

    QColor color;

    if(hit)
        color = Qt::red;
    else if(rate > 1000)
        color = Qt::yellow;
    else
        color = QColor(60,60,60);

    painter->setBrush(color);

    if(hover)
        painter->setPen(QPen(Qt::cyan,3));
    else
        painter->setPen(QPen(Qt::gray,2));

    painter->drawRect(r);

    painter->setPen(Qt::white);

    QFont f = painter->font();
    f.setPointSize(8);
    painter->setFont(f);

    painter->drawText(r,
                      Qt::AlignCenter,
                      name);
}

void ScintillatorModule::Reset()
{
    adc = 0;
    tdc = 0;
    hit = false;
    rate = 0;

    update();
}

void ScintillatorModule::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    hover = true;
    update();
    QGraphicsItem::hoverEnterEvent(event);
}

void ScintillatorModule::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    hover = false;
    update();

    QGraphicsItem::hoverLeaveEvent(event);
}