#ifndef REFPMTCONFIGDIALOG_H
#define REFPMTCONFIGDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QGuiApplication>
#include <QScreen> 
struct RefPMTConfig {
    int selectedPMT;
    double threshold;
};

class RefPMTConfigDialog : public QDialog {
    Q_OBJECT

public:
    RefPMTConfigDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Gain Monitoring Configuration");

        setFixedSize(300, 200); 

        QLabel *pmtLabel = new QLabel("Select Reference PMT:");
        pmtCombo = new QComboBox();
        pmtCombo->addItem("RefPMT 1", 0);
        pmtCombo->addItem("RefPMT 2", 1);
        pmtCombo->addItem("RefPMT 3", 2);
        pmtCombo->setCurrentIndex(0);

        QFont font = pmtLabel->font();
        font.setPointSize(10);
        pmtLabel->setFont(font);
        pmtCombo->setFont(font);

        QLabel *thresholdLabel = new QLabel("Gain Change Threshold (0-1):");
        thresholdSpin = new QDoubleSpinBox();
        thresholdSpin->setRange(0.01, 1.0);
        thresholdSpin->setSingleStep(0.01);
        thresholdSpin->setValue(0.2);
        thresholdLabel->setFont(font);
        thresholdSpin->setFont(font);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            Qt::Horizontal, this
        );
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(15);
        layout->addWidget(pmtLabel);
        layout->addWidget(pmtCombo);
        layout->addWidget(thresholdLabel);
        layout->addWidget(thresholdSpin);
        layout->addWidget(buttonBox);
        setLayout(layout);

        moveToCenter();
    }

    RefPMTConfig getConfig() const {
        RefPMTConfig config;
        config.selectedPMT = pmtCombo->currentData().toInt();
        config.threshold = thresholdSpin->value();
        return config;
    }

private:
    QComboBox *pmtCombo;
    QDoubleSpinBox *thresholdSpin;

    void moveToCenter() {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            QRect screenGeometry = screen->geometry();
            int x = (screenGeometry.width() - width()) / 2;
            int y = (screenGeometry.height() - height()) / 2;
            move(x, y);
        }
    }
};

#endif // REFPMTCONFIGDIALOG_H