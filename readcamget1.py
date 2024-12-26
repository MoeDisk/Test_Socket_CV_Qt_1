import cv2

# 打开摄像头
cap = cv2.VideoCapture(1)

# 检查摄像头是否成功打开
if not cap.isOpened():
    print("无法打开摄像头")
    exit()

# 获取摄像头的默认参数
brightness = cap.get(cv2.CAP_PROP_BRIGHTNESS)
contrast = cap.get(cv2.CAP_PROP_CONTRAST)
saturation = cap.get(cv2.CAP_PROP_SATURATION)
hue = cap.get(cv2.CAP_PROP_HUE)
exposure = cap.get(cv2.CAP_PROP_EXPOSURE)
auto_wb = cap.get(cv2.CAP_PROP_AUTO_WB)
wb_temperature = cap.get(cv2.CAP_PROP_WB_TEMPERATURE)
gain = cap.get(cv2.CAP_PROP_GAIN)
backlight = cap.get(cv2.CAP_PROP_BACKLIGHT)

# 输出摄像头的默认参数
print(f"亮度: {brightness}")
print(f"对比度: {contrast}")
print(f"饱和度: {saturation}")
print(f"色调: {hue}")
print(f"曝光值: {exposure}")
print(f"自动白平衡: {auto_wb}")
print(f"白平衡温度: {wb_temperature}")
print(f"增益: {gain}")
print(f"逆光对比: {backlight}")

# 释放摄像头
cap.release()

