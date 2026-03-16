#ifndef FIT_UTILS_H
#define FIT_UTILS_H

#include <utility>
#include <vector>
#include "TH1.h"
#include "RefPMTParam.h"
#include "PRadHyCalSystem.h"

class FitUtils
{
public:
    static std::pair<double, double> GaussianFit(TH1* hist, double xMin, double xMax);
    static std::vector<ReferencePMTParam> CalculateRefPMTParams(PRadHyCalSystem* hycal_sys);
};

#endif // FIT_UTILS_H