#ifndef GAIN_PARSER_H
#define GAIN_PARSER_H

#include <string>
#include <map>
#include <vector>
#include "GainDataStruct.h"

class GainParser
{
public:
    static void CreateOutputDirectory(const std::string &dir);

    static void ParseAllGainFiles(const std::string &dir,
                                 std::map<std::string, std::vector<ModuleGainData>> &moduleGainHistory,
                                 std::vector<std::vector<RefPMTLMSData>> &refPMTLMSHistory);
    static std::string GetCurrentRunNumber(const std::string &filePath);
};

#endif // GAIN_PARSER_H