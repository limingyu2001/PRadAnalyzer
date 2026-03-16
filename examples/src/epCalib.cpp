#include "epCalib.h"

#include "ConfigOption.h"
#include "TSystem.h"
#include "TMath.h"
#include "TVector2.h"
#include "TCanvas.h"
#include "TLatex.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>

#include "PRadDSTParser.h"
#include "PRadEvioParser.h"
#include "PRadBenchMark.h"
#include "PRadHyCalSystem.h"
#include "PRadCoordSystem.h"
#include "PRadInfoCenter.h"
#include "PRadBenchMark.h"
#include "PRadCalibConst.h"
#include "PRadHyCalDetector.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH1D.h"
#include "TH2F.h"
#include "TF1.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

#define PROGRESS_COUNT 10000
#define MAXCLUSTER 6

float GetExpectedEnergy(float& x, float& y);
float GetElossIonElectron(float &theta, float& E);

int main(int argc, char * argv [])
{
    // determine input files
    int run[2];
    run[0] = 0;
    run[1] = 999999;
    string in_dir = "/home/liyuan/OL_monitor/data/"; //define your evio data file folder here
    string out_dir = "./";

    ConfigOption conf_opt;
    conf_opt.AddOpt(ConfigOption::arg_require, 'i');
    conf_opt.AddOpt(ConfigOption::arg_require, 'o');
    conf_opt.AddOpt(ConfigOption::help_message, 'h');

    conf_opt.SetDesc("usage: %0 <options> <begin_run> <end_run>");
    conf_opt.SetDesc('i', "input file directory");
    conf_opt.SetDesc('o', "output file directory");
    conf_opt.SetDesc('h', "show instruction");

    if(!conf_opt.ParseArgs(argc, argv) || conf_opt.NbofArgs() != 2) {
        std::cout << conf_opt.GetInstruction() << std::endl;
        return -1;
    }
    for(auto &opt : conf_opt.GetOptions())
    {
        switch(opt.mark)
        {
        case 'o':
            out_dir = opt.var.String();
            break;
        case 'i':
            in_dir = opt.var.String();
            break;
        default:
            std::cout << conf_opt.GetInstruction() << std::endl;
            return -1;
        }
    }

    TFile *f_m[6];
    string out_files[6] = {
        out_dir + "ratio_module_iteration1.root",
        out_dir + "ratio_module_iteration2.root",
        out_dir + "ratio_module_iteration3.root",
        out_dir + "ratio_module_iteration4.root",
        out_dir + "ratio_module_iteration5.root",
        out_dir + "ratio_module_iteration6.root"
    };
    for(int i = 0; i < 6; i++) {
        f_m[i] = new TFile(out_files[i].c_str(), "RECREATE");
    }
    TFile* f_all = new TFile((out_dir + "ratio_all_iterations.root").c_str(), "RECREATE");


    for(int i = 0; i < 2; i++)
    {
        run[i] = conf_opt.GetArgument(i).Int();
    }
    vector<string> inputFiles;
    /*for(int r = run[0]; r <= run[1]; r++) {
        string file = in_dir + "prad_001308.evio" + to_string(r);
        inputFiles.push_back(file);
    }*/
    string test_file = in_dir + "prad_001292_sel.dst";
    inputFiles.push_back(test_file);
    test_file = in_dir + "prad_001291_sel.dst";
    inputFiles.push_back(test_file);
    test_file = in_dir + "prad_001293_sel.dst";
    inputFiles.push_back(test_file);


    TH1F* cluster_E_moduleHist[1156];
    for(int i=0; i<1156; i++) {
        std::string hist_name = "Calibration constant W" + std::to_string(i+1);
        cluster_E_moduleHist[i] = new TH1F(hist_name.c_str(),
                                            hist_name.c_str(),
                                            400, 0., 4.);
    }

    TH1D* ratio_all[6];
    for(int i = 0; i < 6; i++) {
        ratio_all[i] = new TH1D(Form("E_{recon}/E_{expect} of all modules iteration%d", i),
                                Form("E_{recon}/E_{expect} of all modules iteration%d", i),
                                2000, 0.0, 2.0);
    }

    TH2D *module_ratio[2];
    module_ratio[0] = new TH2D("#cbar#bar{E_{recon}} - E_{expect}#cbar #/ E_{expect} before calibration", "#cbar#bar{E_{recon}} - E_{expect}#cbar #/ E_{expect} before calibration", 34, 0, 34, 34, 0, 34);
    module_ratio[1] = new TH2D("#cbar#bar{E_{recon}} - E_{expect}#cbar #/ E_{expect} after calibration", "#cbar#bar{E_{recon}} - E_{expect}#cbar #/ E_{expect} after calibration", 34, 0, 34, 34, 0, 34);

    TCanvas *c1 = new TCanvas("c1", "c1", 1200, 1200);
    TCanvas *c2 = new TCanvas("c2", "c2", 1200, 1200);

    PRadHyCalSystem *hycal_sys = new PRadHyCalSystem("/home/liyuan/OL_monitor/calib_test/config/hycal.conf");
    PRadCoordSystem *coord_sys = new PRadCoordSystem("/home/liyuan/OL_monitor/calib_test/database/coordinates.dat");
    PRadDSTParser *dst_parser = new PRadDSTParser();
    PRadHyCalDetector *hycal = hycal_sys->GetDetector();

    hycal_sys->ChooseRun(inputFiles[0]);
    coord_sys->ChooseCoord(PRadInfoCenter::GetRunNumber());

    //initial calibration constants set to 0.2
    for(auto module : hycal->GetModuleList())
    {   
        PRadCalibConst cal_const = module->GetCalibConst();
        //cal_const.SetCalibConst(cal_const.GetCalibConst()*2.0);
        cal_const.SetCalibConst(0.2);
        module->SetCalibConst(cal_const);
    }
    double correction[1156];
    for(int i=0; i<1156; i++) correction[i] = 1.0;
    for(int i = 0; i < 2+1; i++) {
        cout << "Starting iteration " << i+1 << " ..." << endl;
        ofstream outf(Form("/home/liyuan/OL_monitor/calib_test/calibration_constants_iteration%d.dat", i));
        for(auto module : hycal->GetModuleList()){
            int id = module->GetID();
            if(id <= 1000) continue; //only calibrate W modules
            id -= 1000;
            if(id == 561 || id == 562 || id == 595 || id == 596) continue; //skip blank channels
            PRadCalibConst cal_const = module->GetCalibConst();
            cal_const.SetCalibConst( cal_const.GetCalibConst() * correction[id-1] );
            module->SetCalibConst(cal_const);
            outf << std::setw(8) << module->GetName()
                << cal_const
                << std::endl;
        }
        outf.close();
        //reset histograms
        for(int m=0; m<1156; m++) cluster_E_moduleHist[m]->Reset();

        //analyze events
        int count = 0;
        PRadBenchMark timer;
        for (auto &file : inputFiles){
            dst_parser->OpenInput(file.c_str());
            cout << "Open input file: " << file << endl;
            while(dst_parser->Read()){
                if(dst_parser->EventType() == PRadDSTParser::Type::event) {
                    auto event = dst_parser->GetEvent();
                    count++;
                    if (count%PROGRESS_COUNT == 0) {
                        cout <<"------[ ev " << count << " ]---"
                            << "---[ " << timer.GetElapsedTimeStr() << " ]---"
                            << "---[ " << timer.GetElapsedTime()/(double)count << " ms/ev ]------"
                            << "\r" << flush;
                    }

                    //reconstruct HyCal clusters
                    hycal_sys->Reconstruct(event);
                    auto &hits = hycal->GetHits();

                    if(hits.size() != 1 || hits.size() == 0) continue;
                    if (hits[0].nblocks <= 3) continue; //some channel has over charge
                    if(hits[0].E < 700.) continue; //too low energy

                    coord_sys->Transform(hycal->GetDetID(), hits.begin(), hits.end());
                    //only consider the PbWO4 modules
                    int module_id = hits[0].cid;
                    if(module_id <= 1000) continue;
                    module_id -= 1000;

                    float ratio = hits[0].E / GetExpectedEnergy(hits[0].x, hits[0].y);

                    cluster_E_moduleHist[module_id-1]->Fill(ratio);
                }
            }
            dst_parser->CloseInput();
            cout <<"------[ ev " << count << " ]---"
                << "---[ " << timer.GetElapsedTimeStr() << " ]---"
                << "---[ " << timer.GetElapsedTime()/(double)count << " ms/ev ]------"
                << endl;
            cout << "Analyzed " << file << "."
                << endl;
        }
        f_m[i]->cd();
        for(int m=0; m<1156; m++) {
            if(m+1==561 || m+1==562 || m+1==595 || m+1==596) continue; //blank channels
            double mean = fitRaito(cluster_E_moduleHist[m]);
            cluster_E_moduleHist[m]->Write();
            ratio_all[i]->Fill(mean);
            correction[m] = 1.0 / mean;
            if(correction[m] < 0 || correction[m] > 100) correction[m] = 1.0;
            cout << correction[m] << " ";

            int module_id = m+1;
            double XthBin = module_id % 34;
            double YthBin = module_id / 34 + 1;
            if(XthBin == 0) {
                XthBin = 34;
                YthBin -= 1;
            }
            if(i==0) module_ratio[0]->SetBinContent(XthBin, YthBin, fabs(mean-1));
            if(i==2) module_ratio[1]->SetBinContent(XthBin, YthBin, fabs(mean-1));
        }
        cout << endl;
        f_all->cd();
        ratio_all[i]->SetTitle("Calibration constant all W modules");
        ratio_all[i]->GetXaxis()->SetTitle("E_meas / E_exp");
        ratio_all[i]->GetYaxis()->SetTitle("Counts");
        ratio_all[i]->Write();

        f_m[i]->cd();
        f_m[i]->Close();
    }
    f_all->cd();
    module_ratio[0]->SetXTitle("X module index");
    module_ratio[0]->SetYTitle("Y module index");
    module_ratio[1]->SetXTitle("X module index");
    module_ratio[1]->SetYTitle("Y module index");
    module_ratio[0]->GetZaxis()->SetRangeUser(0, 0.5);
    module_ratio[1]->GetZaxis()->SetRangeUser(0, 0.5);
    module_ratio[0]->SetStats(0);
    module_ratio[1]->SetStats(0);
    c1->cd();
    module_ratio[0]->Draw("COLZ");
    c2->cd();
    module_ratio[1]->Draw("COLZ");
    for(int m=0; m<1156; m++){
        if(m+1==561 || m+1==562 || m+1==595 || m+1==596) continue; //blank channels
        int module_id = m+1;
        double XthBin = module_id % 34;
        double YthBin = module_id / 34 + 1;
        if(XthBin == 0) {
            XthBin = 34;
            YthBin -= 1;
        }
        TLatex t;
        t.SetTextSize(0.01);
        t.SetTextColor(kBlack);
        c1->cd();
        t.DrawLatex(XthBin-0.7, YthBin-0.6, Form("%d", module_id));
        c2->cd();
        t.DrawLatex(XthBin-0.7, YthBin-0.6, Form("%d", module_id));
    }
    module_ratio[0]->Write();
    module_ratio[1]->Write();
    c1->Write();
    c2->Write();
    f_all->Close();

    return 0;
}

float GetExpectedEnergy(float& x, float& y)
{   
    float Ebeam = 1103.0; //MeV
    float theta = atan(sqrt(x*x + y*y)/5817.);
    float expectE = Ebeam*938.272046 / ( Ebeam*(1.-cos(theta)) + 938.272046 );
    float eLoss = GetElossIonElectron(theta, expectE);
    //eloss->Fill(expectE, eLoss);
    return expectE - eLoss;
}
float GetElossIonElectron(float &theta, float& E)
{
    // Calculates energy loss dE/dx in MeV/mm due to ionization for relativistic electrons/positrons.
    //
    // For formula used, see:
    // Track fitting with energy loss
    // Stampfer, Regler and Fruehwirth
    // Computer Physics Communication 79 (1994), 157-164
    //
    // ZoverA:     atomic number / atomic mass of passed material
    // density:    density of material in g/mm^3
    // I : mean excitation energy in MeV

    //only the Al thin window now, need to add GEM, GEM frame, and the cover between HyCal and GEM to be exact

    float ZoverA[3]   = { 13./27., 10.6/21.8 , 0.49919};
    float density[3]  = {2.699, 0.1117433, 1.205e-3};                      // g/cm^2
    float I[3]        = {166*1.e-6, 106.6e-6, 85.7e-6};                       // MeV
    float de       = 5.0989 * 1.e-25;                 // 4*pi*re*me*c^2 in MeV * mm^2 with re = classical electron radius
    float avogadro = TMath::Na();                     // Avogadro constant in 1/mol
    float me       = 0.5109989181;   // electron mass in MeV/c^2
    float gamma    = E / me;                          // Relativistic gamma-factor.

    // Formula is slightly different for electrons and positrons.
    float gammaFac = 3.;
    float corr     = 1.95;

    float eDep = 0;
    float length[3] = { 0.2, 1.5, 50 };
    for (int i=0; i<3; i++) {
      length[i] /= cos(theta);
      float dedx = 0.5 * de * avogadro * density[i] * ZoverA[i] * (2 * TMath::Log(2*me/I[i]) + gammaFac * TMath::Log(gamma) - corr);
      eDep += dedx*length[i];
    }

    return eDep;//convert from MeV/mm to GeV/m
}
