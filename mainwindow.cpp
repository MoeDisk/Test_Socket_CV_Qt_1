#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QDebug>
#include <QDataStream>
#include <QPushButton>

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
      socket(new QTcpSocket(this)),
      videoLabel(new QLabel("Waiting for video...", this)),
      frameSize(0) {

    qDebug() << "SendcamCV - made by wh - Build 202412261";

    qApp->setStyleSheet(R"(
        QWidget {
            background-color: #2f2f2f;
            color: #e0e0e0;
            font-family: "Segoe UI", Arial, sans-serif;
            font-size: 14px;
        }

        QPushButton {
            background-color: #444444;
            color: #ffffff;
            border-radius: 5px;
            padding: 8px 16px;
            border: 1px solid #666666;
        }

        QPushButton:hover {
            background-color: #666666;
            border: 1px solid #999999;
        }

        QPushButton:pressed {
            background-color: #333333;
        }

        QSlider::groove:horizontal {
            border: 1px solid #555555;
            height: 8px;
            border-radius: 4px;
            background: #333333;
        }

        QSlider::handle:horizontal {
            background: #888888;
            border: 1px solid #666666;
            width: 14px;
            height: 14px;
            margin: -3px 0;
            border-radius: 7px;
        }

        QLabel {
            color: #e0e0e0;
        }

        QCheckBox {
            spacing: 5px;
            color: #e0e0e0;
        }

        QCheckBox::indicator {
            width: 16px;
            height: 16px;
        }

        QCheckBox::indicator:unchecked {
            border: 1px solid #666666;
            background-color: #333333;
        }

        QCheckBox::indicator:checked {
            border: 1px solid #666666;
            background-color: #888888;
        }

        QGroupBox {
            border: 1px solid #666666;
            border-radius: 5px;
            margin-top: 10px;
            color: #e0e0e0;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top center;
            padding: 2px 5px;
            color: #e0e0e0;
        }

        QMessageBox {
            background-color: #444444;
            color: #e0e0e0;
        }

        QLineEdit {
            border: 1px solid #555555;
            border-radius: 4px;
            padding: 4px;
            color: #e0e0e0;
            selection-background-color: #888888;
        }

        QTextEdit {
            border: 1px solid #555555;
            border-radius: 4px;
            padding: 4px;
            color: #e0e0e0;
            selection-background-color: #888888;
        }

        QComboBox {
            border: 1px solid #555555;
            border-radius: 4px;
            padding: 4px;
            color: #e0e0e0;
            selection-background-color: #888888;
        }

        QComboBox QAbstractItemView {
            selection-background-color: #888888;
            color: #e0e0e0;
        }

    )");

    socket->connectToHost("192.168.3.81", 12345);
    if (!socket->waitForConnected(5000)) {
        qCritical() << "无法连接到服务器！";
        return;
    }

    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::receiveFrame);

    loadSettings();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    videoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(videoLabel);

    createSliderControl("Brightness", "brightness", 0, 100, params[0]);
    createSliderControl("Contrast", "contrast", 0, 100, params[1]);
    createSliderControl("Gamma", "gamma", 1, 50, params[2]); // 映射为 [0.1, 5.0]，后续处理
    createSliderControl("Saturation", "saturation", 0, 100, params[3]);
    createSliderControl("Hue", "hue", -50, 50, params[4]);
    createSliderControl("Rotation Angle", "rotation", 0, 360, params[5]);
    createSliderControl("Backlight", "backlight", 0, 100, params[6]); // 逆光
    createSliderControl("Gain", "gain", 0, 100, params[7]); // 增益
    createSliderControl("Focus", "focus", 0, 100, params[8]); // 焦距

    QPushButton *saveButton = new QPushButton("Save",this);
    connect(saveButton, &QPushButton::clicked,this,&MainWindow::saveSettings);
    mainLayout->addWidget(saveButton);

    QPushButton *resetButton = new QPushButton("Restore",this);
    connect(resetButton, &QPushButton::clicked,this,&MainWindow::resetToDefaults);
    mainLayout->addWidget(resetButton);

    QCheckBox *autoExposureCheckBox = new QCheckBox("Auto Exposure", this);
    autoExposureCheckBox->setChecked(params[9]);
    mainLayout->addWidget(autoExposureCheckBox);

    connect(autoExposureCheckBox, &QCheckBox::stateChanged, this, [=](int state) {
        params[9] = (state == Qt::Checked) ? 1 : 0;
        uploadParams();
        exposureSlider->setEnabled(state != Qt::Checked);
    });

    QLabel *authorLabel = new QLabel("SendcamCV - made by wh - Build 202412261", this);
    authorLabel->setObjectName("authorLabel");
    mainLayout->addWidget(authorLabel);

    createSliderControl("Exposure", "exposure", 0, 1000, params[10]);
    exposureSlider = sliders.last();

    setLayout(mainLayout);

}

MainWindow::~MainWindow() {
    delete socket;
    delete videoLabel;
}

void MainWindow::createSliderControl(const QString &labelText, const QString &paramName, int minValue, int maxValue, int defaultValue) {
    QHBoxLayout *layout = new QHBoxLayout;

    QLabel *label = new QLabel(labelText, this);
    layout->addWidget(label);

    QSlider *slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(minValue, maxValue);
    slider->setValue(defaultValue);
    layout->addWidget(slider);
    sliders.append(slider);

    QLabel *valueLabel = new QLabel(QString::number(defaultValue), this);
    layout->addWidget(valueLabel);

    connect(slider, &QSlider::valueChanged, this, [=](int value) {
        if (paramName == "gamma") {
            double gammaValue = value / 10.0;
            valueLabel->setText(QString::number(gammaValue, 'f', 1));
        } else {
            valueLabel->setText(QString::number(value));
        }
    });

    connect(slider, &QSlider::sliderReleased, this, [=]() {
        if (paramName == "gamma") {
            double sendValue = slider->value() / 10.0;
            onSliderValueChanged(sendValue * 10, paramName);
        } else {
            onSliderValueChanged(slider->value(), paramName);
        }
    });

    this->layout()->addItem(layout);
}

void MainWindow::onSliderValueChanged(int value, const QString &paramName) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "未连接到服务器，无法发送参数";
        return;
    }

    if (paramName == "brightness") params[0] = value;
    else if (paramName == "contrast") params[1] = value;
    else if (paramName == "gamma") {
        double gammaValue = value / 10.0;
        params[2] = static_cast<int>(gammaValue * 10);
    }
    else if (paramName == "saturation") params[3] = value;
    else if (paramName == "hue") params[4] = value;
    else if (paramName == "rotation") params[5] = value;
    else if (paramName == "backlight") params[6] = value;
    else if (paramName == "gain") params[7] = value;
    else if (paramName == "focus") params[8] = value;
    else if (paramName == "exposure") params[10] = value;

    if (socket->write(reinterpret_cast<char*>(params), sizeof(params)) == -1) {
        qWarning() << "发送参数失败：" << socket->errorString();
    } else {
        socket->flush();
    }
}

void MainWindow::saveSettings(){
    QSettings settings("config.ini",QSettings::IniFormat);
    for(int i=0;i<sliders.size();++i){
        settings.setValue(QString("param%1").arg(i),sliders[i]->value());
    }

    settings.setValue("param9", params[9]);
        settings.setValue("param10", params[10]);
    QMessageBox::information(this,"Save","OK");
}

void MainWindow::loadSettings(){
    QSettings settings("config.ini",QSettings::IniFormat);
    if (!QFile::exists("config.ini")) {
            qWarning() << "配置文件 config.ini 未找到，使用默认值!";
            for (int i = 0; i < 11; ++i) {
                params[i] = defaultParams[i];  // 使用默认值
                qDebug() << "默认参数 [" << i << "] = " << params[i];
            }
        } else {
        for(int i=0;i<11;++i){
        params[i]=settings.value(QString("param%1").arg(i),params[i]).toInt();
        qDebug() << params[i] << endl;
        }
    }
    uploadParams();
}

void MainWindow::resetToDefaults(){
    for(int i=0;i<9;++i){
        sliders[i]->setValue(defaultParams[i]);
        params[i]=defaultParams[i];
        qDebug() << params[i] << endl;
    }
    sliders[9]->setValue(1000);
    uploadParams();

    QMessageBox::information(this,"Restore","OK");
}

void MainWindow::uploadParams(){
    if (socket->state() != QAbstractSocket::ConnectedState) {
            qWarning() << "未连接到服务器，无法发送参数";
            return;
        }
        // 直接发送参数数组
        if (socket->write(reinterpret_cast<char*>(params), sizeof(params)) == -1) {
            qWarning() << "发送参数失败：" << socket->errorString();
        } else {
            socket->flush();
            //qDebug() << "发送所有参数：" << QByteArray(reinterpret_cast<char*>(params), sizeof(params)).toHex(' ');
        }
}

void MainWindow::receiveFrame() {
    while (socket->bytesAvailable() > 0) {
        buffer.append(socket->readAll());

        while (buffer.size() >= 4) {
            if (frameSize == 0) {
                frameSize = *reinterpret_cast<const quint32*>(buffer.constData());
                buffer.remove(0, 4);
                //qDebug() << "收到帧大小：" << frameSize;
            }

            if (frameSize > 0 && buffer.size() >= frameSize) {
                QByteArray frameData = buffer.left(frameSize);
                buffer.remove(0, frameSize);
                frameSize = 0;

                QImage image;
                if (image.loadFromData(frameData, "JPEG")) {
                    videoLabel->setPixmap(QPixmap::fromImage(image));
                } else {
                    qWarning() << "解码帧失败。";
                }
            } else {
                break;
            }
        }
    }
}
