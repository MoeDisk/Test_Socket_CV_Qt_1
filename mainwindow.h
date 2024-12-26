#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QWidget>
#include <QTcpSocket>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QVector>
#include <QByteArray>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>
#include <QFile>

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSliderValueChanged(int value, const QString &paramName);
    void receiveFrame();

private:
    void createSliderControl(const QString &labelText, const QString &paramName, int minValue, int maxValue, int defaultValue);
    void loadSettings();
    void saveSettings();
    void resetToDefaults();
    void uploadParams();

    int params[11];
    const int defaultParams[11] = {20, 30, 15, 68, 0, 180, 64, 100, 0, 1, 1000};

    QTcpSocket *socket;
    QLabel *videoLabel;
    QVector<QSlider*> sliders;
    QByteArray buffer;
    QCheckBox *autoExposureCheckBox;
    QSlider *exposureSlider;

    int frameSize;
};

#endif // MAINWINDOW_H
