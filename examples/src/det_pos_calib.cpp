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

#include "PRadDataHandler.h"
#include "PRadDSTParser.h"
#include "PRadInfoCenter.h"
#include "PRadBenchMark.h"
#include "PRadEPICSystem.h"
#include "PRadTaggerSystem.h"
#include "PRadHyCalSystem.h"
#include "PRadGEMSystem.h"
#include "PRadCoordSystem.h"
#include "PRadDetMatch.h"

#include <iostream>
#include <string>
#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TF1.h"

using namespace std;

#define PROGRESS_COUNT 10000

PRadEPICSystem *epics;
PRadHyCalSystem *hycal;
PRadGEMSystem *gem;
PRadCoordSystem *coord_sys;
PRadDetMatch *det_match;

//hycal.z = 5642.32 mm

struct DataPoint
{
    float x;
    float y;
    float E;

    DataPoint() {};
    DataPoint(float xi, float yi, float Ei) : x(xi), y(yi), E(Ei) {};
};
typedef pair<DataPoint, DataPoint> MollerEvent;
typedef vector<MollerEvent> MollerData;

void FillMollerEvents(const string &dst_path, MollerData &Hdata, MollerData &G1data, MollerData &G2data);
void CalibrateHyCal(const MollerData &data, TH2D *XYpos, TH1D *Xpos, TH1D *Ypos);
void CalibrateGEM(const MollerData &data, TH2D *XYpos, TH1D *Xpos, TH1D *Ypos);

int main(int argc, char * argv []){
    std::string in_dir = "/home/liyuan/OL_monitor/data/"; //define your dst data file folder here
    std::string out_dir = "./Calib_det_pos_plot";

    ConfigOption conf_opt;
    conf_opt.AddOpt(ConfigOption::arg_require, 'i');
    conf_opt.AddOpt(ConfigOption::arg_require, 'o');
    conf_opt.AddOpt(ConfigOption::help_message, 'h');

    conf_opt.SetDesc("usage: %0 <options> <input_file>");
    conf_opt.SetDesc('i', "input file directory");
    conf_opt.SetDesc('o', "output file directory");
    conf_opt.SetDesc('h', "show instruction");

    if(!conf_opt.ParseArgs(argc, argv) || conf_opt.NbofArgs() != 1) {
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

    std::string file;
    file = conf_opt.GetArgument(0).String();

    PRadBenchMark timer;

    MollerData MollerEvents, GEM1Events, GEM2Events;
    FillMollerEvents(in_dir + file, MollerEvents, GEM1Events, GEM2Events);

    int NumMoller = MollerEvents.size();
    cout << "Total " << NumMoller << " Moller events found." << endl;

    //test histograms
    TH2D *mollerPos = new TH2D("mollerPos", "Moller Center Position", 1000, -500., 500., 1000, -500., 500.);
    TH2D *mollerEvsTheta = new TH2D("mollerEvsTheta", "Moller E vs Theta", 70, 0., 7., 1000, 0., 2000.);

    TH2D *XYpos_hycal = new TH2D("XYpos_hycal", "Beam XY position from Moller", 100, -10, 10, 100, -10, 10);
    TH1D *Xpos_hycal = new TH1D("Xpos_hycal", "Beam X position from Moller", 100, -10, 10);
    TH1D *Ypos_hycal = new TH1D("Ypos_hycal", "Beam Y position from Moller", 100, -10, 10);

    TH2D *XYpos_gem1 = new TH2D("XYpos_gem1", "Beam XY position from Moller on GEM1", 100, -10, 10, 100, -10, 10);
    TH1D *Xpos_gem1 = new TH1D("Xpos_gem1", "Beam X position from Moller on GEM1", 100, -10, 10);
    TH1D *Ypos_gem1 = new TH1D("Ypos_gem1", "Beam Y position from Moller on GEM1", 100, -10, 10);

    TH2D *XYpos_gem2 = new TH2D("XYpos_gem2", "Beam XY position from Moller on GEM2", 100, -10, 10, 100, -10, 10);
    TH1D *Xpos_gem2 = new TH1D("Xpos_gem2", "Beam X position from Moller on GEM2", 100, -10, 10);
    TH1D *Ypos_gem2 = new TH1D("Ypos_gem2", "Beam Y position from Moller on GEM2", 100, -10, 10);

    // fill GEM1 Moller events (HyCal events already filled above)
    //MollerData GEMMollerEvents;
    //FillGEMMollerEvents(in_dir + file, GEMMollerEvents);
    //cout << "Total " << GEMMollerEvents.size() << " GEM1 Moller events found." << endl;

    // calibrate HyCal beam position using stored HyCal Moller events
    CalibrateHyCal(MollerEvents, XYpos_hycal, Xpos_hycal, Ypos_hycal);

    // calibrate GEM1 beam position using stored GEM1 Moller events
    CalibrateGEM(GEM1Events, XYpos_gem1, Xpos_gem1, Ypos_gem1);

    // calibrate GEM2 beam position using stored GEM2 Moller events
    CalibrateGEM(GEM2Events, XYpos_gem2, Xpos_gem2, Ypos_gem2);

    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;
    cout << PRadInfoCenter::GetBeamCharge() << endl;
    cout << PRadInfoCenter::GetLiveTime() << endl;

    TCanvas *c1 = new TCanvas("c1", "Moller Center Position", 800, 600);
    XYpos_hycal->Draw("COLZ");
    c1->SaveAs((out_dir + "/HyCal_position.png").c_str());

    TCanvas *c2 = new TCanvas("c2", "Moller Center Position X Projection", 800, 600);
    double mean = Xpos_hycal->GetMean();
    Xpos_hycal->Fit("gaus", "rq", "", mean-2, mean+2);
    Xpos_hycal->Draw();
    TLatex *latex = new TLatex();
    latex->SetNDC();
    latex->SetTextSize(0.04);
    latex->DrawLatex(0.15, 0.85, Form("HyCal X pos: %.2f mm +- %.2f mm", Xpos_hycal->GetFunction("gaus")->GetParameter(1), Xpos_hycal->GetFunction("gaus")->GetParError(1)));
    c2->SaveAs((out_dir + "/HyCal_position_x.png").c_str());

    TCanvas *c3 = new TCanvas("c3", "Moller Center Position Y Projection", 800, 600);
    mean = Ypos_hycal->GetMean();
    Ypos_hycal->Fit("gaus", "rq", "", mean-2, mean+2);
    Ypos_hycal->Draw();
    latex->DrawLatex(0.15, 0.85, Form("HyCal Y pos: %.2f mm +- %.2f mm", Ypos_hycal->GetFunction("gaus")->GetParameter(1), Ypos_hycal->GetFunction("gaus")->GetParError(1)));
    c3->SaveAs((out_dir + "/HyCal_position_y.png").c_str());

    TCanvas *c4 = new TCanvas("c4", "Moller Center Position", 800, 600);
    XYpos_gem1->Draw("COLZ");
    c4->SaveAs((out_dir + "/GEM1_position.png").c_str());

    TCanvas *c5 = new TCanvas("c5", "Moller Center Position X Projection", 800, 600);
    mean = Xpos_gem1->GetMean();
    Xpos_gem1->Fit("gaus", "rq", "", mean-1, mean+1);
    Xpos_gem1->Draw();
    latex->DrawLatex(0.15, 0.85, Form("GEM1 X pos: %.2f mm +- %.2f mm", Xpos_gem1->GetFunction("gaus")->GetParameter(1), Xpos_gem1->GetFunction("gaus")->GetParError(1)));
    c5->SaveAs((out_dir + "/GEM1_position_x.png").c_str());

    TCanvas *c6 = new TCanvas("c6", "Moller Center Position Y Projection", 800, 600);
    mean = Ypos_gem1->GetMean();
    Ypos_gem1->Fit("gaus", "rq", "", mean-4, mean+4);
    Ypos_gem1->Draw();
    latex->DrawLatex(0.15, 0.85, Form("GEM1 Y pos: %.2f mm +- %.2f mm", Ypos_gem1->GetFunction("gaus")->GetParameter(1), Ypos_gem1->GetFunction("gaus")->GetParError(1)));
    c6->SaveAs((out_dir + "/GEM1_position_y.png").c_str());

    TCanvas *c7 = new TCanvas("c7", "Moller Center Position", 800, 600);
    XYpos_gem2->Draw("COLZ");
    c7->SaveAs((out_dir + "/GEM2_position.png").c_str());

    TCanvas *c8 = new TCanvas("c8", "Moller Center Position X Projection", 800, 600);
    mean = Xpos_gem2->GetMean();
    Xpos_gem2->Fit("gaus", "rq", "", mean-1, mean+1);
    Xpos_gem2->Draw();
    latex->DrawLatex(0.15, 0.85, Form("GEM2 X pos: %.2f mm +- %.2f mm", Xpos_gem2->GetFunction("gaus")->GetParameter(1), Xpos_gem2->GetFunction("gaus")->GetParError(1)));
    c8->SaveAs((out_dir + "/GEM2_position_x.png").c_str());

    TCanvas *c9 = new TCanvas("c9", "Moller Center Position Y Projection", 800, 600);
    mean = Ypos_gem2->GetMean();
    Ypos_gem2->Fit("gaus", "rq", "", mean-4, mean+4);
    Ypos_gem2->Draw();
    latex->DrawLatex(0.15, 0.85, Form("GEM2 Y pos: %.2f mm +- %.2f mm", Ypos_gem2->GetFunction("gaus")->GetParameter(1), Ypos_gem2->GetFunction("gaus")->GetParError(1)));
    c9->SaveAs((out_dir + "/GEM2_position_y.png").c_str());

    return 0;
}

// Find beam position from HyCal Moller events using the line intersection method.
// Consecutive pairs of events define two lines; their cross point estimates the beam position.
void CalibrateHyCal(const MollerData &data, TH2D *XYpos, TH1D *Xpos, TH1D *Ypos)
{
    int count = 0;
    double x1[2], y1[2];
    double x2[2], y2[2];

    for(const auto &ev : data) {
        if(count == 0) {
            x1[0] = ev.first.x;  y1[0] = ev.first.y;
            x1[1] = ev.second.x; y1[1] = ev.second.y;
            count++;
            continue;
        }
        if(count == 1) {
            x2[0] = ev.first.x;  y2[0] = ev.first.y;
            x2[1] = ev.second.x; y2[1] = ev.second.y;
            count++;
        }
        if(count == 2) {
            // two lines: y = ax + b, y = cx + d
            double a = (y1[0] - y1[1]) / (x1[0] - x1[1]);
            double b = y1[0] - a * x1[0];
            double c = (y2[0] - y2[1]) / (x2[0] - x2[1]);
            double d = y2[0] - c * x2[0];
            double x_cross = (d - b) / (a - c);
            double y_cross = a * x_cross + b;

            XYpos->Fill(x_cross, y_cross);
            Xpos->Fill(x_cross);
            Ypos->Fill(y_cross);

            count = 0;
        }
    }
}

// Find beam position from GEM1 Moller events using the same line intersection method.
void CalibrateGEM(const MollerData &data, TH2D *XYpos, TH1D *Xpos, TH1D *Ypos)
{
    int count = 0;
    double x1[2], y1[2];
    double x2[2], y2[2];

    for(const auto &ev : data) {
        if(count == 0) {
            x1[0] = ev.first.x;  y1[0] = ev.first.y;
            x1[1] = ev.second.x; y1[1] = ev.second.y;
            count++;
            continue;
        }
        if(count == 1) {
            x2[0] = ev.first.x;  y2[0] = ev.first.y;
            x2[1] = ev.second.x; y2[1] = ev.second.y;
            count++;
        }
        if(count == 2) {
            // two lines: y = ax + b, y = cx + d
            double a = (y1[0] - y1[1]) / (x1[0] - x1[1]);
            double b = y1[0] - a * x1[0];
            double c = (y2[0] - y2[1]) / (x2[0] - x2[1]);
            double d = y2[0] - c * x2[0];
            double x_cross = (d - b) / (a - c);
            double y_cross = a * x_cross + b;

            XYpos->Fill(x_cross, y_cross);
            Xpos->Fill(x_cross);
            Ypos->Fill(y_cross);

            count = 0;
        }
    }
}

void FillMollerEvents(const string &dst_path, MollerData &Hdata, MollerData &G1data, MollerData &G2data){

    // initialize objects
    string ROOT_project = "/home/liyuan/OL_monitor/PRadAnalyzer/";
    epics = new PRadEPICSystem(ROOT_project + "config/epics_channels.conf");
    gem = new PRadGEMSystem(ROOT_project + "config/gem.conf");
    hycal = new PRadHyCalSystem(ROOT_project + "config/hycal.conf");
    coord_sys = new PRadCoordSystem(ROOT_project + "database/coordinates.dat");
    det_match = new PRadDetMatch(ROOT_project + "config/det_match.conf");

    hycal->ChooseRun(dst_path);
    coord_sys->ChooseCoord(PRadInfoCenter::GetRunNumber());

    PRadDSTParser *dst_parser = new PRadDSTParser();
    dst_parser->OpenInput(dst_path);

    PRadHyCalDetector *hycal_det = hycal->GetDetector();
    PRadGEMDetector *gem_det1 = gem->GetDetector("PRadGEM1");
    PRadGEMDetector *gem_det2 = gem->GetDetector("PRadGEM2");

    int count = 0;
    float beam_energy = 1100.;
    int epicsCount = 0;

    PRadBenchMark local_timer;

    while(dst_parser->Read())
    {   
        if(dst_parser->EventType() == PRadDSTParser::Type::event) {
            auto event = dst_parser->GetEvent();

            // only interested in physics event
            if(!event.is_physics_event())
                continue;
            if(count > 10000000) break;
            if((++count) % PROGRESS_COUNT == 0) {
                cout << "------[ ev " << count << " ]---"
                     << "---[ " << local_timer.GetElapsedTimeStr() << " ]---"
                     << "---[ " << local_timer.GetElapsedTime()/(double)count << " ms/ev ]------"
                     << "\r" << flush;
            }

            // update run information
            PRadInfoCenter::Instance().UpdateInfo(event);

            // reconstruct
            hycal->ChooseEvent(event);
            hycal->Reconstruct();
            gem->Reconstruct(event);

            // get reconstructed clusters
            auto &hycal_hit = hycal_det->GetHits();
            auto &gem1_hit = gem_det1->GetHits();
            auto &gem2_hit = gem_det2->GetHits();

            // coordinates transform, projection
            coord_sys->Transform(PRadDetector::HyCal, hycal_hit.begin(), hycal_hit.end());
            coord_sys->Transform(PRadDetector::PRadGEM1, gem1_hit.begin(), gem1_hit.end());
            coord_sys->Transform(PRadDetector::PRadGEM2, gem2_hit.begin(), gem2_hit.end());

            // project to HyCal surface
            coord_sys->Projection(hycal_hit.begin(), hycal_hit.end());
            coord_sys->Projection(gem1_hit.begin(), gem1_hit.end());
            coord_sys->Projection(gem2_hit.begin(), gem2_hit.end());

            // hits matching, return matched index
            auto matched = det_match->Match(hycal_hit, gem1_hit, gem2_hit);

            // we need clean double arm Moller
            if(matched.size() != 2)
                continue;

            // display matched hycal hits
            const auto &hit1 = matched.at(0);
            const auto &hit2 = matched.at(1);

            // check if energy is good
            if(fabs(hit1.E + hit2.E - beam_energy) >= 5.*beam_energy*0.025/sqrt(beam_energy/1000.))
                continue;

            // two moller points
            DataPoint Hmoller1(hit1.hycal.x, hit1.hycal.y, hit1.hycal.E);
            DataPoint Hmoller2(hit2.hycal.x, hit2.hycal.y, hit2.hycal.E);

            Hdata.push_back(make_pair(Hmoller1, Hmoller2));

            // only store events where both hits have a GEM1 match
            if(hit1.gem1.size() > 0 && hit2.gem1.size() > 0) {
                G1data.push_back(make_pair(
                    DataPoint(hit1.gem1[0].x, hit1.gem1[0].y, 0.),
                    DataPoint(hit2.gem1[0].x, hit2.gem1[0].y, 0.)
                ));
            }

            // only store events where both hits have a GEM2 match
            if(hit1.gem2.size() > 0 && hit2.gem2.size() > 0) {
                G2data.push_back(make_pair(
                    DataPoint(hit1.gem2[0].x, hit1.gem2[0].y, 0.),
                    DataPoint(hit2.gem2[0].x, hit2.gem2[0].y, 0.)
                ));
            }
        }
        else if(dst_parser->EventType() == PRadDSTParser::Type::epics) {
            // save epics into handler, otherwise get epicsvalue won't work
            epics->AddEvent(dst_parser->GetEPICS());
            beam_energy = epics->GetValue("MBSY2C_energy");
            epicsCount++;
            if(epicsCount == 1)
                cout << "Get one epics event show Beam Energy: " << beam_energy << endl;
        }
    }
    cout <<"------[ ev " << count << " ]---"
         << "---[ " << local_timer.GetElapsedTimeStr() << " ]---"
         << "---[ " << local_timer.GetElapsedTime()/(double)count << " ms/ev ]------"
         << endl;
}

