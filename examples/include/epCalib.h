#include "TH1F.h"
#include "TF1.h"

#include <string>

using namespace std;

double fitRaito(TH1F* hist)
{
    if (!hist) return 0;

    double fit_min, fit_max;
    double peak = hist->GetBinCenter( hist->GetMaximumBin() );
    fit_min = peak - 2. * 0.023;
    fit_max = peak + 2. * 0.023;
    TF1 *fitFunc = new TF1("fitFunc", "gaus", fit_min, fit_max);
    hist->Fit(fitFunc, "RQ");
    double mean_ratio = fitFunc->GetParameter(1);
    if(hist->GetEntries() < 100){
        mean_ratio = hist->GetMean();
    }
    string title = hist->GetTitle();
    title += Form(" : %.4f", mean_ratio);
    hist->GetXaxis()->SetTitle("E_meas / E_exp");
    hist->GetYaxis()->SetTitle("Counts");
    return mean_ratio;
}


