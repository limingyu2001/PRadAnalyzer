#ifndef MODULE_GAIN_CALCULATOR_H
#define MODULE_GAIN_CALCULATOR_H

#include <vector>
#include <array>
#include <string>
#include <map>
#include <QMessageBox> 
#include "PRadHyCalSystem.h" 
#include "GainDataStruct.h"
#include "RefPMTParam.h" 

struct RefPMTConfig;
extern RefPMTConfig g_refPMTConfig;

class PRadADCChannel;
class PRadEventViewer;
class ModuleGainCalculator
{
public:
    ModuleGainCalculator(PRadEventViewer* parent) : parent(parent) {}
    ~ModuleGainCalculator() = default;
    /**
     * Calculate the module gain and save it to a file, while updating the gain history.
     * @param hycal_sys：HyCal system pointer (used to obtain ADC channel and module information)
     * @param gainOutputDir：Gain file output directory (e.g., "module_gain_results")
     * @param fileName：Current data file path (used to extract run number)
     * @param moduleGainHistory：[Output] Module Gain History Mapping (Global/Member Variables of the Main Program)
     * @param lastRunGains：[Output] Gain from the last run (used to mark abnormal modules, member variables of the main program)
     * @param HyCal：[Output] HyCalScene pointer (used to mark abnormal modules, member variable of the main program)
     * @return：true = calculation successful, false = calculation failed
     */
    bool CalculateAndSave(
        PRadHyCalSystem* hycal_sys,
        const std::string& gainOutputDir,
        const std::string& fileName,
        std::map<std::string, std::vector<ModuleGainData>>& moduleGainHistory,
        std::array<std::map<std::string, double>, 3>& lastRunGains,
        void* HyCal 
    );

private:
    std::vector<ReferencePMTParam> CalculateRefPMTParams(PRadHyCalSystem* hycal_sys);
    std::string GetCurrentRunNumber(const std::string& filePath);
    PRadEventViewer* parent = nullptr; 
    double CalculateTotalCumulativeTime(
        const std::string& gainOutputDir,
        double currRunTimeHours
    );

    void MarkAbnormalModules(
        const std::array<std::map<std::string, double>, 3>& currentRunGains,
        const std::map<std::string, std::vector<ModuleGainData>>& moduleGainHistory,
        std::array<std::map<std::string, double>, 3>& lastRunGains, 
        void* HyCal
    );

    void showParentCenteredMessageBox(QMessageBox::Icon icon, 
        const QString &title, 
        const QString &text);
};

#endif // MODULE_GAIN_CALCULATOR_H