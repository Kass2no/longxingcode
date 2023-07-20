/**
 * 程序功能：使用MQTT协议，与阿里云物联网平台进行数据通信。适用的平台为ESP32和ESP8266
 * 使用教程：https://blog.yyzt.site/
 * 注意事项：无
 * 参考资料：https://github.com/0xYootou/arduino-aliyun-iot-sdk
 *          https://github.com/legenddcr/aliyun-mqtt-arduino
 *          https://help.aliyun.com/document_detail/89301.html
 * 阿里云物联网平台网址：https://iot.console.aliyun.com/lk/summary/new
 */

#ifndef YYZT_ALIYUNMQTT_H
#define YYZT_ALIYUNMQTT_H

#include <Arduino.h>
#if defined(ARDUINO_ARCH_ESP8266)  // 如果使用了ESP8266平台
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)  // 如果使用了ESP32平台
#include <WiFi.h>
#endif
#include <ArduinoJson.h>      // 序列化以及反序列化JSON字符串数据
#include <AsyncMqttClient.h>  // 负责整体MQTT协议，以及实现MQTT客户端的功能（包括设置Client端来源、连接服务器、发送数据等）
#include <SHA256.h>           // SHA256加密

#define YYZT_AliyunMqttLibraryVersion 0.0    // 库版本
#define YYZT_abs(x) ((x) >= 0 ? (x) : -(x))  // 计算绝对值

#define sha256HmacSize 32  // 哈希值的大小
#define mqttPort 1883      // MQTT服务器的端口

// Alink协议——设备上报属性数据格式
#define alinkDeviceReportAttributeFormat "{\"id\":\"123\",\"version\":\"1.0\",\"method\":\"thing.event.property.post\",\"params\":%s}"
// Alink协议——设备上报事件数据格式
#define alinkDeviceReportEventFormat "{\"id\":\"123\",\"version\":\"1.0\",\"params\":%s,\"method\":\"thing.event.%s.post\"}"

typedef void (*poniterReceiveDeserialization)(JsonVariant _data);  // 定义一个接收json字符串反序列化的函数指针

class AliyunMqtt {
 public:
  /**
   * 函数功能：设置连接阿里云物联网平台的设备证书（也叫三元组）以及地域，即ProductKey、DeviceName、DeviceSecret
   * 参数1：[_productKey] [String] 产品密钥
   * 参数2：[_deviceName] [String] 设备名称
   * 参数3：[_deviceSecret] [String] 设备密钥
   * 参数4：[_region] [String] 地区，默认为上海，即"cn-shanghai"
   * 返回值：无
   * 注意事项：无
   */
  void setDeviceCertificate(String _productKey, String _deviceName, String _deviceSecret, String _region = "cn-shanghai");

  /**
   * 函数功能：绑定属性以及设置接收回调函数
   * 参数1：[_attributeName] [String] 属性名称
   * 参数2：[_cb] [poniterReceiveDeserialization] 回调函数
   * 返回值：[bool] true--设置成功，false--设置失败，属性名称为空或者已超过最大设置数量，默认最大为50
   * 注意事项：无
   */
  bool bindAttribute(String _attributeName, poniterReceiveDeserialization _cb);

  /**
   * 函数功能：取消绑定的属性
   * 参数1：[_attributeName] [String] 属性名称
   * 返回值：[bool] true--取消设置成功，false--取消设置失败，不存在该属性名称
   * 注意事项：无
   */
  bool unbindAttribute(String _attributeName);

  /**
   * 函数功能：订阅主题
   * 参数1：[_topicName] [String] 主题名称
   * 参数2：[_cb] [poniterReceiveDeserialization] 回调函数
   * 参数3：[_qos] [uint8_t] qos等级，默认为0
   * 返回值：[uint16_t] 非0--订阅成功，返回数据包ID，为0--订阅失败，主题名称为空或者已超过最大设置数量，默认最大为50
   * 注意事项：无
   */
  uint16_t subscribeTopic(String _topicName, poniterReceiveDeserialization _cb, uint8_t _qos = 0);

  /**
   * 函数功能：取消订阅主题
   * 参数1：[_topicName] [String] 主题名称
   * 返回值：[uint16_t] 非0--取消订阅成功，返回数据包ID，为0--取消订阅失败，不存在该主题名称
   * 注意事项：无
   */
  uint16_t unsubscribeTopic(String _topicName);

  /**
   * 函数功能：发送 double 类型数据到云平台，同时兼容float
   * 参数1：[_attributeName] [String] 属性名称
   * 参数2：[_data] [double] 数据，最外层不需要加花括号
   * 参数3：[_decimalPlaces] [uint8_t] 保留小数点后几位，默认为2位
   * 参数4：[_qos] [uint8_t] qos等级，默认为0
   * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
   * 注意事项：无
   */
  uint16_t sendAttributeData(String _attributeName, double _data, uint8_t _decimalPlaces = 2, uint8_t _qos = 0);

  /**
   * 函数功能：发送 int32_t 类型数据到云平台，同时兼容其他整数类型和bool类型
   * 参数1：[_attributeName] [String] 属性名称
   * 参数2：[_data] [int32_t] 数据，最外层不需要加花括号
   * 参数3：[_qos] [uint8_t] qos等级，默认为0
   * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
   * 注意事项：无
   */
  uint16_t sendAttributeData(String _attributeName, int32_t _data, uint8_t _qos = 0);

  /**
   * 函数功能：发送 String 类型数据到云平台
   * 参数1：[_attributeName] [String] 属性名称
   * 参数2：[_data] [String] 数据，最外层不需要加花括号
   * 参数3：[_qos] [uint8_t] qos等级，默认为0
   * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
   * 注意事项：无
   */
  uint16_t sendAttributeData(String _attributeName, String _data, uint8_t _qos = 0);

  /**
   * 函数功能：发送多个数据到云平台
   * 参数1：[_data] [String] 数据内容，需要提前拼接好，_data格式为：{"name":"一叶遮天","age":22,"site":"blog.yyzt.site","height":182.34}，仿照这个格式根据实际情况改写，最外层需要加花括号
   * 参数2：[_qos] [uint8_t] qos等级，默认为0
   * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
   * 注意事项：无
   */
  uint16_t sendMultipleAttributeData(String _data, uint8_t _qos = 0);

  /**
   * 函数功能：发送事件到云平台
   * 参数1：[_eventId] [String] 事件ID，即标识符
   * 参数2：[_data] [String] 数据内容，默认为空则发送空数据，最外层需要加花括号
   * 参数3：[_qos] [uint8_t] qos等级，默认为0
   * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
   * 注意事项：无
   */
  uint16_t sendEventData(String _eventId, String _data = "{}", uint8_t _qos = 0);

  /**
   * 函数功能：发送自定义数据格式和内容到云平台
   * 参数1：[_topicFormat] [String] 数据格式，即Topic类，在产品中的Topic类列表中查看
   * 参数2：[_data] [String] 数据内容，默认为空，最外层需要加花括号
   * 参数3：[_qos] [uint8_t] qos等级，默认为0
   * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
   * 注意事项：数据格式不能错误，否则会导致云平台接收或者解析不了，可以参考本代码中的OMCT_DeviceAttributeReportingFormat、OMCT_DeviceEventReportingFormat的写法
   */
  uint16_t sendCustomData(String _topicFormat, String _data = "{}", uint8_t _qos = 0);

  /**
   * 函数功能：设置回调函数的最大数量
   * 参数1：[_number] [uint8_t] 数量
   * 返回值：无
   * 注意事项：无
   */
  void setCallbackCountMax(uint8_t _number);

  /**
   * 函数功能：设置调试打印，如果未设置，则不开启打印，如果传入串口，则开启打印
   * 参数1：[_serial] [Stream&] 串口
   * 返回值：无
   * 注意事项：无
   */
  void setDebug(Stream& _serial);

  /**
   * 函数功能：订阅设备属性设置主题
   * 参数1：[_qos] [uint8_t] qos等级，默认为0
   * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
   * 注意事项：此功能仅订阅设备属性设置主题，即便不调用也不影响设备属性设置数据的下发与接收
   */
  uint16_t subscribeAttributeSetting(uint8_t _qos = 0);

  AsyncMqttClient mqttClient;  // 实例化mqttClient

 private:
  uint8_t callbackCountMax = 50;  // 回调函数的最大数量，默认为50个
  typedef struct {
    String key = "";                          // 属性名称，默认为空
    poniterReceiveDeserialization cb = NULL;  // 回调函数，默认为空指针
  } poniterJsonDeserialization;
  poniterJsonDeserialization* poniterJsonDeserializationArray = new poniterJsonDeserialization[callbackCountMax];  // 默认最多绑定50个回调
  Stream* debugSerial = NULL;                                                                                      // 调试串口
  bool debugState = false;                                                                                         // 调试串口状态，默认为false即关闭调试串口打印

  char clientId[256] = {0};      // 客户端id
  char mqttUsername[100] = {0};  // MQTT用户名
  char mqttPassword[100] = {0};  // MQTT密码
  char domain[100] = {0};        // 云平台域名

  // OMCT表示 object model communication topic，即物模型通信Topic
  char OMCT_DeviceAttributeReportingFormat[100] = {0};  // 物模型通信Topic的设备属性上报数据格式
  char OMCT_DevicePropertySettingsFormat[100] = {0};    // 物模型通信Topic的设备属性设置数据格式
  char OMCT_DeviceEventReportingFormat[100] = {0};      // 物模型通信Topic的设备事件上报数据格式

  /**
   * 函数功能：接收MQTT数据的回调函数
   * 参数1：[_topic] [char*] 主题，即属性
   * 参数2：[_payload] [uint8_t*] 数据内容
   * 参数3：[_properties] [AsyncMqttClientMessageProperties] 属性
   * 参数4：[_length] [size_t] 数据内容长度
   * 参数5：[_index] [size_t] 索引
   * 参数6：[_total] [size_t] 总大小
   * 返回值：无
   * 注意事项：无
   */
  void mqttReceiveCallback(char* _topic, char* _payload, AsyncMqttClientMessageProperties _properties, size_t _length, size_t _index, size_t _total);

  /**
   * 函数功能：SHA256加密
   * 参数1：[_signContent] [const String&] 签名内容
   * 参数2：[_deviceSecret] [const String&] 设备密钥
   * 返回值：[String] 加密后的字符串
   * 注意事项：无
   */
  String hmac256(const String& _signContent, const String& _deviceSecret);

  /**
   * 函数功能：发送数据到云平台
   * 参数1：[_topic] [String] 主题
   * 参数2：[_payload] [String] 数据
   * 参数3：[_qos] [uint8_t] qos等级，默认为0
   * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
   * 注意事项：无
   */
  uint16_t sendDataToCloud(String _topic, String _payload, uint8_t _qos = 0);
};

#endif
