// FitUtils.cpp
#include "FitUtils.h"
#include "TF1.h"
#include "TH1.h"
#include "PRadHyCalSystem.h"
#include "HyCalModule.h"
#include "QMessageBox" 
#include <cmath>
#include <stdexcept>

static TF1* GetGaussFitFunc()
{
    static TF1* gaussFunc = new TF1("gauss_fit_static", "gaus", 0, 12000);
    return gaussFunc;
}

std::pair<double, double> FitUtils::GaussianFit(TH1* hist, double xMin, double xMax)
{
    if (!hist || hist->GetEntries() < 10) {
        return {0.0, 0.0};
    }

    TF1* gaussFunc = GetGaussFitFunc();
    gaussFunc->SetRange(xMin, xMax);

    int fitStatus = hist->Fit(gaussFunc, "RQN");
    if (fitStatus != 0) {
        return {0.0, 0.0};
    }

    double mean = gaussFunc->GetParameter(1); 
    double meanErr = gaussFunc->GetParError(1);
    double sigma = gaussFunc->GetParameter(2);

    if (mean < xMin || mean > xMax || meanErr < 0) {
        return {0.0, 0.0};
    }
    if (sigma < 0 || sigma > (xMax - xMin)) { 
        return {0.0, 0.0};
    }

    return {mean, meanErr};
}

std::vector<ReferencePMTParam> FitUtils::CalculateRefPMTParams(PRadHyCalSystem* hycal_sys)
{
    std::vector<ReferencePMTParam> refPMTParams(3);
    if (!hycal_sys) {
        QMessageBox::warning(nullptr, "Warning", "HyCal system is null!");
        return refPMTParams;
    }

    auto refChannels = HyCalModule::GetAllREFPMTChannels(hycal_sys);
    if (!refChannels[0] && !refChannels[1] && !refChannels[2]) {
        QMessageBox::warning(nullptr, "Warning", "No valid Reference PMT channels found!");
        return refPMTParams;
    }

    for (int i = 0; i < 3; ++i) {
        ReferencePMTParam& param = refPMTParams[i];
        param.valid = false;
        if (!refChannels[i]) continue;

        TH1* phyHist = refChannels[i]->GetHist("Physics");
        if (!phyHist || phyHist->GetEntries() < 10) continue;

        auto [leftMean, leftMeanErr] = GaussianFit(phyHist, 0, 1000);
        if (leftMean < 0 || leftMeanErr < 0) continue;

        double rightMean = 0.0, rightMeanErr = 0.0;
        if (i == 0) {
            auto [mean1, err1] = GaussianFit(phyHist, 4000, 6000);
            if (mean1 >= 0 && err1 >= 0) {
                rightMean = mean1;
                rightMeanErr = err1;
            } else {
                auto [mean2, err2] = GaussianFit(phyHist, 1000, 3000);
                rightMean = mean2;
                rightMeanErr = err2;
            }
        } else if (i == 1) {
            auto [mean1, err1] = GaussianFit(phyHist, 2000, 4000);
            if (mean1 >= 0 && err1 >= 0) {
                rightMean = mean1;
                rightMeanErr = err1;
            } else {
                auto [mean2, err2] = GaussianFit(phyHist, 800, 1200);
                rightMean = mean2;
                rightMeanErr = err2;
            }
        } else { 
            auto [mean1, err1] = GaussianFit(phyHist, 5000, 7000);
            if (mean1 >= 0 && err1 >= 0) {
                rightMean = mean1;
                rightMeanErr = err1;
            } else {
                auto [mean2, err2] = GaussianFit(phyHist, 1000, 12000);
                rightMean = mean2;
                rightMeanErr = err2;
            }
        }
        if (rightMean < 0 || rightMeanErr < 0) continue;

        param.alpha_signal = rightMean - leftMean;
        param.alpha_error = sqrt(pow(rightMeanErr, 2) + pow(leftMeanErr, 2));
        TH1* lmsHist = refChannels[i]->GetHist("LMS");
        if (!lmsHist || lmsHist->GetEntries() < 10) continue;
        auto [lmsMean, lmsMeanErr] = GaussianFit(lmsHist, 
                                                lmsHist->GetXaxis()->GetXmin(), 
                                                lmsHist->GetXaxis()->GetXmax());
        if (lmsMean < 0 || lmsMeanErr < 0) continue;
        param.lms_signal = lmsMean - leftMean;
        param.lms_error = sqrt(pow(lmsMeanErr, 2) + pow(leftMeanErr, 2));
        param.valid = (param.alpha_signal >= 0 && param.lms_signal >= 0);
        if (!param.valid) {
            QMessageBox::warning(nullptr, "Warning", 
                                QString("Reference PMT %1 params invalid!").arg(i+1));
        }
    }

    return refPMTParams;
}