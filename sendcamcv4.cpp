#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <chrono>

using namespace std;
using namespace cv;

#define PORT 12345

Mat adjust_brightness_contrast(const Mat& image, int brightness, int contrast) {
    Mat result;
    image.convertTo(result, -1, 1 + contrast / 100.0, brightness);
    return result;
}

Mat adjust_gamma(const Mat& image, double gamma) {
    Mat result;
    Mat lut(1, 256, CV_8U);
    uchar* ptr = lut.data;
    for (int i = 0; i < 256; i++) {
        ptr[i] = saturate_cast<uchar>(pow(i / 255.0, 1.0 / gamma) * 255.0);
    }
    LUT(image, lut, result);
    return result;
}

Mat adjust_saturation_hue(const Mat& image, int saturation, int hue) {
    Mat hsv;
    cvtColor(image, hsv, COLOR_BGR2HSV);

    vector<Mat> channels;
    split(hsv, channels);

    // 调整色相，范围为[0, 180]
    channels[0].forEach<uchar>([hue](uchar& px, const int* position) -> void {
        px = (px + hue) % 180;
    });

    // 调整饱和度，范围[0, 255]
    channels[1] = min(max(channels[1] + saturation, 0), 255);

    merge(channels, hsv);

    Mat result;
    cvtColor(hsv, result, COLOR_HSV2BGR);
    return result;
}

Mat rotate_image(const Mat& image, double angle) {
    Point2f center(image.cols / 2.0, image.rows / 2.0);
    Mat rotation_matrix = getRotationMatrix2D(center, angle, 1.0);
    Mat rotated_image;
    warpAffine(image, rotated_image, rotation_matrix, image.size());
    return rotated_image;
}

void set_camera_params(VideoCapture& cap, double backlight, double gain, double focus) {
    cap.set(CAP_PROP_BACKLIGHT, backlight);
    cap.set(CAP_PROP_GAIN, gain);
    cap.set(CAP_PROP_FOCUS, focus);
}

void set_exposure(VideoCapture& cap, int auto_exposure, double exposure_value) {
    if (auto_exposure == 1) {
        cap.set(CAP_PROP_AUTO_EXPOSURE, 1);
    } else {
        cap.set(CAP_PROP_AUTO_EXPOSURE, 0);
        cap.set(CAP_PROP_EXPOSURE, exposure_value);
    }
}

int main() {
    cout << "SendcamCV - made by wh - Build 202412261" << endl;
    VideoCapture cap(1, CAP_V4L2);
    if (!cap.isOpened()) {
        cerr << "无法打开摄像头" << endl;
        return -1;
    }

    double backlight_value = 64;
    double gain_value = 100;
    double focus_value = 0; 
    int brightness = 20;
    int contrast = 30;
    double gamma = 1.5;
    int saturation = 68;
    int hue = 0;
    double rotation_angle = 180;
    double exposure_value = -1;
    int auto_exposure = 1;

    set_camera_params(cap, backlight_value, gain_value, focus_value);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        cerr << "创建套接字失败" << endl;
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "绑定地址失败" << endl;
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 5) < 0) {
        cerr << "监听端口失败" << endl;
        close(server_fd);
        return -1;
    }
    cout << "等待客户端连接..." << endl;

    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);

    if (client_fd < 0) {
        cerr << "接受客户端连接失败" << endl;
        close(server_fd);
        return -1;
    }
    cout << "客户端已连接" << endl;

    // 发送初始参数
//     int params[] = {brightness, contrast, static_cast<int>(gamma * 100), saturation, hue,
//                 static_cast<int>(rotation_angle), static_cast<int>(backlight_value),
//                 static_cast<int>(gain_value), static_cast<int>(focus_value),
//                 auto_exposure, static_cast<int>(exposure_value)};
// send(client_fd, params, sizeof(params), 0);


    while (true) {
        int received_params[11];
        if (recv(client_fd, received_params, sizeof(received_params), MSG_DONTWAIT) > 0) {
            brightness = received_params[0];
            contrast = received_params[1];
            gamma = received_params[2] / 10.0;
            saturation = received_params[3];
            hue = received_params[4];
            rotation_angle = received_params[5];
            backlight_value = received_params[6];
            gain_value = received_params[7];
            focus_value = received_params[8];
            auto_exposure = received_params[9];
            exposure_value = received_params[10];

            cout << "收到客户端更新参数：" << "亮度：" << brightness << ", " << "对比度" << contrast << ", "
                 << "伽马" << gamma << ", " << "饱和度：" << saturation << ", " << "色调：" << hue << ", " << "旋转角度：" << rotation_angle 
                 << ", " << "逆光对比：" << backlight_value << "增益：" << gain_value << "," << "聚焦：" << focus_value 
                 << ", 自动曝光：" << auto_exposure << ", 曝光值：" << exposure_value << endl;

            set_camera_params(cap, backlight_value, gain_value, focus_value);
            set_exposure(cap, auto_exposure, exposure_value);
        }

        Mat frame;
        cap >> frame;
        if (frame.empty()) {
            cerr << "无法捕获帧" << endl;
            break;
        }

        frame = adjust_brightness_contrast(frame, brightness - 50, contrast - 50);
        frame = adjust_gamma(frame, gamma);
        frame = adjust_saturation_hue(frame, saturation - 50, hue);
        frame = rotate_image(frame, rotation_angle);

        vector<uchar> encoded_frame;
        imencode(".jpg", frame, encoded_frame);

        int frame_size = encoded_frame.size();

        if (send(client_fd, &frame_size, sizeof(frame_size), 0) < 0) {
            cerr << "发送帧大小失败" << endl;
            break;
        }

        if (send(client_fd, encoded_frame.data(), frame_size, 0) < 0) {
            cerr << "发送帧数据失败" << endl;
            break;
        }
    }

    close(client_fd);
    close(server_fd);

    return 0;
}
