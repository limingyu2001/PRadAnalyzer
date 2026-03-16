#ifndef SCINTILLATORVIEW_H
#define SCINTILLATORVIEW_H

#include <QGraphicsView>

class ScintillatorScene;

class ScintillatorView : public QGraphicsView
{

public:

    ScintillatorView(QWidget *parent=nullptr);

    ScintillatorScene* GetScene();

private:

    ScintillatorScene *scene;
};

#endif