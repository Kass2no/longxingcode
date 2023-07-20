/**
 * 程序功能：使用MQTT协议，与阿里云物联网平台进行数据通信。
 *          在本示例程序中，实现了一次上报多个属性数据到云平台。
 * 使用教程：https://blog.yyzt.site/
 * 注意事项：1.所连接的WIFI名称和WIFI需要设置正确，并且仅支持连接2.4GHz的WIFI，不支持5GHz的WIFI
 *          2.阿里云物联网平台的设备证书（也叫三元组，ProductKey、DeviceName、DeviceSecret）需要设置正确
 *          3.相关属性名称标识符和数据类型需要设置正确
 * 参考资料：https://github.com/0xYootou/arduino-aliyun-iot-sdk
 *          https://github.com/legenddcr/aliyun-mqtt-arduino
 *          https://help.aliyun.com/document_detail/89301.html
 * 阿里云物联网平台网址：https://iot.console.aliyun.com/lk/summary/new
 */

#include <Arduino.h>          // 导入Arduino库文件
#include <YYZT_AliyunMqtt.h>  // 导入YYZT_AliyunMqtt库文件
#include "DHT.h"
#define MQPIN 32
#define MQPINDO 15
#define DHTPIN 4   
#define DHTTYPE DHT11   // DHT 11

/**
 * 串口调试打印相关参数
 */
#define debugState true             // 串口调试打印状态，true--开启调试打印，false--关闭调试打印
#define debugSerial Serial          // 调试打印的串口
#define debugSerialBuadrate 115200  // 调试打印的串口波特率

/**
 * 阿里云物联网平台相关参数
 */
#define productKey "hmi4jIZEyHv"                         // 产品密钥 ProductKey
#define deviceName "esp32_001"                           // 设备名称 DeviceName
#define deviceSecret "a32d8b2f17d3501313b8429004ad351b"  // 设备密钥 DeviceSecret

#define wendu_ID "temperature"        
#define wendubool_ID "temperaturebool"      
#define humidity_ID "humidity"    
#define humiditybool_ID "humiditybool" 
#define MQ2_ID "MQ2"
#define MQ2bool_ID "MQ2bool"
#define dianya_ID "dianya"
DHT dht(DHTPIN, DHTTYPE);
/**
 * WIFI相关参数
 */
// #define wifiName "kjcxlab03"       // WIFI名称
// #define wifiPassword "kjcxlab2021"  // WIFI密码
 #define wifiName "Xiaomi 13"       // WIFI名称
 #define wifiPassword "lzdsg207021011"  // WIFI密码

/**
 * MQTT相关参数
 */
#define mqttHeartbeatTime 60        // 心跳间隔时间，默认为60s，单位为：秒
#define mqttPacketSize 1024         // MQTT数据包的大小设置最大为1024字节
#define ckeckMqttConnectTime 10000  // 检查MQTT连接的间隔时间，默认为10秒
AliyunMqtt aliyunMqtt;              // 实例化aliyunMqtt对象

// MQTT的连接状态
typedef enum {
  MqttDisconnected = 0,  // MQTT已断开连接
  MqttConnecting,        // MQTT连接中
  MqttConnected          // MQTT已连接
} MqttConnectState;
uint8_t nowMqttConnectState = MqttDisconnected;  // 当前MQTT的连接状态，初始化为MqttDisconnected

/**
 * 函数功能：上报多种类型属性，10秒上报1次
 * 参数：无
 * 返回值：无
 * 注意事项：无
 */
void reportMultipleAttribute() {
  static uint32_t nowTime = millis();
  if (millis() - nowTime > 10000 && nowMqttConnectState == MqttConnected) {                                         // 如果大于发送时间并且MQTT已连接
    nowTime = millis();                                                                                             // 更新发送时间
   float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  //  uint32_t adc_val = 0;
  //  double voltage = 0;
   uint32_t adc_val = analogRead(34);
   double voltage = (((double)adc_val)/4095)*3.3*2;
  int MQ2value = analogRead(MQPIN);
  bool MQ2bool = digitalRead(MQPINDO);
  bool t_bool =0;
  if (t>=30)    //阈值设置
  {
    t_bool = 1;
  }
   bool h_bool =0;
  if (h>=40)
  {
    h_bool = 1;
  }
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) ) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
    // 方式1：使用String拼接要发送的字符串数据（比较繁琐，效率低，最不推荐）
    // String data =
    //     "{\"" +
    //     String(INT_TEST_ID) + "\":" + String(intData) + "," +
    //     "\"" + String(BOOL_TEST_ID) + "\":" + String(boolData) + "," +
    //     "\"" + String(FLOAT_TEST_ID) + "\":" + String(floatData) + "," +
    //     "\"" + String(STRING_TEST_ID) + "\":\"" + String(stringData) + "\"" +
    //     "}";

    // 方式2：使用sprintf拼接要发送的字符串数据（写法简洁高效，推荐）
    // char data[1024] = {0};
    // sprintf(data, "{\"%s\":%d,\"%s\":%d,\"%s\":%.2f,\"%s\":\"%s\"}",
    //         INT_TEST_ID, intData, BOOL_TEST_ID, boolData,
    //         FLOAT_TEST_ID, floatData, STRING_TEST_ID, stringData.c_str());

    // 方式3：使用JSON对象封装要发送的字符串数据（写法简洁高效，清晰明了，十分推荐！）

    
    StaticJsonDocument<1024> jsonData;                           // 实例化一个为1024字节容量的StaticJsonDocument对象
    jsonData[wendu_ID] = t;                             // 给整数型测试属性此键赋值
    jsonData[wendubool_ID] = (uint8_t)t_bool;                  // 给布尔型测试属性此键赋值，需要将bool转化成int类型，否则json解析后将会是true或false，而不是1或0
    jsonData[humidity_ID] = h;  // 给单精度浮点型测试属性此键赋值，保留2位小数
    jsonData[humiditybool_ID] = (uint8_t)h_bool;                      // 给字符串测试属性此键赋值
    jsonData[MQ2_ID] = MQ2value;
    jsonData[MQ2bool_ID] = (uint8_t)MQ2bool;
    jsonData[dianya_ID] = (double)voltage;
    String data = "";                                            // 定义一个存放json对象转化成字符串的变量
    serializeJson(jsonData, data);                               // 将JSON对象转换成字符串，即反序列化

    uint16_t result = aliyunMqtt.sendMultipleAttributeData(data);  // 发送数据到云平台
#if debugState
    debugSerial.println("上报多种类型属性，发送数据为：[" + String(data) + "]，发送状态为：" + String(result ? "已发送，ID为：" + String(result) : "失败！"));
#endif
  }
}

/**
 * 函数功能：检查MQTT和重连
 * 参数：无
 * 返回值：无
 * 注意事项：无
 */
void checkMqttAndReconnect() {
  static uint8_t mqttConnectStateMachine = 0;  // MQTT连接的状态机
  static uint32_t nowTime = 0;                 // 检查时间
  switch (mqttConnectStateMachine) {
    // 检查MQTT的连接状态
    case 0: {
      if (MqttDisconnected == nowMqttConnectState) {  // 如果当前MQTT的连接状态为已断开连接
        aliyunMqtt.mqttClient.disconnect(true);       // 断开之前的连接
        aliyunMqtt.mqttClient.connect();              // 连接MQTT服务器，连接结果在连接事件的回调函数或断开连接事件的回调函数中处理
        mqttConnectStateMachine = 1;                  // MQTT连接中，状态机=1，重连中
        nowTime = millis();                           // 更新检查时间
#if debugState
        debugSerial.println("阿里云MQTT服务器已断开连接！开始尝试重连中......");
#endif
      }
    } break;
    // MQTT重连中
    case 1: {
      if (MqttConnected == nowMqttConnectState || millis() - nowTime > ckeckMqttConnectTime) {  // 如果当前MQTT的连接状态为已连接或者超过重连时间
        mqttConnectStateMachine = 0;                                                            // 状态机=0，回到检查中
      }
    } break;

    default:
      break;
  }
}

/**
 * 函数功能：MQTT连接事件的回调函数
 * 参数1：[_sessionPresent] [bool] 会话是否存在
 * 返回值：无
 * 注意事项：无
 */
void onMqttConnect(bool _sessionPresent) {
#if debugState
  debugSerial.println("阿里云物联网MQTT服务器已连接，会话存在状态为：" + String(_sessionPresent));
#endif
  nowMqttConnectState = MqttConnected;  // 当前MQTT的连接状态为已连接
}

/**
 * 函数功能：MQTT断开连接事件的回调函数
 * 参数1：[_reason] [AsyncMqttClientDisconnectReason] MQTT断开连接的原因，在AsyncMqttClientDisconnectReason枚举中查看
 * 返回值：无
 * 注意事项：无
 */
void onMqttDisconnect(AsyncMqttClientDisconnectReason _reason) {
#if debugState
  debugSerial.println("阿里云物联网MQTT服务器已断开连接！原因为：" + String(uint8_t(_reason)));
#endif
  nowMqttConnectState = MqttDisconnected;  // 当前MQTT的连接状态为已断开连接
}

/**
 * 函数功能：程序初始化时调用，仅执行一次ss
 * 参数：无
 * 返回值：无
 * 注意事项：无
 */
void setup() {
  delay(1000);  // 等待设备上电稳定
  pinMode(MQPIN,INPUT);
  pinMode(MQPINDO,INPUT);
  Serial.println(F("DHTxx test!"));
  Serial.println(F("MQ2 test!"));
  dht.begin();
  delay(10000);
#if debugState
  debugSerial.begin(debugSerialBuadrate);  // 初始化调试打印的串口
  debugSerial.println("\r\n程序开始初始化......");
#endif

  WiFi.disconnect(true);                   // 断开当前WIFI连接
  WiFi.mode(WIFI_STA);                     // 设置WIFI模式为STA模式
  WiFi.begin(wifiName, wifiPassword);      // 开始连接WIFI
  while (WL_CONNECTED != WiFi.status()) {  // 判断WIFI的连接状态
    delay(500);
#if debugState
    debugSerial.print(".");
#endif
  }

  // aliyunMqtt.setDebug(debugSerial);                                       // 设置是否开启aliyunMqtt库中的打印，传入串口即开启打印
  aliyunMqtt.setDeviceCertificate(productKey, deviceName, deviceSecret);  // 设置连接阿里云物联网平台的设备证书（也叫三元组）

  aliyunMqtt.mqttClient.setKeepAlive(mqttHeartbeatTime);    // 设置心跳间隔时间
  aliyunMqtt.mqttClient.setMaxTopicLength(mqttPacketSize);  // 设置MQTT数据包大小
  aliyunMqtt.mqttClient.onConnect(onMqttConnect);           // 设置MQTT连接事件的回调函数
  aliyunMqtt.mqttClient.onDisconnect(onMqttDisconnect);     // 设置MQTT断开连接事件的回调函数
  aliyunMqtt.mqttClient.connect();                          // 连接MQTT服务器，连接结果在连接事件的回调函数或断开连接事件的回调函数中处理
  nowMqttConnectState = MqttConnecting;                     // 当前MQTT的连接状态为连接中

#if debugState
  debugSerial.println("\r\n程序初始化完成！");
#endif
}

/**
 * 函数功能：程序主循环函数，重复运行
 * 参数：无
 * 返回值：无
 * 注意事项：无
 */

void loop() {
  // uint32_t adc_val = 0;
  // double voltage = 0;
  //  adc_val = analogRead(34);
  //  voltage = (((double)adc_val)/4095)*3.3*2;
  //Serial.printf("adc_val:%d, voltage:%lfV.\r\n", adc_val, voltage);
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  float sensorValue = analogRead(MQPIN);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  checkMqttAndReconnect();    // 检查MQTT和重连
  reportMultipleAttribute();  // 上报多种类型属性，10秒上报1次
  
}
