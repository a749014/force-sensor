#include <HX711.h>
#include <U8g2lib.h>
#include <phyphoxBle.h>

// 宏定义引脚配置
#define HX711_DT_PIN  2  // HX711 数据引脚
#define HX711_SCK_PIN 4  // HX711 时钟引脚

// SSD1306 SPI引脚配置
#define OLED_RST  17  // 复位引脚
#define OLED_DC   16  // 数据/命令选择引脚
#define OLED_CS   5   // 片选引脚
#define OLED_MOSI 23  // 主出从入引脚
#define OLED_CLK  18  // 时钟引脚
#define buttonPin 15

// 创建HX711对象
HX711 scale;

// 创建U8g2对象（SPI接口，SSD1306 128x64）
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, OLED_CS, OLED_DC, OLED_RST);

// 校准参数
float calibration_factor = 40287; // 根据传感器调整

volatile bool isNeedTare = false;

void myButtonCallback(){
  isNeedTare=true;
}

void phyphoxSetup(){
  PhyphoxBLE::start("Misaka Mikoto");  // 蓝牙名称

  // 实验配置
  PhyphoxBleExperiment forceSensorExperiment;
  forceSensorExperiment.setTitle("力传感器实验");
  forceSensorExperiment.setCategory("Arduino Experiments");
  forceSensorExperiment.setDescription("实时测量力的大小并动态显示范围");

  // 第一页 - 时间 vs 力的大小（动态Y轴）
  PhyphoxBleExperiment::View firstView;
  firstView.setLabel("TimeVsForce");

  // 图表配置
  PhyphoxBleExperiment::Graph firstGraph;
  firstGraph.setLabel("力的大小随时间变化");
  firstGraph.setUnitX("s");
  firstGraph.setUnitY("N");
  firstGraph.setLabelX("时间");
  firstGraph.setLabelY("力的大小");
  firstGraph.setYPrecision(4);  // Y轴显示2位小数
  firstGraph.setMinY(0, LAYOUT_AUTO);     // Y轴最小值固定为0
  firstGraph.setMaxY(-1, LAYOUT_AUTO);    // Y轴最大值自动适配（-1表示自动）

  firstGraph.setChannel(0,1); 
  firstView.addElement(firstGraph);

  // 第二页 - 当前力值（只显示数值）
  PhyphoxBleExperiment::View secondView;
  secondView.setLabel("CurrentForce");

  PhyphoxBleExperiment::Value forceValue;
  forceValue.setLabel("实时力值");
  forceValue.setPrecision(2);  // 显示2位小数
  forceValue.setUnit("牛");
  forceValue.setColor("FFFFFF");
  forceValue.setChannel(1);
  secondView.addElement(forceValue);

  // 数据导出配置
  PhyphoxBleExperiment::ExportSet mySet;
  mySet.setLabel("导出数据");
  
  PhyphoxBleExperiment::ExportData timeData;
  timeData.setLabel("时间");
  timeData.setDatachannel(0);
  
  PhyphoxBleExperiment::ExportData forceData;
  forceData.setLabel("力值");
  forceData.setDatachannel(1);
  
  mySet.addElement(timeData);
  mySet.addElement(forceData);

  // 整合实验配置
  forceSensorExperiment.addExportSet(mySet);
  forceSensorExperiment.addView(firstView);
  forceSensorExperiment.addView(secondView);
  PhyphoxBLE::addExperiment(forceSensorExperiment);
}

void setup() {
  Serial.begin(115200);
  Serial.println("HX711 力传感器示例");

  // 初始化HX711
  scale.begin(HX711_DT_PIN, HX711_SCK_PIN);
  scale.set_scale(calibration_factor); // 设置校准因子
  scale.tare(); // 清零
  pinMode(buttonPin, INPUT_PULLUP);
    
    // 配置外部中断，检测下降沿
  attachInterrupt(digitalPinToInterrupt(buttonPin), myButtonCallback, FALLING);

  // 初始化SSD1306
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); // 设置字体
  u8g2.drawStr(0, 10, "Misaka Mikoto");
  u8g2.sendBuffer();
  phyphoxSetup();

}

void loop() {
  if (isNeedTare){
    Serial.println("需要清零");
    scale.tare();
    isNeedTare=false;
  }
  // 读取传感器数据
  float force_N = scale.get_units(5); // 读取5次取平均值

  // 在串口监视器输出
  Serial.print("力值: ");
  Serial.print(force_N);
  Serial.println(" N");

    // 在SSD1306上显示力值
  u8g2.clearBuffer();

  // 设置更大的字体
  u8g2.setFont(u8g2_font_ncenB24_tr); // 这里选择了一个更大的字体

  u8g2.setCursor(0, 30);
  u8g2.print(force_N);

  // 恢复原来的字体
  u8g2.setFont(u8g2_font_ncenB14_tr); // 恢复到原来的字体

  u8g2.drawStr(110, 30, "N");
  u8g2.sendBuffer();

  PhyphoxBLE::write(force_N);


  // Poll for phyphoxBLE server communication
  PhyphoxBLE::poll(); 
}