#include "ScintillatorView.h"
#include "ScintillatorScene.h"

ScintillatorView::ScintillatorView(QWidget *parent)
    : QGraphicsView(parent)
{
    scene = new ScintillatorScene(this);

    setScene(scene);
}

ScintillatorScene* ScintillatorView::GetScene()
{
    return scene;
}