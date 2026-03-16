//============================================================================//
// Start Qt and root applications                                             //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include <QApplication>
#include <TApplication.h>
#include "PRadEventViewer.h"
#include "RefPMTConfigDialog.h"

RefPMTConfig g_refPMTConfig;

int main(int argc, char *argv[])
{
    //TODO handle the input arguments separately for root and Qt

    // the program needs root
    TApplication *myRootApp = new TApplication("My ROOT Application", &argc, argv);
    myRootApp->SetReturnFromRun(true);

    // Qt application
    QApplication app(argc, argv);

    RefPMTConfigDialog configDialog;
    if (configDialog.exec() == QDialog::Accepted) {
        g_refPMTConfig = configDialog.getConfig();
    } else {
        g_refPMTConfig.selectedPMT = -1;
        g_refPMTConfig.threshold = 0.2;
    }

    // event viewer
    PRadEventViewer* viewer = new PRadEventViewer;

    viewer->show();
    viewer->AutoScale();

    app.connect(viewer, SIGNAL(destroyed()), &app, SLOT(quit()));

    // load style sheet
    QFile File("config/gui_style.qss");
    if(File.open(QFile::ReadOnly)) {
        QString style = QLatin1String(File.readAll());
        app.setStyleSheet(style);
    }

    return app.exec();
}

