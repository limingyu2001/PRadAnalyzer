//============================================================================//
// A class contains a few root canvas                                         //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include <QLayout>

#include "TSystem.h"
#include "TStyle.h"
#include "TColor.h"
#include "TF1.h"
#include "TH1.h"
#include "TH2.h"
#include "TAxis.h"
#include "TLatex.h"
#include "TMultiGraph.h"
#include "TLegend.h"
#include <array>

#include "HistCanvas.h"
#include "QRootCanvas.h"

#include <iostream>

#define HIST_FONT_SIZE 0.07
#define HIST_LABEL_SIZE 0.07

HistCanvas::HistCanvas(QWidget *parent) : QWidget(parent)
{
    layout = new QGridLayout(this);

    bkgColor = new TColor(200, 1, 1, 0.96);
    gStyle->SetTitleFontSize(HIST_FONT_SIZE);
    gStyle->SetStatFontSize(HIST_FONT_SIZE);
}

void HistCanvas::AddCanvas(int row, int column, int color)
{
    QRootCanvas *newCanvas = new QRootCanvas(this);
    canvases.push_back(newCanvas);
    fillColors.push_back(color);

    // add canvas in vertical layout
    layout->addWidget(newCanvas, row, column);
    newCanvas->SetFillColor(bkgColor->GetNumber());
    newCanvas->SetFrameFillColor(10); // white
}

void HistCanvas::UpdateHist(int index, TH1 *hist, bool auto_range, std::string option)
{
    if(!hist || index < 0 || index >= canvases.size())
        return;

    canvases[index]->cd();
    canvases[index]->SetGrid();

    //gPad->SetLogy();

    if(auto_range) {
        int firstBin = hist->FindFirstBinAbove(0,1)*0.7;
        int lastBin = hist->FindLastBinAbove(0,1)*1.3;

        hist->GetXaxis()->SetRange(firstBin, lastBin);
    }

    hist->GetXaxis()->SetLabelSize(HIST_LABEL_SIZE);
    hist->GetYaxis()->SetLabelSize(HIST_LABEL_SIZE);

    hist->SetFillColor(fillColors[index]);
    hist->Draw(option.c_str());

    canvases[index]->Refresh();
}

// show the histogram in first slot, try a Gaussian fit with given parameters
void HistCanvas::UpdateHist(int index, TH1 *hist, int range_min, int range_max)
{
    if(!hist || index < 0 || index >= canvases.size())
        return;

    canvases[index]->cd();
    canvases[index]->SetGrid();

    //gPad->SetLogy();

    hist->GetXaxis()->SetRange(range_min, range_max);

    hist->GetXaxis()->SetLabelSize(HIST_LABEL_SIZE);
    hist->GetYaxis()->SetLabelSize(HIST_LABEL_SIZE);

    hist->SetFillColor(fillColors[index]);
    hist->Draw();

    canvases[index]->Refresh();
}

void HistCanvas::UpdateHist(int index, TH2 *hist)
{
    if(!hist || index < 0 || index >= canvases.size())
        return;

    canvases[index]->cd();

    gPad->SetLogz();

    hist->GetXaxis()->SetLabelSize(HIST_LABEL_SIZE);
    hist->GetYaxis()->SetLabelSize(HIST_LABEL_SIZE);

    hist->Draw("colz");

    canvases[index]->Refresh();
}

void HistCanvas::UpdateHist(int index, TGraph *gr1, TGraph *gr2, TGraph *gr3, TLegend *legend)
{
    if((!gr1 && !gr2 && !gr3) || index < 0 || index >= canvases.size())
        return;

    canvases[index]->cd();

    TMultiGraph *mg = new TMultiGraph();
    
    if(gr1) mg->Add(gr1);
    if(gr2) mg->Add(gr2);
    if(gr3) mg->Add(gr3);

    mg->GetXaxis()->SetLabelSize(HIST_LABEL_SIZE);
    mg->GetYaxis()->SetLabelSize(HIST_LABEL_SIZE);

    mg->Draw("ALP");
    if (legend) legend->Draw();

    canvases[index]->Refresh();
}

//new
 std::array<double, 2> HistCanvas::FitClusterEHist(TH1 *hist)
{
    /*if(!hist || index < 0 || index >= canvases.size())
        return;

    canvases[index]->cd();*/

    double fit_min, fit_max;
    double peak = hist->GetBinCenter( hist->GetMaximumBin() );
    if( hist->GetBinContent( hist->GetMaximumBin() - 1 ) < 0.1 * hist->GetBinContent( hist->GetMaximumBin() ) ){
        hist->SetBinContent( hist->GetMaximumBin(), 0);
        peak = hist->GetBinCenter( hist->GetMaximumBin() );
    }

    fit_min = peak - 2. * 0.026 / sqrt(peak/1000.) * peak;
    fit_max = peak + 2. * 0.026 / sqrt(peak/1000.) * peak;
    TF1 *fitFunc = new TF1("fitFunc", "gaus", fit_min, fit_max);
    hist->Fit(fitFunc, "RQ");
    double mean = fitFunc->GetParameter(1);
    double sigma = fitFunc->GetParameter(2);
    std::string title = hist->GetTitle();
    title += Form(" Peak: %.1f +- %.1f MeV, Res: %.2f %% / sqrt(E[GeV])", mean, sigma, sigma / mean * sqrt(mean/1000.) * 100. );
    hist->SetTitle(title.c_str());
    std::array<double, 2> result = {mean, sigma};
    return result;
}
