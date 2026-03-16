#include "GainParser.h"
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <QDir>
#include <sys/stat.h>
#include <stdexcept>
#include <iostream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#else
#include <dirent.h>
#endif

void GainParser::CreateOutputDirectory(const std::string &dir)
{
#ifdef _WIN32
    if (_mkdir(dir.c_str()) == -1 && errno != EEXIST) {
        throw std::runtime_error("Failed to create directory: " + dir);
    }
#else
    if (mkdir(dir.c_str(), 0755) == -1 && errno != EEXIST) {
        throw std::runtime_error("Failed to create directory: " + dir);
    }
#endif
}

static void ParseGainFile(const std::string &filename,
                         std::map<std::string, std::vector<ModuleGainData>> &moduleGainHistory,
                         std::vector<std::vector<RefPMTLMSData>> &refPMTLMSHistory)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open gain file: " << filename << std::endl;
        return;
    }

    std::string line;
    int runNumber = 0;
    double cumulativeTime = 0.0; 
    RefPMTLMSData refData[3];

    while (std::getline(file, line)) {
        if (line.empty() || line[0] != '#') break;

        if (line.find("Run: ") != std::string::npos) {
            size_t pos = line.find("Run: ") + 5;
            std::string runStr = line.substr(pos);
            size_t endPos = runStr.find(')');
            if (endPos != std::string::npos) runStr = runStr.substr(0, endPos);
            try {
                runNumber = std::stoi(runStr);
            } catch (...) {
                std::cerr << "Warning: Invalid run number in " << filename << std::endl;
                runNumber = 0;
            }
        }

        if (line.find("# Cumulative Time: ") != std::string::npos) {
            size_t timeStart = line.find(": ") + 2;
            size_t timeEnd = line.find(" hours", timeStart);
            if (timeStart != std::string::npos && timeEnd != std::string::npos) {
                std::string timeStr = line.substr(timeStart, timeEnd - timeStart);
                try {
                    cumulativeTime = std::stod(timeStr);
                } catch (...) {
                    std::cerr << "Warning: Invalid cumulative time in " << filename << std::endl;
                    cumulativeTime = 0.0;
                }
            }
        }

        for (int i = 0; i < 3; ++i) {
            std::string pmtTag = "# RefPMT" + std::to_string(i+1);
            if (line.find(pmtTag) != std::string::npos) {
                refData[i].run_number = runNumber;
                refData[i].cumulative_time = cumulativeTime;

                size_t lmsPos = line.find("LMSSignal=") + 10;
                if (lmsPos != std::string::npos) {
                    size_t lmsEnd = line.find(",", lmsPos);
                    if (lmsEnd != std::string::npos) {
                        std::string lmsStr = line.substr(lmsPos, lmsEnd - lmsPos);
                        try {
                            refData[i].lms_signal = std::stod(lmsStr);
                        } catch (...) {
                            std::cerr << "Warning: Invalid LMSSignal for RefPMT" << i+1 << " in " << filename << std::endl;
                        }
                    }
                }

                size_t errPos = line.find("LMSError=") + 9;
                if (errPos != std::string::npos) {
                    size_t errEnd = line.find(",", errPos);
                    if (errEnd != std::string::npos) {
                        std::string errStr = line.substr(errPos, errEnd - errPos);
                        try {
                            refData[i].lms_error = std::stod(errStr);
                        } catch (...) {
                            std::cerr << "Warning: Invalid LMSError for RefPMT" << i+1 << " in " << filename << std::endl;
                        }
                    }
                }
            }
        }
    }

    if (refPMTLMSHistory.size() < 3) {
        refPMTLMSHistory.resize(3);
    }
    for (int i = 0; i < 3; ++i) {
        refPMTLMSHistory[i].push_back(refData[i]);
    }

    bool isGModule = false, isWModule = false;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        if (line.find("# G module") != std::string::npos) {
            isGModule = true;
            isWModule = false;
            continue;
        }
        if (line.find("# W module") != std::string::npos) {
            isGModule = false;
            isWModule = true;
            continue;
        }
        if (line[0] == '#') continue;

        std::istringstream iss(line);
        std::string moduleName;
        double netSignal, netSignalErr;
        double gain1, gain1Err, gain2, gain2Err, gain3, gain3Err;
        long long lmsEntries;

        std::vector<std::string> tokens;
        std::string token;
        while (std::getline(iss, token, '\t')) {
            tokens.push_back(token);
        }
        if (tokens.size() < 10) {
            std::cerr << "Warning: Invalid data line in " << filename << ": " << line << std::endl;
            continue;
        }

        try {
            moduleName = tokens[0];
            netSignal = std::stod(tokens[1]);
            netSignalErr = std::stod(tokens[2]);
            gain1 = std::stod(tokens[3]);
            gain1Err = std::stod(tokens[4]);
            gain2 = std::stod(tokens[5]);
            gain2Err = std::stod(tokens[6]);
            gain3 = std::stod(tokens[7]);
            gain3Err = std::stod(tokens[8]);
            lmsEntries = std::stoll(tokens[9]);
        } catch (...) {
            std::cerr << "Warning: Invalid data in " << filename << ": " << line << std::endl;
            continue;
        }

        ModuleGainData data;
        data.run_number = runNumber;
        data.gain1 = gain1;
        data.gain1_err = gain1Err;
        data.gain2 = gain2;
        data.gain2_err = gain2Err;
        data.gain3 = gain3;
        data.gain3_err = gain3Err;
        data.cumulative_time = cumulativeTime;

        moduleGainHistory[moduleName].push_back(data);
    }

    file.close();
}

void GainParser::ParseAllGainFiles(const std::string &dir,
                                  std::map<std::string, std::vector<ModuleGainData>> &moduleGainHistory,
                                  std::vector<std::vector<RefPMTLMSData>> &refPMTLMSHistory)
{
#ifdef _WIN32
    std::string searchPath = dir + "\\run_*_gains.txt";
    _finddata_t fileInfo;
    intptr_t hFind = _findfirst(searchPath.c_str(), &fileInfo);

    if (hFind == -1) {
        if (errno == ENOENT) {
            return;
        }
        std::cerr << "Warning: Could not open directory: " << dir << std::endl;
        return;
    }

    do {
        if (!(fileInfo.attrib & _A_SUBDIR)) {
            std::string filename = dir + "\\" + fileInfo.name;
            ParseGainFile(filename, moduleGainHistory, refPMTLMSHistory);
        }
    } while (_findnext(hFind, &fileInfo) == 0);

    _findclose(hFind);
#else
    DIR *dp = opendir(dir.c_str());
    if (!dp) {
        std::cerr << "Warning: Could not open directory: " << dir << std::endl;
        return;
    }

    dirent *entry;
    while ((entry = readdir(dp)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.find("run_") == 0 && filename.find("_gains.txt") != std::string::npos) {
            ParseGainFile(dir + "/" + filename, moduleGainHistory, refPMTLMSHistory);
        }
    }
    closedir(dp);
#endif
}

std::string GainParser::GetCurrentRunNumber(const std::string &filePath)
{
    if (filePath.empty()) return "";

    size_t pradPos = filePath.find("prad_00");
    if (pradPos == std::string::npos) return "";

    size_t start = pradPos + 7;  
    size_t end = filePath.find("_mon.dst", start);
    if (end == std::string::npos) return "";

    return filePath.substr(start, end - start);
}