#include "OnlineRefPMTCalculator.h"
#include "PRadInfoCenter.h" 
#include "FitUtils.h"
#include "HyCalModule.h" 
#include "QDir"
#include "fstream"
#include "iomanip"
#include "cmath"
#include "iostream"
#include "sstream"
#include <TStyle.h>
#include <QRegExp>
#include <algorithm>
#include "TF1.h"
#include <string>
#include <ctime>
#include <chrono>
#include <sstream>

const std::string OnlineRefPMTCalculator::OUTPUT_DIR = "online_data";
const std::string OnlineRefPMTCalculator::EVENT_TIME_DIR = "online_event_time";
const int OnlineRefPMTCalculator::HIST_BINS;

double ConvertTimeStringToTimestamp(const std::string& time_str)
{
    std::string clean_str = time_str;
    size_t utc_pos = clean_str.find("UTC");
    size_t edt_pos = clean_str.find("EDT");
    if (utc_pos != std::string::npos) {
        clean_str = clean_str.substr(0, utc_pos);
    } else if (edt_pos != std::string::npos) {
        clean_str = clean_str.substr(0, edt_pos);
    }
    clean_str.erase(0, clean_str.find_first_not_of(" \t"));
    clean_str.erase(clean_str.find_last_not_of(" \t") + 1);

    std::tm tm = {};
    double fractional_sec = 0.0;
    std::istringstream ss(clean_str);
    
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        return -1.0;
    }

    if (ss.peek() == '.') {
        ss.ignore();
        std::string frac_str;
        ss >> frac_str;
        frac_str = frac_str.substr(0, 6);
        while (frac_str.length() < 6) {
            frac_str += "0";
        }
        fractional_sec = std::stod("0." + frac_str);
    }

    std::time_t t = timegm(&tm);
    if (t == -1) {
        return -1.0;
    }

    return static_cast<double>(t) + fractional_sec;
}

std::string ConvertTimestampToTimeString(double timestamp_sec)
{
    if (timestamp_sec < 0) {
        return "Invalid Time";
    }

    std::time_t t_sec = static_cast<std::time_t>(timestamp_sec);
    long long usec = static_cast<long long>((timestamp_sec - t_sec) * 1e6);

    std::tm* utc_tm = std::gmtime(&t_sec);
    if (!utc_tm) {
        return "Invalid Time";
    }

    char buf[128];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", utc_tm);
    return std::string(buf) + "." + std::to_string(usec).substr(0, 6) + " (UTC)";
}

std::string GetEventTimeFileName(const std::string& dir)
{
    time_t now = time(nullptr);
    char time_str[20];
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(time_str, sizeof(time_str), "%Y%m%d_%H%M%S", &timeinfo);
    return dir + "/online_event_time_" + std::string(time_str) + ".txt";
}

#ifdef USE_ONLINE_MODE

void OnlineRefPMTCalculator::CloseEventTimeLogging()
{
    if (event_time_file_.is_open()) {
        event_time_file_.close();
        //std::cout << "[OnlineCalc] Event time logging closed: " << event_time_filename_ << std::endl;
    }
}

void OnlineRefPMTCalculator::InitEventTimeLogging()
{
    QDir event_time_dir(QString::fromStdString(EVENT_TIME_DIR));
    if (!event_time_dir.exists() && !event_time_dir.mkpath(".")) {
        std::cerr << "[OnlineCalc] Failed to create event time dir: " << EVENT_TIME_DIR << std::endl;
        return;
    }

    event_time_filename_ = GetEventTimeFileName(EVENT_TIME_DIR);
    bool file_is_empty = true;
    std::ifstream check_file(event_time_filename_);
    if (check_file.is_open()) {
        check_file.seekg(0, std::ios::end);
        file_is_empty = (check_file.tellg() == 0);
        check_file.close();
    }

    event_time_file_.open(event_time_filename_, std::ios::out | std::ios::app);
    if (!event_time_file_.is_open()) {
        std::cerr << "[OnlineCalc] Failed to open event time file: " << event_time_filename_ << std::endl;
        return;
    }

    if (file_is_empty) {
        event_time_file_ << std::left 
                        << std::setw(12) << "Event Number  " 
                        << std::setw(40) << "Absolute Time (UTC)  " 
                        << std::setw(40) << "Absolute Time (EDT)  " 
                        << std::setw(20) << "Relative Time (sec)  " 
                        << std::setw(15) << "Trigger Type" 
                        << std::endl;
        event_time_file_ << std::string(120, '-') << std::endl;
    }

    first_event_timestamp_ = 0;
    event_counter_ = 0;

    fit_start_utc_str_ = "";
    fit_end_utc_str_ = "";
    fit_start_edt_str_ = "";
    fit_end_edt_str_ = "";
    fit_start_sec_ = 0.0;
    fit_end_sec_ = 0.0;
    fit_event_count_ = 0;

    //std::cout << "[OnlineCalc] Event time logging initialized: " << event_time_filename_ << std::endl;
}

void OnlineRefPMTCalculator::WriteEventTimeRecord(const EventData& event)
{
    // if (!event_time_file_.is_open()) {
    //     std::cerr << "[OnlineCalc] Event time file not open, skip writing record" << std::endl;
    //     return;
    // }

    std::string utc_time = event.absolute_time_utc.empty() ? "N/A" : event.absolute_time_utc;
    std::string edt_time = event.absolute_time_edt.empty() ? "N/A" : event.absolute_time_edt;
    double rel_time_sec = event.relative_time_sec;

    if (fit_event_count_ == 0) {
        fit_start_utc_str_ = utc_time;
        fit_start_edt_str_ = edt_time;
        fit_start_sec_ = ConvertTimeStringToTimestamp(utc_time);
    }
    fit_end_utc_str_ = utc_time;
    fit_end_edt_str_ = edt_time;
    fit_end_sec_ = ConvertTimeStringToTimestamp(utc_time);
    fit_event_count_++;

    if (event_counter_ == 0) {
        first_event_timestamp_ = event.timestamp;
    }

    std::string trigger_type;
    // switch (event.trigger) {
    //     case LMS_Led: trigger_type = "LMS_Led"; break;
    //     case LMS_Alpha: trigger_type = "LMS_Alpha"; break;
    //     default: trigger_type = "Unknown(" + std::to_string(event.trigger) + ")"; break;
    // }

    // event_time_file_ << std::left 
    //                 << std::setw(12) << event_counter_
    //                 << std::setw(40) << utc_time
    //                 << std::setw(40) << edt_time
    //                 << std::setw(20) << std::fixed << std::setprecision(6) << rel_time_sec
    //                 << std::setw(15) << trigger_type
    //                 << std::endl;

    // event_time_file_.flush();

    // if ((event_counter_ + 1) % 1000 == 0) {
    //     std::cout << "[OnlineCalc] Processed " << event_counter_ + 1 << " events (time logging)" << std::endl;
    // }

    event_counter_++;
}

OnlineRefPMTCalculator::OnlineRefPMTCalculator()
    : first_event_timestamp_(0), event_counter_(0),
      hycal_sys_(nullptr), is_initialized_(false),
      fit_start_sec_(0.0), fit_end_sec_(0.0), fit_event_count_(0)
{
    for (int i = 0; i < 3; ++i) {
        refpmt_channels_[i] = nullptr;
        valid_event_count_[i] = 0;
        calc_lms_hist_[i] = nullptr;
        calc_physics_hist_[i] = nullptr;
    }

    online_start_time_ = time(nullptr);

    //std::cout << "[OnlineCalc] Init: Current session start time = " << ctime(&online_start_time_);
}

OnlineRefPMTCalculator::~OnlineRefPMTCalculator()
{
    for (int i = 0; i < 3; ++i) {
        if (calc_lms_hist_[i]) delete calc_lms_hist_[i];
        if (calc_physics_hist_[i]) delete calc_physics_hist_[i];
    }

    for (auto& module_data : module_data_list_) {
        if (module_data.calc_lms_hist) delete module_data.calc_lms_hist;
        if (module_data.calc_pedestal_hist) delete module_data.calc_pedestal_hist;
    }
    //CloseEventTimeLogging();
}

void OnlineRefPMTCalculator::Init(PRadHyCalSystem* hycal_sys)
{
    if (!hycal_sys) {
        std::cerr << "[OnlineCalc] Init failed: HyCal system is null!" << std::endl;
        return;
    }
    hycal_sys_ = hycal_sys;

    FindRefPMTChannels(hycal_sys);
    CreateCalcHistograms();
    ValidateTriggerMapping();

    FindAllModules(hycal_sys);
    CreateModuleHistograms();

    QDir output_dir(QString::fromStdString(OUTPUT_DIR));
    if (!output_dir.exists() && !output_dir.mkpath(".")) {
         std::cerr << "[OnlineCalc] Failed to create dir: " << OUTPUT_DIR << std::endl;
     }

    //InitEventTimeLogging();

    is_initialized_ = true;
}

void OnlineRefPMTCalculator::FindAllModules(PRadHyCalSystem* hycal_sys)
{
    if (!hycal_sys) return;

    const std::vector<PRadADCChannel*>& all_channels = hycal_sys->GetADCList();
    for (auto* channel : all_channels) {
        if (!channel) continue;

        std::string module_name = channel->GetName();
        if (module_name.find("LMS") != std::string::npos) continue;
        if (module_name[0] != 'G' && module_name[0] != 'W') continue;

        OnlineModuleData module_data;
        module_data.channel = channel;
        module_name_to_idx_[module_name] = module_data_list_.size();
        module_data_list_.push_back(module_data);
    }
}

void OnlineRefPMTCalculator::CreateModuleHistograms()
{
    for (auto& module_data : module_data_list_) {
        if (!module_data.channel) continue;

        std::string module_name = module_data.channel->GetName();
        std::string lms_hist_name = "calc_module_lms_" + module_name;
        std::string ped_hist_name = "calc_module_ped_" + module_name;

        module_data.calc_lms_hist = new TH1F(lms_hist_name.c_str(), 
                                             (module_name + " Online LMS").c_str(),
                                             HIST_BINS, HIST_MIN, HIST_MAX);
        module_data.calc_lms_hist->SetDirectory(nullptr);

        module_data.calc_pedestal_hist = new TH1F(ped_hist_name.c_str(),
                                                  (module_name + " Online Pedestal").c_str(),
                                                  1050, 0, 1023);
        module_data.calc_pedestal_hist->SetDirectory(nullptr);
    }
}

void OnlineRefPMTCalculator::ProcessOnlineEvent(const EventData& event)
{
    if (!is_initialized_ || !hycal_sys_) return;

    const int trigger = event.trigger;
    bool is_lms_trigger = (trigger == LMS_Led); 
    bool is_ped_trigger = (trigger == LMS_Alpha);
    if (!is_lms_trigger && !is_ped_trigger) return;

    WriteEventTimeRecord(event);

    for (int i = 0; i < 3; ++i) {
        PRadADCChannel* ref_chan = refpmt_channels_[i];
        if (!ref_chan || !calc_lms_hist_[i] || !calc_physics_hist_[i]) continue;

        double adc_value = -1.0;
        for (const auto& adc_data : event.adc_data) {
            if (adc_data.channel_id == ref_chan->GetID()) {
                adc_value = adc_data.value;
                ref_chan->SetValue(adc_data.value);
                break;
            }
        }
        if (adc_value < 0) continue;

        if (is_lms_trigger) {
            calc_lms_hist_[i]->Fill(adc_value);
            valid_event_count_[i]++;
        } else if (is_ped_trigger) {
            calc_physics_hist_[i]->Fill(adc_value);
        }
    }

    ProcessModuleEvent(event);

    bool refpmt_ready = true;
    for (int i = 0; i < 3; ++i) {
        if (valid_event_count_[i] < FIT_TRIGGER_EVENTS) {
            refpmt_ready = false;
            break;
        }
    }

    bool modules_ready = CheckAllModulesReady();
    if (refpmt_ready && modules_ready) {
        std::vector<RefPMTLMSData> refpmt_records;
        std::vector<ReferencePMTParam> refpmt_params = FitUtils::CalculateRefPMTParams(hycal_sys_);

        double avg_time_sec = 0.0;
        std::string avg_time_str = "Invalid Time";
        if (fit_event_count_ > 0 && fit_start_sec_ >= 0 && fit_end_sec_ >= 0) {
            avg_time_sec = (fit_start_sec_ + fit_end_sec_) / 2.0;
            avg_time_str = ConvertTimestampToTimeString(avg_time_sec);
        }

        for (int i = 0; i < 3; ++i) {
            RefPMTLMSData record;
            record.run_number = GetCurrentRunNumber();
            record.cumulative_time = avg_time_sec;
            record.lms_signal = refpmt_params[i].lms_signal;
            record.lms_error = refpmt_params[i].lms_error;
            refpmt_records.push_back(record);
        }

        std::vector<OnlineModuleInfo> module_gain_records = CalculateAllModuleGains(refpmt_params, avg_time_sec);
        SaveFitResult(refpmt_records, module_gain_records, 
                     fit_start_utc_str_, fit_end_utc_str_, 
                     fit_start_edt_str_, fit_end_edt_str_,
                     avg_time_sec, avg_time_str);

        ClearCalcHistograms();
        ClearModuleHistograms();
        
        fit_start_utc_str_ = "";
        fit_end_utc_str_ = "";
        fit_start_edt_str_ = "";
        fit_end_edt_str_ = "";
        fit_start_sec_ = 0.0;
        fit_end_sec_ = 0.0;
        fit_event_count_ = 0;
    }
}

void OnlineRefPMTCalculator::ProcessModuleEvent(const EventData& event)
{
    const int trigger = event.trigger;
    bool is_lms_trigger = (trigger == LMS_Led);
    bool is_ped_trigger = (trigger == LMS_Alpha);
    if (!is_lms_trigger && !is_ped_trigger) return;

    for (const auto& adc_data : event.adc_data) {
        PRadADCChannel* adc_chan = hycal_sys_->GetADCChannel(adc_data.channel_id);
        if (!adc_chan) continue;

        auto it = module_name_to_idx_.find(adc_chan->GetName());
        if (it == module_name_to_idx_.end()) continue;

        size_t module_idx = it->second;
        OnlineModuleData& module_data = module_data_list_[module_idx];
        if (!module_data.calc_lms_hist || !module_data.calc_pedestal_hist) continue;

        double adc_value = adc_data.value;
        if (is_lms_trigger) {
                module_data.calc_lms_hist->Fill(adc_value);
                module_data.valid_lms_count++;
        } else if (is_ped_trigger) {
                module_data.calc_pedestal_hist->Fill(adc_value);
        }
    }
}

bool OnlineRefPMTCalculator::CheckAllModulesReady() const
{
    if (module_data_list_.empty()) return false;

    for (const auto& module_data : module_data_list_) {
        if (module_data.valid_lms_count < module_fit_trigger_) {
            return false;
        }
    }
    return true;
}

std::vector<OnlineModuleInfo> OnlineRefPMTCalculator::CalculateAllModuleGains(const std::vector<ReferencePMTParam>& refpmt_params, 
                                                                             double avg_timestamp)
{
    std::vector<OnlineModuleInfo> g_modules;
    std::vector<OnlineModuleInfo> w_modules;
    int run_number = GetCurrentRunNumber();

    for (const auto& module_data : module_data_list_) {
        if (!module_data.channel || !module_data.calc_lms_hist || !module_data.calc_pedestal_hist) continue;

        std::string module_name = module_data.channel->GetName();
        char module_type = module_name[0];
        //int module_number = HyCalModule::ExtractModuleNumber(module_name);
        int module_number = QString::fromStdString(module_name).mid(1).toInt();
        if (module_number < 0) {
            std::cerr << "[OnlineCalc] Warning: Invalid module number for " << module_name << std::endl;
            continue;
        }

        OnlineModuleInfo info;
        info.name = module_name;
        info.number = module_number;
        info.cumulative_time = avg_timestamp;
        info.lms_entries = module_data.calc_lms_hist->GetEntries();

        auto [lms_mean, lms_err] = FitUtils::GaussianFit(module_data.calc_lms_hist, 
                                                       MODULE_LMS_MIN, MODULE_LMS_MAX);
        if (lms_mean <= 0 || lms_err < 0 || std::isnan(lms_mean) || std::isnan(lms_err)) {
            std::cerr << "[OnlineCalc] Module " << module_name << " LMS fit failed (mean=" << lms_mean << ", err=" << lms_err << ")" << std::endl;
            info.lms_mean = -1.0;
            info.lms_err = -1.0;
            info.ped_mean = -1.0;
            info.ped_err = -1.0;
            info.net_signal = -1.0;
            info.net_signal_err = -1.0;
            info.gain1 = info.gain2 = info.gain3 = 0.0;
            info.gain1_err = info.gain2_err = info.gain3_err = 0.0;
            if (module_type == 'G') g_modules.push_back(info);
            else if (module_type == 'W') w_modules.push_back(info);
            continue;
        }
        info.lms_mean = lms_mean;
        info.lms_err = lms_err;

        double ped_mean = 0.0, ped_err = 0.0;
        TF1* ped_gauss = new TF1("ped_gauss", "gaus", MODULE_PED_MIN, MODULE_PED_MAX);
        bool ped_fit_success = false;
        int fit_attempt = 0;
        int ped_fit_status = -1;

        std::vector<std::pair<double, double>> fit_ranges = {
            {MODULE_PED_MIN, MODULE_PED_MAX},
            {0.0, 1000.0}, 
            {200.0, 600.0}
        };

        while (fit_attempt < 3 && !ped_fit_success) {
            double current_min = fit_ranges[fit_attempt].first;
            double current_max = fit_ranges[fit_attempt].second;
            
            ped_gauss->SetRange(current_min, current_max);
            
            if (fit_attempt < 2) {
                ped_fit_status = module_data.calc_pedestal_hist->Fit(ped_gauss, "QNR");
            } else if (fit_attempt == 2) {
                ped_fit_status = module_data.calc_pedestal_hist->Fit(ped_gauss, "QNRE");
            }
            
            ped_fit_success = (ped_fit_status == 0);
            
            if (ped_fit_success) {
                ped_mean = ped_gauss->GetParameter(1);
                ped_err = ped_gauss->GetParError(1);
                double sigma = ped_gauss->GetParameter(2);
                
                if (ped_mean < current_min - 10 || ped_mean > current_max + 10 ||
                    ped_err < 0 || std::isnan(ped_mean) || std::isnan(ped_err) ) {
                    ped_fit_success = false;
                }
            }
            
            fit_attempt++;
        }

        if (!ped_fit_success) {
            std::cerr << "[OnlineCalc] Module " << module_name << " Ped fit failed after 3 attempts (all ranges failed)" << std::endl;
            info.ped_mean = -1.0;
            info.ped_err = -1.0;
            info.net_signal = -1.0;
            info.net_signal_err = -1.0;
            info.gain1 = info.gain2 = info.gain3 = 0.0;
            info.gain1_err = info.gain2_err = info.gain3_err = 0.0;
            delete ped_gauss;
            if (module_type == 'G') g_modules.push_back(info);
            else if (module_type == 'W') w_modules.push_back(info);
            continue;
        }

        delete ped_gauss;

        info.ped_mean = ped_mean;
        info.ped_err = ped_err;

        info.net_signal = lms_mean - ped_mean;
        info.net_signal_err = sqrt(pow(lms_err, 2) + pow(ped_err, 2));
        if (info.net_signal <= 0) {
            std::cerr << "[OnlineCalc] Module " << module_name << " real LMS signal (LMS - Ped) <= 0 (=" << info.net_signal << ")" << std::endl;
            info.gain1 = info.gain2 = info.gain3 = 0.0;
            info.gain1_err = info.gain2_err = info.gain3_err = 0.0;
            if (module_type == 'G') g_modules.push_back(info);
            else if (module_type == 'W') w_modules.push_back(info);
            continue;
        }

        if (refpmt_params.size() >= 1 && refpmt_params[0].valid && refpmt_params[0].lms_signal != 0) {
            info.gain1 = info.net_signal * refpmt_params[0].alpha_signal / refpmt_params[0].lms_signal;
            double rel_err_net = info.net_signal_err / info.net_signal;
            double rel_err_alpha = refpmt_params[0].alpha_error / refpmt_params[0].alpha_signal;
            double rel_err_lms = refpmt_params[0].lms_error / refpmt_params[0].lms_signal;
            info.gain1_err = info.gain1 * sqrt(pow(rel_err_net, 2) + pow(rel_err_alpha, 2) + pow(rel_err_lms, 2));
        } else {
            info.gain1 = 0.0;
            info.gain1_err = 0.0;
        }

        if (refpmt_params.size() >= 2 && refpmt_params[1].valid && refpmt_params[1].lms_signal != 0) {
            info.gain2 = info.net_signal * refpmt_params[1].alpha_signal / refpmt_params[1].lms_signal;
            double rel_err_net = info.net_signal_err / info.net_signal;
            double rel_err_alpha = refpmt_params[1].alpha_error / refpmt_params[1].alpha_signal;
            double rel_err_lms = refpmt_params[1].lms_error / refpmt_params[1].lms_signal;
            info.gain2_err = info.gain2 * sqrt(pow(rel_err_net, 2) + pow(rel_err_alpha, 2) + pow(rel_err_lms, 2));
        } else {
            info.gain2 = 0.0;
            info.gain2_err = 0.0;
        }

        if (refpmt_params.size() >= 3 && refpmt_params[2].valid && refpmt_params[2].lms_signal != 0) {
            info.gain3 = info.net_signal * refpmt_params[2].alpha_signal / refpmt_params[2].lms_signal;
            double rel_err_net = info.net_signal_err / info.net_signal;
            double rel_err_alpha = refpmt_params[2].alpha_error / refpmt_params[2].alpha_signal;
            double rel_err_lms = refpmt_params[2].lms_error / refpmt_params[2].lms_signal;
            info.gain3_err = info.gain3 * sqrt(pow(rel_err_net, 2) + pow(rel_err_alpha, 2) + pow(rel_err_lms, 2));
        } else {
            info.gain3 = 0.0;
            info.gain3_err = 0.0;
        }

        if (module_type == 'G') {
            g_modules.push_back(info);
        } else if (module_type == 'W') {
            w_modules.push_back(info);
        }
    }

    std::sort(g_modules.begin(), g_modules.end(), 
              [](const OnlineModuleInfo& a, const OnlineModuleInfo& b) {
                  return a.number < b.number;
              });
    std::sort(w_modules.begin(), w_modules.end(), 
              [](const OnlineModuleInfo& a, const OnlineModuleInfo& b) {
                  return a.number < b.number;
              });

    std::vector<OnlineModuleInfo> all_modules;
    all_modules.insert(all_modules.end(), g_modules.begin(), g_modules.end());
    all_modules.insert(all_modules.end(), w_modules.begin(), w_modules.end());

    return all_modules;
}

int OnlineRefPMTCalculator::GetMaxFileIndex(const std::string& output_dir, const std::string& base_name)
{
    int max_index = 0;
    QDir dir(QString::fromStdString(output_dir));
    if (!dir.exists()) {
        return max_index;
    }

    QString regex_pattern = QString("^%1_(\\d+)\\.txt$").arg(QString::fromStdString(base_name));
    QRegExp regex(regex_pattern);
    regex.setCaseSensitivity(Qt::CaseInsensitive);
    regex.setPatternSyntax(QRegExp::RegExp);

    QStringList file_list = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    for (const QString& file : file_list) {
        if (regex.exactMatch(file)) {
            QString index_str = regex.cap(1);
            bool ok;
            int index = index_str.toInt(&ok);
            if (ok && index > max_index) {
                max_index = index;
            }
        }
    }

    return max_index;
}

void OnlineRefPMTCalculator::SaveFitResult(const std::vector<RefPMTLMSData>& refpmt_records, 
                                           const std::vector<OnlineModuleInfo>& module_info_list,
                                           const std::string& fit_start_utc,
                                           const std::string& fit_end_utc,
                                           const std::string& fit_start_edt,
                                           const std::string& fit_end_edt,
                                           double avg_time_sec,
                                           const std::string& avg_time_str)
{
    if (refpmt_records.empty() && module_info_list.empty()) {
        std::cerr << "[OnlineCalc] No valid data to save!" << std::endl;
        return;
    }

    int run_num = 0;
    if (!refpmt_records.empty()) {
        run_num = refpmt_records[0].run_number;
    } else if (!module_info_list.empty()) {
        run_num = GetCurrentRunNumber();
    }

    std::string date_str = GetCurrentDateString();
    std::string run_str = (run_num != 0) ? std::to_string(run_num) : "online";
    std::string base_filename = "online_calculation_result_" + run_str + "_" + date_str;
    int max_index = GetMaxFileIndex(OUTPUT_DIR, base_filename);
    int new_index = max_index + 1;
    std::string filename = OUTPUT_DIR + "/" + base_filename + "_" + std::to_string(new_index) + ".txt";
    QDir output_dir(QString::fromStdString(OUTPUT_DIR));
    if (!output_dir.exists()) {
        std::cout << "[OnlineCalc] Directory " << OUTPUT_DIR << " not found, creating..." << std::endl;
        if (!output_dir.mkpath(".")) {
            std::cerr << "[OnlineCalc] Failed to create directory: " << OUTPUT_DIR << std::endl;
            return;
        }
    }

    std::ofstream output_file(filename);
    if (!output_file.is_open()) {
        std::cerr << "[OnlineCalc] Failed to save file: " << filename << std::endl;
        return;
    }

    double avg_time_edt_sec = avg_time_sec - 14400.0;
    std::string avg_time_edt_str = ConvertTimestampToTimeString(avg_time_edt_sec);
    avg_time_edt_str = avg_time_edt_str.replace(avg_time_edt_str.find("(UTC)"), 5, "(EDT)");

    output_file << "# Online RefPMT LMS + Module Gain Results (Run: " << run_str << ")\n";
    output_file << "# Trigger Condition: " << FIT_TRIGGER_EVENTS << " LMS events per fit\n";
    output_file << "# Fit Event Absolute Time Range:\n";
    output_file << "#   Start Time (UTC): " << fit_start_utc << "\n";
    output_file << "#   End Time (UTC):   " << fit_end_utc << "\n";
    output_file << "#   Start Time (EDT): " << fit_start_edt << "\n";
    output_file << "#   End Time (EDT):   " << fit_end_edt << "\n";
    output_file << "#   Average Time (UTC): " << avg_time_str << "\n";
    output_file << "#   Average Time (seconds since epoch, UTC): " << std::fixed << std::setprecision(6) << avg_time_sec << "\n";
    output_file << "#   Average Time (EDT): " << avg_time_edt_str << "\n";
    output_file << "#   Average Time (seconds since epoch, EDT): " << std::fixed << std::setprecision(6) << avg_time_edt_sec << "\n";
    
    output_file << "# Formula: Module Gain = (Module LMS Mean - Module Pedestal Mean) × RefPMT Alpha Signal / RefPMT LMS Signal\n";
    output_file << "# Reference PMT Params:\n";
    std::vector<ReferencePMTParam> refpmt_params = FitUtils::CalculateRefPMTParams(hycal_sys_);
    for (size_t i = 0; i < 3; ++i) {
        output_file << "# RefPMT" << (i+1) 
                   << ": AlphaSignal=" << refpmt_params[i].alpha_signal 
                   << ", AlphaError=" << refpmt_params[i].alpha_error
                   << ", LMSSignal=" << refpmt_params[i].lms_signal 
                   << ", LMSError=" << refpmt_params[i].lms_error
                   << ", Valid=" << (refpmt_params[i].valid ? "Yes" : "No") << "\n";
    }
    output_file << "# Note: -1.0 = Fit failed / Invalid data\n\n";

    output_file << "# G module\n";
    output_file << "# module name\tLMS_Mean\tLMS_Err\tPedestal_Mean\tPedestal_Err\t"
               << "module signal(LMS-Pedestal)\tsignal error\t"
               << "gain1(RefPMT1)\tgain1 error\tgain2(RefPMT2)\tgain2 error\tgain3(RefPMT3)\tgain3 error\n";
    for (const auto& info : module_info_list) {
        if (info.name[0] != 'G') break;
        output_file << std::fixed << std::setprecision(6)
                    << info.name << "\t"
                    << info.lms_mean << "\t"
                    << info.lms_err << "\t"
                    << info.ped_mean << "\t"
                    << info.ped_err << "\t"
                    << info.net_signal << "\t"
                    << info.net_signal_err << "\t"
                    << info.gain1 << "\t"
                    << info.gain1_err << "\t"
                    << info.gain2 << "\t"
                    << info.gain2_err << "\t"
                    << info.gain3 << "\t"
                    << info.gain3_err << "\n";
    }

    output_file << "\n# W module\n";
    output_file << "# module name\tLMS_Mean\tLMS_Err\tPedestal_Mean\tPedestal_Err\t"
               << "module signal(LMS-Pedestal)\tsignal error\t"
               << "gain1(RefPMT1)\tgain1 error\tgain2(RefPMT2)\tgain2 error\tgain3(RefPMT3)\tgain3 error\n";
    for (const auto& info : module_info_list) {
        if (info.name[0] == 'W') { 
            output_file << std::fixed << std::setprecision(6)
                        << info.name << "\t"
                        << info.lms_mean << "\t"
                        << info.lms_err << "\t"
                        << info.ped_mean << "\t"
                        << info.ped_err << "\t"
                        << info.net_signal << "\t"
                        << info.net_signal_err << "\t"
                        << info.gain1 << "\t"
                        << info.gain1_err << "\t"
                        << info.gain2 << "\t"
                        << info.gain2_err << "\t"
                        << info.gain3 << "\t"
                        << info.gain3_err << "\n";
        }
    }

    output_file.close();
    std::cout << "[OnlineCalc] Result saved to: " << filename << std::endl;
}

void OnlineRefPMTCalculator::FindRefPMTChannels(PRadHyCalSystem* hycal_sys)
{
    if (!hycal_sys) return;

    auto ref_channels = HyCalModule::GetAllREFPMTChannels(hycal_sys);
    int pmt_count = 0;

    for (auto* ref_chan : ref_channels) {
        if (pmt_count >= 3) break;
        if (ref_chan) {
            refpmt_channels_[pmt_count] = ref_chan;
            pmt_count++;
        }
    }

    if (pmt_count < 3) {
        std::cerr << "[OnlineCalc] Warning: Only " << pmt_count << " RefPMT channels found!" << std::endl;
    }
}

void OnlineRefPMTCalculator::CreateCalcHistograms()
{
    for (int i = 0; i < 3; ++i) {
        if (!refpmt_channels_[i]) continue;

        std::string pmt_name = refpmt_channels_[i]->GetName();
        std::string lms_hist_name = "calc_lms_" + pmt_name;
        std::string physics_hist_name = "calc_physics_" + pmt_name;
        std::string lms_hist_title = "RefPMT" + std::to_string(i+1) + " LMS (Online Calc)";
        std::string physics_hist_title = "RefPMT" + std::to_string(i+1) + " Physics (Online Calc)";

        calc_lms_hist_[i] = new TH1F(lms_hist_name.c_str(), lms_hist_title.c_str(),
                                     HIST_BINS, HIST_MIN, HIST_MAX);
        calc_lms_hist_[i]->SetDirectory(nullptr);
        calc_physics_hist_[i] = new TH1F(physics_hist_name.c_str(), physics_hist_title.c_str(),
                                         HIST_BINS, HIST_MIN, HIST_MAX);
        calc_physics_hist_[i]->SetDirectory(nullptr);
    }
}

void OnlineRefPMTCalculator::ValidateTriggerMapping()
{
    for (int i = 0; i < 3; ++i) {
        PRadADCChannel* ref_chan = refpmt_channels_[i];
        if (!ref_chan) continue;
    }
}

std::pair<double, double> OnlineRefPMTCalculator::CalculateLMSignal(int pmt_idx)
{
    if (pmt_idx < 0 || pmt_idx >= 3 || !refpmt_channels_[pmt_idx]) {
        std::cerr << "[OnlineCalc] Invalid RefPMT index: " << pmt_idx << std::endl;
        return {0.0, 0.0};
    }
    std::vector<ReferencePMTParam> ref_params = FitUtils::CalculateRefPMTParams(hycal_sys_);
    if (static_cast<size_t>(pmt_idx) >= ref_params.size() || !ref_params[pmt_idx].valid) {
        std::cerr << "[OnlineCalc] RefPMT" << pmt_idx+1 << " params invalid" << std::endl;
        return {0.0, 0.0};
    }
    return {ref_params[pmt_idx].lms_signal, ref_params[pmt_idx].lms_error};
}

void OnlineRefPMTCalculator::ClearCalcHistograms()
{
    for (int i = 0; i < 3; ++i) {
        if (calc_lms_hist_[i]) {
            calc_lms_hist_[i]->Reset();
        }
        if (calc_physics_hist_[i]) {
            calc_physics_hist_[i]->Reset();
        }
        valid_event_count_[i] = 0;
    }
}

void OnlineRefPMTCalculator::ClearModuleHistograms()
{
    for (auto& module_data : module_data_list_) {
        if (module_data.calc_lms_hist) {
            module_data.calc_lms_hist->Reset();
        }
        if (module_data.calc_pedestal_hist) {
            module_data.calc_pedestal_hist->Reset();
        }
        module_data.valid_lms_count = 0;
    }
}

int OnlineRefPMTCalculator::GetCurrentRunNumber()
{
    if (hycal_sys_) {
        return PRadInfoCenter::GetRunNumber();
    }
    return 0;
}

std::string GetCurrentDateString()
{
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char date_str[20];
    strftime(date_str, sizeof(date_str), "%b_%d_%Y", &timeinfo);
    return std::string(date_str);
}

#endif
