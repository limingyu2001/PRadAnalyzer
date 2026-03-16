#include "ScintillatorScene.h"
#include "ScintillatorModule.h"

ScintillatorScene::ScintillatorScene(QObject *parent)
    : QGraphicsScene(parent)
{
}

void ScintillatorScene::AddModule(QString name,
                                  QRectF rect)
{
    ScintillatorModule* m =
            new ScintillatorModule(name,rect);

    modules[name] = m;

    addItem(m);
}

void ScintillatorScene::UpdateADC(QString name,double val)
{
    if(modules.count(name))
        modules[name]->SetADC(val);
}

void ScintillatorScene::UpdateHit(QString name,bool hit)
{
    if(modules.count(name))
        modules[name]->SetHit(hit);
}

void ScintillatorScene::UpdateRate(QString name,double rate)
{
    if(modules.count(name))
        modules[name]->SetRate(rate);
}

void ScintillatorScene::Clear()
{
    clear();
    modules.clear();
}