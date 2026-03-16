// RefPMTParam.h
#ifndef REF_PMT_PARAM_H
#define REF_PMT_PARAM_H

#include <vector>
struct ReferencePMTParam {
    double alpha_signal;
    double alpha_error;
    double lms_signal;
    double lms_error;
    bool valid;
};

#endif // REF_PMT_PARAM_H