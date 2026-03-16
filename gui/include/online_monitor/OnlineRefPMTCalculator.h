#ifndef ONLINE_REFPMT_CALCULATOR_H
#define ONLINE_REFPMT_CALCULATOR_H

#include "PRadEventStruct.h"
#include "RefPMTParam.h"
#include "GainDataStruct.h"

#include <string>
#include <vector>
#include <array>
#include <map>
#include <utility>
#include <TH1F.h>
#include <ctime>
#include <TFile.h>
#include <TDirectory.h>
#include <fstream>

struct OnlineModuleInfo {
    std::string name;
    int number;
    double lms_mean;
    double lms_err;
    double ped_mean;
    double ped_err;
    double net_signal;
    double net_signal_err;
    double gain1;
    double gain1_err;
    double gain2;
    double gain2_err;
    double gain3;
    double gain3_err;
    long long lms_entries;
    double cumulative_time;
};

class PRadHyCalSystem;
class PRadADCChannel;
class HyCalModule;

struct OnlineModuleData {
    PRadADCChannel* channel = nullptr;
    TH1F* calc_lms_hist = nullptr;
    TH1F* calc_pedestal_hist = nullptr;
    long long valid_lms_count = 0;
};

class OnlineRefPMTCalculator {
public:
    static const std::string OUTPUT_DIR;
    static const std::string EVENT_TIME_DIR;
    static const int HIST_BINS = 2048;
    static constexpr double HIST_MIN = 0.0;
    static constexpr double HIST_MAX = 8191.0;
    static const int FIT_TRIGGER_EVENTS = 1200;
    static constexpr double MODULE_LMS_MIN = 0.0;
    static constexpr double MODULE_LMS_MAX = 8191.0;
    static constexpr double MODULE_PED_MIN = 0.0;
    static constexpr double MODULE_PED_MAX = 1023.0;

public:
    OnlineRefPMTCalculator();
    ~OnlineRefPMTCalculator();

    void Init(PRadHyCalSystem* hycal_sys);
    void ProcessOnlineEvent(const EventData& event);
    bool IsInitialized() const { return is_initialized_; }

private:
    uint64_t first_event_timestamp_;
    uint64_t event_counter_;
    std::ofstream event_time_file_;
    std::string event_time_filename_;

    std::string fit_start_utc_str_;
    std::string fit_end_utc_str_;
    std::string fit_start_edt_str_;
    std::string fit_end_edt_str_;
    double fit_start_sec_;
    double fit_end_sec_;
    int fit_event_count_;

    void InitEventTimeLogging(); 
    void CloseEventTimeLogging();
    void WriteEventTimeRecord(const EventData& event);
    
    PRadHyCalSystem* hycal_sys_;
    bool is_initialized_;
    std::array<PRadADCChannel*, 3> refpmt_channels_;
    std::array<long long, 3> valid_event_count_;
    std::array<TH1F*, 3> calc_lms_hist_;
    std::array<TH1F*, 3> calc_physics_hist_;
    time_t online_start_time_;

    std::vector<OnlineModuleData> module_data_list_;
    std::map<std::string, size_t> module_name_to_idx_;
    int module_fit_trigger_ = FIT_TRIGGER_EVENTS;
    
    int GetCurrentRunNumber();
    void FindRefPMTChannels(PRadHyCalSystem* hycal_sys);
    void CreateCalcHistograms();
    void ValidateTriggerMapping();
    std::pair<double, double> CalculateLMSignal(int pmt_idx);
    
    void SaveFitResult(const std::vector<RefPMTLMSData>& refpmt_records, 
                       const std::vector<OnlineModuleInfo>& module_info_list,
                       const std::string& fit_start_utc,
                       const std::string& fit_end_utc,
                       const std::string& fit_start_edt,
                       const std::string& fit_end_edt,
                       double avg_time_sec,
                       const std::string& avg_time_str);
    
    void ClearCalcHistograms();

    void FindAllModules(PRadHyCalSystem* hycal_sys);
    void CreateModuleHistograms();
    void ProcessModuleEvent(const EventData& event);
    bool CheckAllModulesReady() const;
    
    std::vector<OnlineModuleInfo> CalculateAllModuleGains(const std::vector<ReferencePMTParam>& refpmt_params, 
                                                        double avg_timestamp);
    
    void ClearModuleHistograms();

    static int GetMaxFileIndex(const std::string& output_dir, const std::string& base_name);
};

std::string GetCurrentDateString();
std::string GetEventTimeFileName(const std::string& dir);

double ConvertTimeStringToTimestamp(const std::string& time_str);
std::string ConvertTimestampToTimeString(double timestamp_sec);

#endif // ONLINE_REFPMTCALCULATOR_H