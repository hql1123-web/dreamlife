\# dreamlife



\## ex01：嵌入式开发作业——HelloWorld程序（C++）

\- 输出 Hello, World!

\- 读取键盘输入并回显



\## lab01（实验一）：ESP32 LED 亮灭控制

\- 使用 Visual Studio 对 ESP32 开发板编程

\- 控制 LED 亮灭

\- 结果见 Sketch1/result.jpg



\##  lab02（实验二）：基于 millis() 的多 LED SOS 信号

\- 使用 `millis()` 非阻塞方式控制板载 LED 和三个外接 LED 同时闪烁 SOS 信号。

\- SOS 编码：三短闪、三长闪、三短闪，循环之间带有长熄灭停顿。

\- 代码包含串口状态信息输出。

\- 实验视频见 Sketch2/rusult2.mp4



\##  lab03（实验三）：双 LED 呼吸灯（板载 + 外接）

\- 同时控制板载 LED (GPIO2) 和一个外接 LED (GPIO5) 实现同步呼吸效果。

\- 使用两个独立 LEDC 通道，可通过修改延时参数独立调节呼吸速率。

\- 实验视频见 Sketch3/result3.mp4

