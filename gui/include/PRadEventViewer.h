#ifndef PRAD_EVENT_VIEWER_H
#define PRAD_EVENT_VIEWER_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFutureWatcher>
#include <vector>

#include "FitUtils.h"
#include "RefPMTParam.h"
#include "GainParser.h"
#include "GainDataStruct.h"
#include "TGraphErrors.h"
#include "TLegend.h"
#include "TSpectrum.h"
#include "TText.h" 


#define HYCAL_SHIFT -50
#define CARTESIAN_TO_HYCALSCENE(x, y) x+HYCAL_SHIFT, -y

struct OnlineModuleGainData {
    std::string module_name;
    double cumulative_time;
    double gain1;
    double gain1_err;
    double gain2;
    double gain2_err;
    double gain3;
    double gain3_err;
};

struct OnlineRefPMTLMSData {
    double cumulative_time;
    double lms_signal[3];
    double lms_error[3];
};

class TH1D;

class HyCalScene;
class HyCalView;
class HyCalModule;
class Spectrum;
class SpectrumSettingPanel;
class HistCanvas;
class LogsBox;
class PRadDataHandler;
class PRadEPICSystem;
class PRadTaggerSystem;
class PRadHyCalSystem;
class PRadGEMSystem;
class PRadCoordSystem;
class PRadDetMatch;
class ScintillatorScene;
class ScintillatorModule;
class ScintillatorView;

#ifdef RECON_DISPLAY
class ReconSettingPanel;
#endif

#ifdef USE_ONLINE_MODE
class PRadETChannel;
class ETSettingPanel;
class OnlineRefPMTCalculator;
#endif

#ifdef USE_CAEN_HV
class PRadHVSystem;
#endif

QT_BEGIN_NAMESPACE
class QPushButton;
class QComboBox;
class QSpinBox;
class QSlider;
class QString;
class QLabel;
class QSplitter;
class QTreeWidget;
class QTreeWidgetItem;
class QTimer;
class QAction;
QT_END_NAMESPACE

enum HistType {
    EnergyTDCHist,
    ModuleHist,
    TaggerHist,
    //new
    RefPMTLMSHist,
    RefPMTAlphaHist,
    StabilityHist,
    ModuleWaveformHist,

    ClusterReconHist,
    //end new
};

enum DetectorType {
    HyCalDetector,
    ScintillatorDetector
};

enum AnnoType {
    NoAnnotation,
    ShowID,
    ShowDAQ,
    ShowTDC,
};

enum ViewMode {
    EnergyView,
    OccupancyView,
    PedestalView,
    SigmaView,
    CustomView,
    ResolutionView,
    CoinHitMapView,
    HighVoltageView,
    VoltageSetView,
};

enum ViewerStatus {
    NO_INPUT,
    DATA_FILE,
    ONLINE_MODE,
};

struct ScintConfig
{
    QString name;
    double width;
    double height;
    double cx;
    double cy;
};

class PRadEventViewer : public QMainWindow
{
    Q_OBJECT

public:
    PRadEventViewer();
    virtual ~PRadEventViewer();
    ViewMode GetViewMode() {return viewMode;}
    AnnoType GetAnnoType() {return annoType;}
    QColor GetColor(const double &val);
    void UpdateStatusBar(ViewerStatus mode);
    void UpdateStatusInfo();
    void UpdateHistCanvas();
    void SelectModule(HyCalModule* module);
    void AutoScale();
    PRadDataHandler *GetHandler() {return handler;}

signals:
    void currentEventChanged(int evt);

public slots:
    void Refresh();

private slots:
    void handleEventChange(int event);
    void openDataFile();
    void initializeFromFile();
    void openCalibrationFile();
    void openGainFactorFile();
    void openCustomMap();
    void handleRootEvents();
    void saveHistToFile();
    void findPeak();
    void fitPedestal();
    void fitHistogram();
    void correctGainFactor();
    void takeSnapShot();
    void changeHistType(int index);
    void changeAnnoType(int index);
    void changeViewMode(int index);
    void changeSpectrumSetting();
    void changeCurrentEvent(int evt);
    void eraseBufferAction();
    void findEvent();
    void editCustomValueLabel(QTreeWidgetItem* item, int column);
    void switchDetector();

private:
    void initView();
    void setupUI();
    void resolutionHists();
    TH1D* resolutionHistoryHist[1156];
    int resolutionGood(int id);
    void generateSpectrum();
    void generateHyCalModules();
    void generateScalerBoxes();
    void setTDCGroupBox();
    void eraseData();
    void createMainMenu();
    void createControlPanel();
    void createStatusBar();
    void createStatusWindow();
    void setupInfoWindow();
    void updateEventRange();
    void chooseEvent(int index);
    void readEventFromFile(const QString &filepath);
    void readCustomValue(const QString &filepath);
    void onlineUpdate(const size_t &max_events);
    bool onlineSettings();
    void generateScintillatorModules();
    void updateScintillator();
    std::vector<ScintConfig> scintConfigs;
    bool loadScintillatorConfig(const QString& file);
    QMenu *setupFileMenu();
    QMenu *setupCalibMenu();
    QMenu *setupToolMenu();
    QMenu *setupSettingMenu();
    QString getFileName(const QString &title,
                        const QString &dir,
                        const QStringList &filter,
                        const QString &suffix,
                        QFileDialog::AcceptMode mode = QFileDialog::AcceptOpen);
    QStringList getFileNames(const QString &title,
                             const QString &dir,
                             const QStringList &filter,
                             const QString &suffix,
                             QFileDialog::AcceptMode mode = QFileDialog::AcceptOpen,
                             QFileDialog::FileMode fmode = QFileDialog::ExistingFiles);

    std::string prad_root;
    PRadDataHandler *handler;
    PRadEPICSystem *epic_sys;
    PRadTaggerSystem *tagger_sys;
    PRadHyCalSystem *hycal_sys;
    PRadGEMSystem *gem_sys;
    int event_number;
    HistType histType;
    AnnoType annoType;
    ViewMode viewMode;
    DetectorType currentDetector;
    QPushButton *detectorSwitchBtn;

    HyCalModule *selection;
    Spectrum *energySpectrum;
    //GEM *myGEM;
    HyCalScene *HyCal;
    HyCalView *view;
    HistCanvas *histCanvas;
    ScintillatorScene *scintScene;
    ScintillatorView *scintView;

    std::vector<std::unique_ptr<ScintillatorModule>> scintModules;

    QString fileName;

    QSplitter *statusWindow;
    QSplitter *rightPanel;
    QSplitter *mainSplitter;

    QWidget *controlPanel;
    QSpinBox *eventSpin;
    QLabel *eventCntLabel;
    QComboBox *histTypeBox;
    QComboBox *annoTypeBox;
    QComboBox *viewModeBox;
    QPushButton *spectrumSettingButton;

    QTreeWidget *statusInfoWidget;
    QTreeWidgetItem *statusItem[6];

    QLabel *lStatusLabel;
    QLabel *rStatusLabel;

    QAction *openDataAction;

    QFileDialog *fileDialog;
    SpectrumSettingPanel *specSetting;
    LogsBox *logBox;

    QFuture<bool> future;
    QFutureWatcher<void> watcher;

    TH1D *accumulateWaveform;

    //gain monitoring
    std::vector<std::vector<RefPMTLMSData>> refPMTLMSHistory;
    std::map<std::string, std::vector<ModuleGainData>> moduleGainHistory;
    std::vector<PRadADCChannel*> getAllREFPMTChannels();
    std::string gainOutputDir;
    std::map<std::string, std::pair<double, double>> moduleXRange;
    std::array<std::map<std::string, double>, 3> lastRunGains; 
    std::map<std::string, std::vector<ModuleGainData>>* moduleGainHistoryPtr;

        // 稳定性图的返回结果
    struct StabilityPlots {
        // Gain 图：3 条线 + legend
        std::vector<TGraphErrors*> gainGraphs;
        TLegend* gainLegend = nullptr;
        bool hasGainData = false;

        // LMS 图：最多 3 条线 + legend
        std::vector<TGraphErrors*> lmsGraphs;
        TLegend* lmsLegend = nullptr;
        bool hasLMSData = false;
    };

    StabilityPlots BuildStabilityPlots(
        const std::string& moduleName,
        bool is_online_mode,
        std::map<std::string, std::vector<ModuleGainData>>& moduleGainHistory,
        std::vector<std::vector<RefPMTLMSData>>& refPMTLMSHistory);
    
    std::pair<std::map<std::string, std::vector<OnlineModuleGainData>>, std::vector<OnlineRefPMTLMSData>> 
    LoadOnlineGainData(const std::string& dir);

    bool ParseOnlineGainFile(const std::string& filename, 
                         std::map<std::string, std::vector<OnlineModuleGainData>>& moduleGainMap,
                         std::vector<OnlineRefPMTLMSData>& refPMTLMSList);

#ifdef USE_ONLINE_MODE
public:
    void UpdateOnlineInfo();
private slots:
    void initOnlineMode();
    bool connectETClient();
    void startOnlineMode();
    void stopOnlineMode();
    void handleOnlineTimer();
private:
    void setupOnlineMode();
    QMenu *setupOnlineMenu();
    OnlineRefPMTCalculator* online_refpmt_calc_;

    PRadETChannel *etChannel;
    QTimer *onlineTimer;
    ETSettingPanel *etSetting;
    QAction *onlineEnAction;
    QAction *onlineDisAction;
#endif

#ifdef USE_CAEN_HV
signals:
    void HVSystemInitialized();
private slots:
    void connectHVSystem();
    void initHVSystem();
    void disconnectHVSystem();
    void startHVMonitor();
    void saveHVSetting();
    void restoreHVSetting();
private:
    void setupHVSystem(const QString &list_file);
    QMenu *setupHVMenu();

    PRadHVSystem *hvSystem;
    QAction *hvEnableAction;
    QAction *hvDisableAction;
    QAction *hvSaveAction;
    QAction *hvRestoreAction;
#endif

#ifdef RECON_DISPLAY
private slots:
    void setupReconMethods();
    void enableReconstruct();
    void handleClusterChange(int idx);
private:
    void setupReconDisplay();

    PRadCoordSystem *coordSystem;
    PRadDetMatch *detMatch;
    ReconSettingPanel *reconSetting;
    QSpinBox *clusterSpin;
#endif
};

#endif
