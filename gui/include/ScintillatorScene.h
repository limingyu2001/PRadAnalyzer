#ifndef SCINTILLATORSCENE_H
#define SCINTILLATORSCENE_H

#include <QGraphicsScene>
#include <map>

class ScintillatorModule;

class ScintillatorScene : public QGraphicsScene
{

public:

    ScintillatorScene(QObject *parent=nullptr);

    void AddModule(QString name,
                   QRectF rect);

    void UpdateADC(QString name,double val);

    void UpdateHit(QString name,bool hit);

    void UpdateRate(QString name,double rate);

    void Clear();

private:

    std::map<QString,ScintillatorModule*> modules;
};

#endif