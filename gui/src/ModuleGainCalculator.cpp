#include "ModuleGainCalculator.h"
#include "PRadEventViewer.h" 
#include "RefPMTConfigDialog.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <iterator> 
#include <QDir>
#include <QGuiApplication>
#include <QScreen>
#include <array>
#include <QMessageBox>
#include "FitUtils.h"        
#include "GainParser.h" 
#include "HyCalModule.h" 
#include "HyCalScene.h"      
#include "PRadADCChannel.h"  
#include "TH1.h"  
#include "HyCalView.h"   

extern RefPMTConfig g_refPMTConfig;

struct ModuleInfo {
    std::string name;
    int number;
    double net_signal;
    double net_signal_error;
    double gain1, gain1_err;
    double gain2, gain2_err;
    double gain3, gain3_err;
    long long lms_entries;
    double cumulative_time;
};

std::vector<ReferencePMTParam> ModuleGainCalculator::CalculateRefPMTParams(PRadHyCalSystem* hycal_sys)
{
    if (!hycal_sys) {
        std::cerr << "[ModuleGainCalculator] Error: PRadHyCalSystem is null!" << std::endl;
        return {};
    }
    return FitUtils::CalculateRefPMTParams(hycal_sys);
}

std::string ModuleGainCalculator::GetCurrentRunNumber(const std::string& filePath)
{
    std::string runNum = GainParser::GetCurrentRunNumber(filePath);
    if (runNum.empty()) {
        std::cerr << "[ModuleGainCalculator] Warning: Failed to get run number from " << filePath << std::endl;
    }
    return runNum;
}

void ModuleGainCalculator::showParentCenteredMessageBox(
    QMessageBox::Icon icon,
    const QString &title,
    const QString &text
)
{
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setIcon(icon);
    msgBox.addButton(QMessageBox::Ok);

    if (parent && parent->isVisible()) {
        QRect parentRect = parent->geometry();
        QPoint parentCenter = parentRect.center();

        QRect msgBoxRect = msgBox.geometry();
        int targetX = parentCenter.x() - msgBoxRect.width() / 2;
        int targetY = parentCenter.y() - msgBoxRect.height() / 2;

        msgBox.move(targetX, targetY);
    } else {
        QScreen *primaryScreen = QGuiApplication::primaryScreen();
        if (primaryScreen) {
            QRect screenRect = primaryScreen->geometry();
            QPoint screenCenter = screenRect.center();

            QRect msgBoxRect = msgBox.geometry();
            int targetX = screenCenter.x() - msgBoxRect.width() / 2;
            int targetY = screenCenter.y() - msgBoxRect.height() / 2;

            msgBox.move(targetX, targetY);
        }
    }

    msgBox.exec();
}

double ModuleGainCalculator::CalculateTotalCumulativeTime(
    const std::string& gainOutputDir,
    double currRunTimeHours
)
{
    double maxHistoryCumulative = 0.0;
    QDir gainDir(QString::fromStdString(gainOutputDir));

    if (!gainDir.exists()) {
        std::cerr << "[ModuleGainCalculator] Info: Gain directory " << gainOutputDir << " not found, use current run time." << std::endl;
        return currRunTimeHours;
    }

    QStringList nameFilters = { "run_*_gains.txt" };
    QStringList gainFileList = gainDir.entryList(nameFilters, QDir::Files);

    for (const QString& fileName : gainFileList) {
        std::string fullPath = gainOutputDir + "/" + fileName.toStdString();
        std::ifstream histFile(fullPath);
        if (!histFile.is_open()) {
            std::cerr << "[ModuleGainCalculator] Warning: Failed to open history file " << fullPath << std::endl;
            continue;
        }

        std::string line;
        bool foundCumulative = false;
        while (std::getline(histFile, line)) {
            if (line.find("# Cumulative Time:") != std::string::npos) {
                size_t colonPos = line.find(":");
                size_t hourPos = line.find("hours");
                if (colonPos != std::string::npos && hourPos != std::string::npos) {
                    try {
                        std::string timeStr = line.substr(colonPos + 1, hourPos - colonPos - 1);
                        double histCumulative = std::stod(timeStr);
                        if (histCumulative > maxHistoryCumulative) {
                            maxHistoryCumulative = histCumulative;
                        }
                    } catch (...) {
                        // std::cerr << "[ModuleGainCalculator] Warning: Invalid cumulative time in " << fullPath << std::endl;
                    }
                }
                foundCumulative = true;
                break;
            }
        }
        histFile.close();
        if (!foundCumulative) {
            // std::cerr << "[ModuleGainCalculator] Warning: No cumulative time in " << fullPath << std::endl;
        }
    }

    return maxHistoryCumulative + currRunTimeHours;
}

void ModuleGainCalculator::MarkAbnormalModules(
    const std::array<std::map<std::string, double>, 3>& currentRunGains,
    const std::map<std::string, std::vector<ModuleGainData>>& moduleGainHistory,
    std::array<std::map<std::string, double>, 3>& lastRunGains,
    void* HyCal
)
{
    if (!HyCal) return;
    HyCalScene* hycalScene = static_cast<HyCalScene*>(HyCal);

    int pmtIndex = g_refPMTConfig.selectedPMT;
    if (pmtIndex < 0) {
        return;
    }
    if (pmtIndex >= 3) {
        pmtIndex = 0;
    }

    bool isFirstRun = lastRunGains[pmtIndex].empty();
    std::map<std::string, double> prevLastRunGains = lastRunGains[pmtIndex];

    if (isFirstRun) {
        lastRunGains[0].insert(currentRunGains[0].begin(), currentRunGains[0].end());
        lastRunGains[1].insert(currentRunGains[1].begin(), currentRunGains[1].end());
        lastRunGains[2].insert(currentRunGains[2].begin(), currentRunGains[2].end());
        return;
    }

    hycalScene->clearAllAbnormalMarks();

    for (const auto& entry : moduleGainHistory) {
        const std::string& moduleName = entry.first;
        const std::vector<ModuleGainData>& gainHistory = entry.second;
        if (gainHistory.empty()) continue;

        auto currGainIt = currentRunGains[pmtIndex].find(moduleName);
        if (currGainIt == currentRunGains[pmtIndex].end()) continue;
        double currentGain = currGainIt->second;

        auto prevGainIt = prevLastRunGains.find(moduleName);
        if (prevGainIt == prevLastRunGains.end()) continue;
        double lastGain = prevGainIt->second;

        bool isAbnormal = false;
        if (lastGain == 0) {
            isAbnormal = (currentGain != 0);
        } else {
            double changeRatio = fabs(currentGain - lastGain) / lastGain;
            isAbnormal = (changeRatio > g_refPMTConfig.threshold);
        }

        if (isAbnormal) {
            hycalScene->markAbnormalModule(moduleName, isAbnormal, pmtIndex);
        }
    }

    lastRunGains[0].clear();
    lastRunGains[0].insert(currentRunGains[0].begin(), currentRunGains[0].end());
    lastRunGains[1].clear();
    lastRunGains[1].insert(currentRunGains[1].begin(), currentRunGains[1].end());
    lastRunGains[2].clear();
    lastRunGains[2].insert(currentRunGains[2].begin(), currentRunGains[2].end());
}

bool ModuleGainCalculator::CalculateAndSave(
    PRadHyCalSystem* hycal_sys,
    const std::string& gainOutputDir,
    const std::string& fileName,
    std::map<std::string, std::vector<ModuleGainData>>& moduleGainHistory,
    std::array<std::map<std::string, double>, 3>& lastRunGains,
    void* HyCal
)
{

    std::vector<ReferencePMTParam> refPMTParams = CalculateRefPMTParams(hycal_sys);
    bool hasValidPMT = false;
    for (const auto& param : refPMTParams) {
        if (param.valid) {
            hasValidPMT = true;
            break;
        }
    }
    if (!hasValidPMT) {
        showParentCenteredMessageBox(
            QMessageBox::Warning,
            "Warning",
            "No valid Reference PMT params!"
        );
        return false;
    }

    const double lmsSampleRate = 1.0;
    double currRunTimeHours = 0.0;
    long long totalLmsEntries = 0;

    const std::vector<PRadADCChannel*>& allChannels = hycal_sys->GetADCList();
    PRadADCChannel* refChannel = nullptr;
    for (auto* channel : allChannels) {
        if (!channel) continue;
        std::string moduleName = channel->GetName();
        if (moduleName.find("LMS") == std::string::npos && (moduleName[0] == 'G' || moduleName[0] == 'W')) {
            refChannel = channel;
            break;
        }
    }

    if (refChannel) {
        TH1* lmsHist = refChannel->GetHist("LMS");
        if (lmsHist) {
            totalLmsEntries = lmsHist->GetEntries();
            double runTimeSeconds = totalLmsEntries / lmsSampleRate;
            currRunTimeHours = runTimeSeconds / 3600.0; 
        }
    } else {
        showParentCenteredMessageBox(
            QMessageBox::Warning,
            "Warning",
            "No valid ADC channel found to calculate run time!"
        );
        currRunTimeHours = 0.0;
    }

    double totalCumulativeTime = CalculateTotalCumulativeTime(gainOutputDir, currRunTimeHours);

    std::string runNumber = GetCurrentRunNumber(fileName);
    if (runNumber.empty()) {
        showParentCenteredMessageBox(
            QMessageBox::Warning,
            "Warning",
            "Failed to get Run Number!"
        );
        return false;
    }

    QDir gainDir(QString::fromStdString(gainOutputDir));
    if (!gainDir.exists()) {
        if (!gainDir.mkpath(".")) {
            showParentCenteredMessageBox(
                QMessageBox::Critical,
                "Error",
                "Failed to create directory: " + QString::fromStdString(gainOutputDir)
            );
        }
    }

    std::string outputFileName = gainOutputDir + "/run_" + runNumber + "_gains.txt";
    std::ofstream outputFile(outputFileName.c_str());
    if (!outputFile.is_open()) {
        showParentCenteredMessageBox(
            QMessageBox::Critical,
            "Error",
            "Failed to create gain file: " + QString::fromStdString(outputFileName)
        );
        return false;
    }

    outputFile << "# Module Gain Results (Run: " << runNumber << ")\n";
    outputFile << "# LMS Entries: " << totalLmsEntries << ", Sample Rate: " << std::fixed << std::setprecision(2) << lmsSampleRate << " Hz\n";
    outputFile << "# Current Run Time: " << std::fixed << std::setprecision(2) << currRunTimeHours << " hours \n";
    outputFile << "# Cumulative Time: " << std::fixed << std::setprecision(2) << totalCumulativeTime << " hours (total time up to this run)\n";
    outputFile << "# Formula: Gain = (Module LMS Mean - Module Pedestal Mean) × RefPMT Alpha Signal / RefPMT LMS Signal\n";
    outputFile << "# Reference PMT Params:\n";
    for (int i = 0; i < 3; ++i) {
        outputFile << "# RefPMT" << (i+1) 
                   << ": AlphaSignal=" << refPMTParams[i].alpha_signal 
                   << ", AlphaError=" << refPMTParams[i].alpha_error
                   << ", LMSSignal=" << refPMTParams[i].lms_signal 
                   << ", LMSError=" << refPMTParams[i].lms_error
                   << ", Valid=" << (refPMTParams[i].valid ? "Yes" : "No") << "\n";
    }

    std::vector<ModuleInfo> g_modules;
    std::vector<ModuleInfo> w_modules;

    for (auto* channel : allChannels) {
        if (!channel) continue;
        std::string moduleName = channel->GetName();
        if (moduleName.find("LMS") != std::string::npos) continue;

        char module_type = moduleName[0];
        //int module_number = HyCalModule::ExtractModuleNumber(moduleName);
        int module_number = QString::fromStdString(moduleName).mid(1).toInt();
        if (module_number < 0) { 
            std::cerr << "[ModuleGainCalculator] Warning: Invalid module number for " << moduleName << std::endl;
            continue;
        }

        TH1* lmsHist = channel->GetHist("LMS");
        double lmsMean = 0.0, lmsMeanErr = 0.0;
        long long lmsEntries = 0;
        if (lmsHist) {
            auto [mean, err] = FitUtils::GaussianFit(
                lmsHist,
                lmsHist->GetXaxis()->GetXmin(),
                lmsHist->GetXaxis()->GetXmax()
            );
            lmsMean = mean;
            lmsMeanErr = err;
            lmsEntries = lmsHist->GetEntries();
        } else {
            std::cerr << "[ModuleGainCalculator] Warning: No LMS histogram for " << moduleName << std::endl;
            continue;
        }

        TH1* pedHist = channel->GetHist("Pedestal");
        double pedMean = 0.0, pedMeanErr = 0.0;
        if (pedHist) {
            auto [mean, err] = FitUtils::GaussianFit(
                pedHist,
                pedHist->GetXaxis()->GetXmin(),
                pedHist->GetXaxis()->GetXmax()
            );
            pedMean = mean;
            pedMeanErr = err;
        } else {
            std::cerr << "[ModuleGainCalculator] Warning: No Pedestal histogram for " << moduleName << std::endl;
            continue;
        }

        if (lmsMean < 0 || pedMean < 0 || lmsMeanErr < 0 || pedMeanErr < 0) {
            std::cerr << "[ModuleGainCalculator] Warning: Invalid data for " << moduleName << " (LMS/Pedestal < 0)" << std::endl;
            continue;
        }
        double moduleNetSignal = lmsMean - pedMean;
        double moduleNetSignalErr = sqrt(pow(lmsMeanErr, 2) + pow(pedMeanErr, 2));
        // if (moduleNetSignal <= 0) { 
        //     std::cerr << "[ModuleGainCalculator] Warning: Non-positive net signal for " << moduleName << std::endl;
        //     continue;
        // }

        double gain1 = 0.0, gain1Err = 0.0;
        double gain2 = 0.0, gain2Err = 0.0;
        double gain3 = 0.0, gain3Err = 0.0;

        if (refPMTParams[0].valid && refPMTParams[0].lms_signal != 0) {
            gain1 = moduleNetSignal * refPMTParams[0].alpha_signal / refPMTParams[0].lms_signal;
            double relErrNet = moduleNetSignalErr / moduleNetSignal;
            double relErrAlpha = refPMTParams[0].alpha_error / refPMTParams[0].alpha_signal;
            double relErrLms = refPMTParams[0].lms_error / refPMTParams[0].lms_signal;
            double relErrGain = sqrt(pow(relErrNet, 2) + pow(relErrAlpha, 2) + pow(relErrLms, 2));
            gain1Err = gain1 * relErrGain;
        }

        if (refPMTParams[1].valid && refPMTParams[1].lms_signal != 0) {
            gain2 = moduleNetSignal * refPMTParams[1].alpha_signal / refPMTParams[1].lms_signal;
            double relErrNet = moduleNetSignalErr / moduleNetSignal;
            double relErrAlpha = refPMTParams[1].alpha_error / refPMTParams[1].alpha_signal;
            double relErrLms = refPMTParams[1].lms_error / refPMTParams[1].lms_signal;
            double relErrGain = sqrt(pow(relErrNet, 2) + pow(relErrAlpha, 2) + pow(relErrLms, 2));
            gain2Err = gain2 * relErrGain;
        }

        if (refPMTParams[2].valid && refPMTParams[2].lms_signal != 0) {
            gain3 = moduleNetSignal * refPMTParams[2].alpha_signal / refPMTParams[2].lms_signal;
            double relErrNet = moduleNetSignalErr / moduleNetSignal;
            double relErrAlpha = refPMTParams[2].alpha_error / refPMTParams[2].alpha_signal;
            double relErrLms = refPMTParams[2].lms_error / refPMTParams[2].lms_signal;
            double relErrGain = sqrt(pow(relErrNet, 2) + pow(relErrAlpha, 2) + pow(relErrLms, 2));
            gain3Err = gain3 * relErrGain;
        }

        ModuleInfo info;
        info.name = moduleName;
        info.number = module_number;
        info.net_signal = moduleNetSignal;
        info.net_signal_error = moduleNetSignalErr;
        info.gain1 = gain1;
        info.gain1_err = gain1Err;
        info.gain2 = gain2;
        info.gain2_err = gain2Err;
        info.gain3 = gain3;
        info.gain3_err = gain3Err;
        info.lms_entries = lmsEntries;
        info.cumulative_time = totalCumulativeTime;

        if (module_type == 'G') {
            g_modules.push_back(info);
        } else if (module_type == 'W') {
            w_modules.push_back(info);
        } else {
            // std::cerr << "[ModuleGainCalculator] Warning: Unknown module type for " << moduleName << std::endl;
            continue;
        }
    }

    std::sort(g_modules.begin(), g_modules.end(), 
              [](const ModuleInfo& a, const ModuleInfo& b) {
                  return a.number < b.number;
              });
    std::sort(w_modules.begin(), w_modules.end(), 
              [](const ModuleInfo& a, const ModuleInfo& b) {
                  return a.number < b.number;
              });

    outputFile << "# G module\n";
    outputFile << "# module name\tmodule signal(LMS-Pedestal)\tsignal error\t"
               << "gain1(RefPMT1)\tgain1 error\tgain2(RefPMT2)\tgain2 error\tgain3(RefPMT3)\tgain3 error\tLMS entries\n";
    for (const auto& info : g_modules) {
        outputFile << std::fixed << std::setprecision(6)
                   << info.name << "\t" 
                   << info.net_signal << "\t" << info.net_signal_error << "\t"
                   << info.gain1 << "\t" << info.gain1_err << "\t"
                   << info.gain2 << "\t" << info.gain2_err << "\t"
                   << info.gain3 << "\t" << info.gain3_err << "\t"
                   << std::fixed << std::setprecision(0) << info.lms_entries << "\n";
    }

    outputFile << "\n# W module\n";
outputFile << "# module name\tmodule signal(LMS-Pedestal)\tsignal error\t"
           << "gain1(RefPMT1)\tgain1 error\tgain2(RefPMT2)\tgain2 error\tgain3(RefPMT3)\tgain3 error\tLMS entries\n";
    for (const auto& info : w_modules) {
        outputFile << std::fixed << std::setprecision(6)
                << info.name << "\t" 
                << info.net_signal << "\t" << info.net_signal_error << "\t"
                << info.gain1 << "\t" << info.gain1_err << "\t"
                << info.gain2 << "\t" << info.gain2_err << "\t"
                << info.gain3 << "\t" << info.gain3_err << "\t"
                << std::fixed << std::setprecision(0) << info.lms_entries << "\n";
    }

    outputFile.close();

    QString timeInfo;
    if (currRunTimeHours > 0) {
        timeInfo = QString("Run Time: %1 hours\nCumulative Time: %2 hours\n")
                   .arg(currRunTimeHours, 0, 'f', 2)
                   .arg(totalCumulativeTime, 0, 'f', 2);
    } else {
        timeInfo = "Run Time: Failed to calculate\nCumulative Time: N/A\n";
    }
    showParentCenteredMessageBox(
        QMessageBox::Information,
        "Success",
        QString("Gain file saved to:\n%1\n%2")
        .arg(QString::fromStdString(outputFileName))
        .arg(timeInfo)
    );
    
        std::array<std::map<std::string, double>, 3> currentRunGains;

    for (const auto& info : g_modules) {
            currentRunGains[0][info.name] = info.gain1;
            currentRunGains[1][info.name] = info.gain2;
            currentRunGains[2][info.name] = info.gain3;
        }
    for (const auto& info : w_modules) {
            currentRunGains[0][info.name] = info.gain1;
            currentRunGains[1][info.name] = info.gain2;
            currentRunGains[2][info.name] = info.gain3;
        }
    
    for (const auto& info : g_modules) {
        auto& history = moduleGainHistory[info.name];
        ModuleGainData data;
        try {
            data.run_number = std::stoi(runNumber);
        } catch (...) {
            data.run_number = 0;
            std::cerr << "[ModuleGainCalculator] Warning: Invalid run number: " << runNumber << std::endl;
        }
        data.gain1 = info.gain1;
        data.gain1_err = info.gain1_err;
        data.gain2 = info.gain2;
        data.gain2_err = info.gain2_err;
        data.gain3 = info.gain3;
        data.gain3_err = info.gain3_err;
        data.cumulative_time = info.cumulative_time;
        history.push_back(data);
    }
    
    for (const auto& info : w_modules) {
        auto& history = moduleGainHistory[info.name];
        ModuleGainData data;
        try {
            data.run_number = std::stoi(runNumber);
        } catch (...) {
            data.run_number = 0;
            std::cerr << "[ModuleGainCalculator] Warning: Invalid run number: " << runNumber << std::endl;
        }
        data.gain1 = info.gain1;
        data.gain1_err = info.gain1_err;
        data.gain2 = info.gain2;
        data.gain2_err = info.gain2_err;
        data.gain3 = info.gain3;
        data.gain3_err = info.gain3_err;
        data.cumulative_time = info.cumulative_time;
        history.push_back(data);
    }

    MarkAbnormalModules(
        currentRunGains,
        moduleGainHistory,
        lastRunGains,
        HyCal
    );
    if (HyCal) {
        HyCalScene* hycalScene = static_cast<HyCalScene*>(HyCal);
        HyCalView* view = hycalScene->getView();
        if (view) {
            view->update();
        }
    }
    return true;
}