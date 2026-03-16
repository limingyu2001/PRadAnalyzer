//============================================================================//
// An application of replay raw data file and save the replayed data into DST //
// file. This is the 1st-level replay, it only discards the pedestal data     //
//                                                                            //
// Chao Peng                                                                  //
// 10/04/2016                                                                 //
//============================================================================//

#include "PRadDataHandler.h"
#include "PRadEPICSystem.h"
#include "PRadTaggerSystem.h"
#include "PRadHyCalSystem.h"
#include "PRadGEMSystem.h"
#include "PRadInfoCenter.h"
#include "PRadEvioParser.h"
#include "PRadBenchMark.h"
#include "ConfigOption.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <thread>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

void process(const string &input, const int start, const int end, const string &output, bool evio_database, int run);
void combineDST(const string &input_prefix, const int num_parts, const string &output, int run);

int main(int argc, char * argv[])
{
    ConfigOption conf_opt;
    conf_opt.AddLongOpt(ConfigOption::arg_none, "init-evio", 'e');
    conf_opt.AddLongOpt(ConfigOption::arg_none, "init-database", 'd');
    conf_opt.AddOpt(ConfigOption::arg_require, 'r');
    conf_opt.AddOpt(ConfigOption::help_message, 'h');

    conf_opt.SetDesc("usage: %0 <in_file> <out_file> <num_threads>");
    conf_opt.SetDesc('r', "set run number, only valid for --init-database, default -1 (determined from file name).");
    conf_opt.SetDesc('e', "initialize from evio.0 file");
    conf_opt.SetDesc('d', "initialize from database");
    conf_opt.SetDesc('h', "show instruction.");

    if(!conf_opt.ParseArgs(argc, argv) || conf_opt.NbofArgs() != 3) {
        std::cout << conf_opt.GetInstruction() << std::endl;
        return -1;
    }

    bool evio_database = true;

    int run = -1;
    for(auto &opt : conf_opt.GetOptions())
    {
        switch(opt.mark)
        {
        case 'e':
            evio_database = true;
            break;
        case 'd':
            evio_database = false;
            break;
        case 'r':
            run = opt.var.Int();
            break;
        default:
            std::cout << conf_opt.GetInstruction() << std::endl;
            return -1;
        }
    }

    string input = conf_opt.GetArgument(0).String();
    string output = conf_opt.GetArgument(1).String();
    int num_threads = conf_opt.GetArgument(2).Int();

    input = "/home/liyuan/OL_monitor/data/test_evio/prad_001308.evio";
    output = "/home/liyuan/OL_monitor/data/test_evio/";

    PRadBenchMark timer;
    std::cout << "Using multi-thread replay!" << std::endl;

    const int num_process = num_threads > 0 ? num_threads : 1;
    std::vector<pid_t> pids(num_process);

    int num_file = 48;
    num_file = (num_file / num_process) * num_process;
    int part[num_process+1] = {0};

    for(int i = 1; i < num_process + 1; i++){
        part[i] = part[i-1] + num_file/num_process;
    }

    for(int i = 0; i < num_process; i++){
        pid_t pid = fork();
        if(pid < 0){
            std::cerr << "Fork process " << i << " failed!" << std::endl;
            continue;
        }
        else if(pid == 0){
            process(input, part[i]+1, part[i+1], output+to_string(i+1)+"_", evio_database, run);
            exit(0);
        }
        else pids[i] = pid;
    }
    for(int i =0; i < num_process; i++){
        int status;
        waitpid(pids[i], &status, 0);
    }

    PRadHyCalSystem *hycal = new PRadHyCalSystem("/home/liyuan/OL_monitor/calib_test/config/hycal.conf");
    if(run < 0)
        hycal->ChooseRun(input+".0");
    else
        hycal->ChooseRun(run);
    run = PRadInfoCenter::GetRunNumber();
    delete hycal;
    combineDST(output, num_process, output + to_string(run) + ".dst", run);

    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;
    cout << PRadInfoCenter::GetBeamCharge() << endl;
    cout << PRadInfoCenter::GetLiveTime() << endl;

    return 0;
}

void process(const string &input, const int start, const int end, const string &output, bool evio_database, int run){
    PRadDataHandler *handler = new PRadDataHandler();
    PRadEPICSystem *epics = new PRadEPICSystem("/home/liyuan/OL_monitor/calib_test/config/epics_channels.conf");
    PRadHyCalSystem *hycal = new PRadHyCalSystem("/home/liyuan/OL_monitor/calib_test/config/hycal.conf");
    PRadGEMSystem *gem = new PRadGEMSystem("/home/liyuan/OL_monitor/calib_test/config/gem.conf");
    PRadTaggerSystem *tagger = new PRadTaggerSystem;

    handler->SetEPICSystem(epics);
    handler->SetTaggerSystem(tagger);
    handler->SetHyCalSystem(hycal);
    handler->SetGEMSystem(gem);

    if(evio_database) {
        handler->InitializeByData(input + ".0");
    } else {
        if(run < 0)
            hycal->ChooseRun(input+".0");
        else
            hycal->ChooseRun(run);
    }
    run = PRadInfoCenter::GetRunNumber();
    handler->Replay_range(input, start, end, output+to_string(run)+".dst");

    delete handler;
    delete epics;
    delete tagger;
    delete hycal;
    delete gem;
}

void combineDST(const string &input_prefix, const int num_parts, const string &output, int run){
    PRadDSTParser dst_parser;
    dst_parser.OpenOutput(output);

    for(int i = 1; i <= num_parts; i++){
        dst_parser.OpenInput(input_prefix + to_string(i) + "_" + to_string(run) + ".dst");
        while(dst_parser.Read()){
            if(dst_parser.EventType() == PRadDSTParser::Type::event){
                dst_parser.Write(dst_parser.GetEvent());
            }
            else if(dst_parser.EventType() == PRadDSTParser::Type::epics){
                dst_parser.Write(dst_parser.GetEPICS());
            }
        }
        dst_parser.CloseInput();
    }

    dst_parser.CloseOutput();
}

