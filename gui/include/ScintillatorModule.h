#ifndef SCINTILLATORMODULE_H
#define SCINTILLATORMODULE_H

#include <QGraphicsItem>
#include <QString>

class ScintillatorModule : public QGraphicsItem
{
public:

    ScintillatorModule(const QString& name, const QRectF& rect);

    QRectF boundingRect() const override;

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *,
               QWidget *) override;

    void SetADC(double val);
    void SetTDC(double val);
    void SetHit(bool val);
    void SetRate(double val);

    void Reset();

    QString GetName() const { return name; }

    QRectF GetRect() const { return rect; }

protected:

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:

    QString name;

    QRectF rect;

    double adc;
    double tdc;

    bool hit;

    double rate;

    bool hover;

    void UpdateTooltip();
};

#endif