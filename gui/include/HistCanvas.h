
#ifndef PRAD_HIST_CANVAS_H
#define PRAD_HIST_CANVAS_H

#include <QWidget>

class QRootCanvas;
class QGridLayout;
class TCanvas;
class TColor;
class TF1;
class TH1;
class TH2;
class TGraph;
class TMultiGraph;
class TLegend;

class HistCanvas : public QWidget
{
    Q_OBJECT

public:
    HistCanvas(QWidget *parent = 0);
    virtual ~HistCanvas() {}
    void AddCanvas(int row, int column, int fillColor = 38);
    void UpdateHist(int index, TH1 *hist, bool auto_range = true, std::string option = "hist");
    void UpdateHist(int index, TH1 *hist, int range_min, int range_max);
    void UpdateHist(int index, TH2 *hist);
    void UpdateHist(int index, TGraph *gr1 = nullptr, TGraph *gr2 = nullptr, TGraph *gr3 = nullptr, TLegend *legend = nullptr);
    std::array<double, 2> FitClusterEHist(TH1 *hist);


protected:
    QGridLayout *layout;
    TColor *bkgColor;
    TColor *frmColor;
    QVector<QRootCanvas *> canvases;
    QVector<int> fillColors;
};

#endif
