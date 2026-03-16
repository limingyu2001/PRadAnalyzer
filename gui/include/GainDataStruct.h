#ifndef GAIN_DATA_STRUCT_H
#define GAIN_DATA_STRUCT_H

struct ModuleGainData {
    int run_number;
    double cumulative_time;
    double gain1;
    double gain1_err;
    double gain2;
    double gain2_err;
    double gain3;
    double gain3_err;
    double lms_mean;
    double lms_err;
    double ped_mean;
    double ped_err;
    double net_signal;
    double net_signal_err;
};

struct RefPMTLMSData {
    int run_number;
    double lms_signal;
    double lms_error;
    double cumulative_time; 
};

#endif // GAIN_DATA_STRUCT_H