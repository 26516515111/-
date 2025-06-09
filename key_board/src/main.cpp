#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <cmath>
//---------宏定义-----------
// 按键
#define NUM_KEYS 18
// 旋钮
#define EC11_CLK 11
#define EC11_DT 10
// 屏幕
#define SDA_PIN 8
#define SCL_PIN 9
#define OLED_ADDR 0x3C
//RGB
#define LED_PIN 27
#define LED_PIN2 14
#define NUM_LEDS 16
#define BTN_RAINBOW 13

//----------------屏幕--------------------------
//声明对象
Adafruit_SSD1306 display(128, 32, &Wire, -1);

//----------------按键--------------------------
// 键位对应的IO引脚（PCB背面有标注）
const uint8_t keyPins[NUM_KEYS] = {
    12, 13,
    7, 6, 5, 4,
    20, 21, 22, 26,
    3, 2, 1, 0,
    16, 17, 18, 19};
// 键位对应的HID码
const uint8_t keyCodes[NUM_KEYS] = {
    0X53, 0x2A,
    0x5F, 0x60, 0x61, 0x57,
    0x5C, 0x5D, 0x5E, 0x56,
    0x59, 0x5A, 0x5B, 0x55,
    0x62, 0x63, 0x58, 0x54};
// 键位对应的字符
const String keyNum[NUM_KEYS] = {
    "AC", "DEL",
    "7", "8", "9", "+",
    "4", "5", "6", "-",
    "1", "2", "3", "x",
    ".", "0", "=", "/"};
// 键位状态
bool keyPressed[NUM_KEYS] = {false};



//----------------USB-HID--------------------------
// 对象声明
Adafruit_USBD_HID usb_hid;
// HID报告描述符
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)),
};
// USB HID结构体
typedef struct
{
  uint8_t modifier;
  uint8_t reserved;
  uint8_t keycode[6];
} KeyboardReport;
KeyboardReport keyReport = {0};


//----------------RGB灯光--------------------------
// 声明对象
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(NUM_LEDS, LED_PIN2, NEO_GRB + NEO_KHZ800);
// RGB功能变量
volatile int targetBrightness = 50;          // 目标亮度值
int currentBrightness = 0;                   // 当前实际亮度值
const int SMOOTHING_STEP = 1;                // 亮度渐变步长
const unsigned long SMOOTHING_INTERVAL = 10; // 渐变间隔(ms)
int BRIGHTNESS_STEP = 1;
// RGB模式枚举
enum LEDMode
{
  MODE_RAINBOW, // 原版彩虹
  MODE_MARQUEE, // 跑马灯
};
volatile LEDMode ledMode = MODE_RAINBOW; // 当前灯效模式
unsigned long effectTimer = 0;           // 效果计时器
// 跑马灯参数
int marqueePosition = 0;
// 彩虹色轮函数
// 函数：颜色轮生成器 - 根据输入位置生成彩虹渐变颜色
// 参数：WheelPos - 颜色位置值（0-255），0为起始位置，255为结束位置
// 返回值：32位颜色值（通常格式为0x00RRGGBB）
/* 颜色过渡逻辑说明：
 * 整个255色环被均分为三个区段：
 * 1) 红->蓝（0-84）  2) 蓝->绿（85-169） 3) 绿->红（170-255）
 * 每个区段通过两个颜色分量的线性变化实现平滑过渡
 * 使用255色阶（3*85=255）确保颜色变化连贯无断层
 * 最终效果呈现旋转彩虹色光带效果 */
// 彩虹灯效
uint32_t Wheel(byte WheelPos) 
{
  // 反转输入值，使颜色轮逆向旋转（255为起点，0为终点）
  WheelPos = 255 - WheelPos;

  /* 第一阶段：红色 -> 蓝色渐变（当WheelPos在0-84范围时）
   * 红色分量从255递减至3（255 - 0*3 ~ 255 - 84*3）
   * 蓝色分量从0递增至252（0*3 ~ 84*3）
   * 绿色分量保持为0 */
  if (WheelPos < 85)
    return strip.Color(
      255 - WheelPos * 3, 
      0,                  
      WheelPos * 3       
  );
  /* 第二阶段：蓝色 -> 绿色渐变（当WheelPos在85-169范围时）
   * 蓝色分量从252递减至3（255 - 0*3 ~ 255 - 84*3）
   * 绿色分量从0递增至252（0*3 ~ 84*3）
   * 红色分量保持为0 */
  if (WheelPos < 170) 
  {
    WheelPos -= 85;       
    return strip.Color(
      0,                  
      WheelPos * 3,       
      255 - WheelPos * 3  
    );
  }

  /* 第三阶段：绿色 -> 红色渐变（当WheelPos在170-255范围时）
   * 绿色分量从252递减至3（255 - 0*3 ~ 255 - 84*3）
   * 红色分量从0递增至252（0*3 ~ 84*3）
   * 蓝色分量保持为0 */
  WheelPos -= 170;         
  return strip.Color(
    WheelPos * 3,         
    255 - WheelPos * 3,   
    0                     
  );
}


void effectRainbow()
{
  static uint8_t offset = 0;
  for (int i = 0; i < NUM_LEDS; i++)
  {
    strip.setPixelColor(i, Wheel((i + offset) & 255));
      strip2.setPixelColor(marqueePosition, 0xFF0000);  
  }
  strip.show();
  strip2.show();
  offset++;
  delay(50);
}
// 跑马灯效果
void effectMarquee()
{
  if (millis() - effectTimer > 200)
  {
    effectTimer = millis();

    strip.clear();
    strip.setPixelColor(marqueePosition, 0xFF0000);                  // 红色光点
    strip.setPixelColor((marqueePosition + 4) % NUM_LEDS, 0x00FF00); // 绿色跟随
    strip.setPixelColor((marqueePosition + 8) % NUM_LEDS, 0x0000FF); // 蓝色跟随
    strip.show();

    marqueePosition = (marqueePosition + 1) % NUM_LEDS;
  }
}
// 灯效模式调用
void runLEDEffect()
{
  switch (ledMode)
  {
  case MODE_RAINBOW:
    effectRainbow();
    break;
  case MODE_MARQUEE:
    effectMarquee();
    break;
  }
}

//----------------EC11旋钮逻辑判断--------------------------
void handleEncoderRotation()
{
  // 静态变量保存前次状态（CLK和DT的组合状态）
  static uint8_t lastState = 0;    // 存储上一次的引脚组合状态（2位二进制）
  static unsigned long lastTime = 0; // 存储上次状态变化的时间戳（防抖用）
  const unsigned long debounceDelay = 5; // 消抖时间阈值（单位：毫秒）

  // 读取当前编码器状态（将两个引脚状态合并为2位二进制数）
  // 格式：高1位是CLK引脚状态，低1位是DT引脚状态
  uint8_t state = (digitalRead(EC11_CLK) << 1) | digitalRead(EC11_DT);

  // 检测有效状态变化（状态不同且满足消抖时间条件）
  if (state != lastState && (millis() - lastTime > debounceDelay))
  {
    /* 顺时针旋转状态序列检测（EC11编码器四步变化规律）：
     * 00 -> 01 -> 11 -> 10 -> 00
     * 当检测到以下任一有效转换时判定为顺时针旋转：
 */
    if ((lastState == 0b00 && state == 0b01) ||
        (lastState == 0b01 && state == 0b11) ||
        (lastState == 0b11 && state == 0b10) ||
        (lastState == 0b10 && state == 0b00))
    {
      // 顺时针旋转：降低目标亮度（需确保不低于最小值）
      targetBrightness = (targetBrightness - BRIGHTNESS_STEP);
    }
    /* 逆时针旋转状态序列检测（反向四步变化规律）：
     * 00 -> 10 -> 11 -> 01 -> 00
     * 当检测到以下任一有效转换时判定为逆时针旋转：
 */
    else if (
        (lastState == 0b00 && state == 0b10) ||
        (lastState == 0b10 && state == 0b11) ||
        (lastState == 0b11 && state == 0b01) ||
        (lastState == 0b01 && state == 0b00))
    {
      // 逆时针旋转：增加目标亮度（需确保不超过最大值）
      targetBrightness = (targetBrightness + BRIGHTNESS_STEP);
    }

    // 更新状态变化时间戳
    lastTime = millis();
    // 保存当前状态用于下次比较
    lastState = state;
  }
}



//----------------计算器--------------------------
String calcExpression = "";
String calcDisplay = "0";
bool needsClear = false;
String calcDisplayNum = "";

//计算器逻辑处理
float evaluateExpression(String expr)
{
  expr.replace(" ", "");  // 移除所有空格，确保表达式紧凑
  float result = 0;       // 最终计算结果
  float currentTerm = 0;  // 当前处理的运算项（用于处理乘除优先级）
  char op = '+';          // 当前运算符（初始化为+）
  String numStr;          // 临时存储数字字符串（支持多位数和小数）
  bool newNum = true;     // 标志位，表示是否开始新数字输入

  // 逐字符解析表达式
  for (int i = 0; i < expr.length(); i++)
  {
    char c = expr[i];
    /* 数字/符号处理分支（支持以下情况）：
     * 1. 常规数字：0-9
     * 2. 小数点：.
     * 3. 合法负号：出现在数字开头或运算符后 */
    if (isdigit(c) || c == '.' || (c == '-' && newNum))
    {
      numStr += c;        // 将字符追加到数字缓冲区
      newNum = false;     // 标记进入数字输入状态
    }
    // 运算符处理分支（当前字符为运算符且数字缓冲区非空）
    else if (!newNum)
    {
      float num = numStr.toFloat(); // 转换缓冲区为浮点数
      numStr = "";                  // 清空数字缓冲区
      
      /* 根据前一个运算符处理当前项（实现乘除优先级）：
       * 加减运算符：将当前项加入结果
       * 乘除运算符：立即计算当前项 */
      switch (op)
      {
      case '+':
        currentTerm = num;  // 加法项直接存入当前项
        break;
      case '-':
        currentTerm = -num; // 减法项转为负数存储
        break;
      case 'x':
        currentTerm *= num; // 乘法立即计算
        break;
      case '/':
        if (num == 0)
          return NAN;      // 除零错误返回特殊值
        currentTerm /= num; // 除法立即计算
        break;
      }

      /* 运算符类型判断：
       * 遇到加减运算符时将当前项累加到结果
       * 遇到乘除运算符时暂存当前项继续运算 */
      if (c == '+' || c == '-')
      {
        result += currentTerm; // 将累计的当前项加入最终结果
        currentTerm = 0;       // 重置当前项
      }
      op = c;          // 更新当前运算符
      newNum = true;   // 标记需要开始新数字
    }
  }

  /* 处理最后一个数字项（表达式末尾没有运算符的情况）
   * 示例：处理"5+3"中的3 */
  if (numStr.length() > 0)
  {
    float num = numStr.toFloat();
    switch (op)  // 根据最后的运算符处理
    {
    case '+':
      currentTerm = num;
      break;
    case '-':
      currentTerm = -num;
      break;
    case 'x':
      currentTerm *= num;
      break;
    case '/':
      if (num == 0)
        return NAN;
      currentTerm /= num;
      break;
    }
  }
  
  result += currentTerm;  // 将最后的当前项加入结果
  return result;          // 返回最终计算结果
}




//----------------核心1--------------------------
//----------------入口--------------------------
void setup()
{
  // 初始化按键矩阵
  for (int i = 0; i < NUM_KEYS; i++)
  {
    pinMode(keyPins[i], INPUT_PULLUP);
  }

  // 初始化编码器
  pinMode(EC11_CLK, INPUT_PULLUP);
  pinMode(EC11_DT, INPUT_PULLUP);

  // HID 初始化
  Serial1.print("HID INIT");
  usb_hid.setBootProtocol(false);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setPollInterval(2);
  usb_hid.begin();
}
//----------------循环--------------------------
void loop()
{
  handleEncoderRotation();
  if (TinyUSBDevice.mounted())
  {
    // 处理按键
    for (int i = 0; i < NUM_KEYS; i++)
    {
      if (digitalRead(keyPins[i]) == LOW && !keyPressed[i])
      {
        if (usb_hid.ready())
        {
          keyReport.keycode[0] = keyCodes[i];
          usb_hid.sendReport(1, &keyReport, sizeof(keyReport));
          delay(20);
          memset(&keyReport, 0, sizeof(keyReport));
          usb_hid.sendReport(1, &keyReport, sizeof(keyReport));
        }
        keyPressed[i] = true;
      }
      else if (digitalRead(keyPins[i]) == HIGH)
      {
        keyPressed[i] = false;
      }
    }

  }
  else
  { // 计算器模式
    for (int i = 0; i < NUM_KEYS; i++)
    {
      if (digitalRead(keyPins[i]) == LOW && !keyPressed[i])
      {
        String key = keyNum[i];

        if (key == "=")
        { // 计算结果
          if (calcExpression.length() > 0)
          {
            float result = evaluateExpression(calcExpression);
            if (isnan(result))
            {
              calcDisplay = "Error";
            }
            else
            {
              calcDisplay = String(result, 3);
            }
            needsClear = true;
            calcExpression = String(result);
            calcDisplayNum = "";
          }
        }
        else if (key == ".")
        { // 处理小数点
          if (calcExpression.indexOf('.') == -1)
          {
            calcExpression += key;
            calcDisplayNum += key;
            calcDisplay = calcDisplayNum;
          }
        }
        else if (key == "DEL")
        { // 新增删除键处理
          if (calcExpression.length() > 0)
          {
            calcExpression.remove(calcExpression.length() - 1);
            calcDisplayNum.remove(calcDisplayNum.length() - 1);
            calcDisplay = calcDisplayNum;
            if (calcDisplay.length() == 0)
            {
              calcDisplay = "0";
            }
          }
          else
          {
            calcDisplay = "0";
          }
          needsClear = false;
        }
        else if (key == "+" || key == "-" || key == "x" || key == "/")
        { // 其他有效按键

          calcExpression += key;
          calcDisplayNum = key;
          calcDisplay = calcDisplayNum;
        }
        else if (key == "AC")
        { // 其他有效按键
          calcDisplay = "";
          needsClear = false;
          calcExpression = "";
          calcDisplayNum = "";
          calcDisplay = "0";
        }
        else if (key.length() > 0)
        { // 其他有效按键
          if (needsClear)
          {
            calcDisplay = "";
            needsClear = false;
          }
          calcExpression += key;
          calcDisplayNum += key;
          calcDisplay = calcDisplayNum;
        }
        keyPressed[i] = true;
      }
      else if (digitalRead(keyPins[i]) == HIGH)
      {
        keyPressed[i] = false;
      }
    }

  }
  delay(10);
}

//----------------核心2--------------------------
//----------------入口--------------------------
void setup1()
{
  Wire.setSDA(SDA_PIN);
  Wire.setSCL(SCL_PIN);
  Wire.begin();
  // OLED 初始化
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("EDA-Keypad");
   display.setCursor(100, 22);
   display.setTextSize(1);
  display.println("v1.0");
  display.display();
  delay(3000);
  // LED 初始化
  pinMode(BTN_RAINBOW, INPUT_PULLUP);
  strip.begin();
  strip.setBrightness(currentBrightness);
  strip.clear();
  strip.show();
    strip2.begin();
  strip2.setBrightness(currentBrightness);
  strip2.clear();
  strip2.show();
}
//----------------循环--------------------------
void loop1()
{
  static unsigned long lastSmoothUpdate = 0;
  // 亮度平滑处理
  if (millis() - lastSmoothUpdate >= SMOOTHING_INTERVAL)
  {
    lastSmoothUpdate = millis();
    // 渐进式调整当前亮度
    if(targetBrightness>255)targetBrightness=255;
    if (currentBrightness != targetBrightness)
    {
      int direction = (targetBrightness > currentBrightness) ? 1 : -1;
      currentBrightness = constrain(currentBrightness + direction * SMOOTHING_STEP, 0, 255);
      strip.setBrightness(currentBrightness);
      strip2.setBrightness(currentBrightness);
    }
  }
  runLEDEffect();
  if (TinyUSBDevice.mounted())//USB通讯成功时
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("--  KeyBoard Mode  --");
    display.printf("Brightness: %d%%\n", (currentBrightness * 100) / 255);
    if (ledMode == 0)//
    {
      display.println("Light-Mode: RAINBOW");
    }
    else
    {
      display.println("Light-Mode: MARQUEE");
    }
    display.println("Version:    V1.0");
    display.display();
  }
  else//连接设备不支持USB通讯协议时
  {
    // 计算器显示模式
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("-- Calculator Mode --");
    display.setTextSize(2);
    display.setCursor(0, 16);
    // 处理长数字显示
    if (calcDisplay.length() > 10 && calcDisplay.length() <= 42)
    {
      display.setTextSize(1);
      display.setCursor(0, 16);
      display.println(calcDisplay);
    }
    else if (calcDisplay.length() <= 10)
    {
      display.println(calcDisplay);
    }
    else
    {
      display.println("Too long...");
      needsClear = true;
      calcExpression = "";
    }
    display.display();
  }
}