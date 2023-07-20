#include "YYZT_AliyunMqtt.h"

/**
 * 函数功能：设置连接阿里云物联网平台的设备证书（也叫三元组）以及地域，即ProductKey、DeviceName、DeviceSecret
 * 参数1：[_productKey] [String] 产品密钥
 * 参数2：[_deviceName] [String] 设备名称
 * 参数3：[_deviceSecret] [String] 设备密钥
 * 参数4：[_region] [String] 地区，默认为上海，即"cn-shanghai"
 * 返回值：无
 * 注意事项：无
 */
void AliyunMqtt::setDeviceCertificate(String _productKey, String _deviceName, String _deviceSecret, String _region) {
  String timestamp = String(millis());                                                                                                                      // 获取设备上电的时间戳
  char signContent[512] = {0};                                                                                                                              // 定义签名字符串变量
  sprintf(clientId, "%s|securemode=3,signmethod=hmacsha256,timestamp=%s|", _deviceName.c_str(), timestamp.c_str());                                         // 拼接客户端id
  sprintf(signContent, "clientId%sdeviceName%sproductKey%stimestamp%s", _deviceName.c_str(), _deviceName.c_str(), _productKey.c_str(), timestamp.c_str());  // 拼接签名内容

  hmac256(signContent, _deviceSecret).toCharArray(mqttPassword, sizeof(mqttPassword));  // MQTT密码进行SHA256加密

  sprintf(mqttUsername, "%s&%s", _deviceName.c_str(), _productKey.c_str());
  sprintf(OMCT_DeviceAttributeReportingFormat, "/sys/%s/%s/thing/event/property/post", _productKey.c_str(), _deviceName.c_str());
  sprintf(OMCT_DevicePropertySettingsFormat, "/sys/%s/%s/thing/service/property/set", _productKey.c_str(), _deviceName.c_str());
  sprintf(OMCT_DeviceEventReportingFormat, "/sys/%s/%s/thing/event", _productKey.c_str(), _deviceName.c_str());
  sprintf(domain, "%s.iot-as-mqtt.%s.aliyuncs.com", _productKey.c_str(), _region.c_str());

  mqttClient.setServer(domain, mqttPort);                                                                                                         // 设置服务器ip和端口
  mqttClient.setClientId(clientId);                                                                                                               // 设置clientId
  mqttClient.setCredentials(mqttUsername, mqttPassword);                                                                                          // 设置MQTT用户名和密码
  mqttClient.onMessage([this](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {  // 设置接收MQTT数据的回调函数
    this->mqttReceiveCallback(topic, payload, properties, len, index, total);
  });
}

/**
 * 函数功能：绑定属性以及设置接收回调函数
 * 参数1：[_attributeName] [String] 属性名称
 * 参数2：[_region] [poniterReceiveDeserialization] 回调函数
 * 返回值：[bool] true--设置成功，false--设置失败，属性名称为空或者已超过最大设置数量，默认最大为50
 * 注意事项：无
 */
bool AliyunMqtt::bindAttribute(String _attributeName, poniterReceiveDeserialization _cb) {
  for (uint8_t i = 0; i < callbackCountMax; i++) {
    if (_attributeName == "") {  // 如果值为空
      return false;
    }

    if (poniterJsonDeserializationArray[i].key == "" || poniterJsonDeserializationArray[i].key == _attributeName) {  // 判断该key是否为空或者是否已存在该key
      poniterJsonDeserializationArray[i].key = _attributeName;                                                       // 添加属性
      poniterJsonDeserializationArray[i].cb = _cb;                                                                   // 绑定回调函数
      return true;
    }
  }
  return false;
}

/**
 * 函数功能：取消绑定的属性
 * 参数1：[_attributeName] [String] 属性名称
 * 返回值：[bool] true--取消设置成功，false--取消设置失败，不存在该属性名称
 * 注意事项：无
 */
bool AliyunMqtt::unbindAttribute(String _attributeName) {
  for (uint8_t i = 0; i < callbackCountMax; i++) {
    if (poniterJsonDeserializationArray[i].key == _attributeName) {  // 判断属性名称是否相同，如果相同则将该key赋值为空并且回调函数也赋值为NULL
      poniterJsonDeserializationArray[i].key = "";                   // key赋值为空
      poniterJsonDeserializationArray[i].cb = NULL;                  // 回调函数赋值为NULL
      return true;
    }
  }
  return false;
}

/**
 * 函数功能：订阅主题
 * 参数1：[_topicName] [String] 主题名称
 * 参数2：[_cb] [poniterReceiveDeserialization] 回调函数
 * 参数3：[_qos] [uint8_t] qos等级，默认为0
 * 返回值：[uint16_t] 非0--订阅成功，返回数据包ID，为0--订阅失败，主题名称为空或者已超过最大设置数量，默认最大为50
 * 注意事项：无
 */
uint16_t AliyunMqtt::subscribeTopic(String _topicName, poniterReceiveDeserialization _cb, uint8_t _qos) {
  for (uint8_t i = 0; i < callbackCountMax; i++) {
    if (_topicName == "") {  // 如果值为空
      return 0;
    }

    if (poniterJsonDeserializationArray[i].key == "" || poniterJsonDeserializationArray[i].key == _topicName) {  // 判断该key是否为空或者是否已存在该key
      poniterJsonDeserializationArray[i].key = _topicName;                                                       // 添加主题
      poniterJsonDeserializationArray[i].cb = _cb;                                                               // 绑定回调函数
      return mqttClient.subscribe(_topicName.c_str(), _qos);                                                     // 订阅主题，返回订阅结果
    }
  }
  return 0;
}

/**
 * 函数功能：取消订阅主题
 * 参数1：[_topicName] [String] 主题名称
 * 返回值：[uint16_t] 非0--取消订阅成功，返回数据包ID，为0--取消订阅失败，不存在该主题名称
 * 注意事项：无
 */
uint16_t AliyunMqtt::unsubscribeTopic(String _topicName) {
  for (uint8_t i = 0; i < callbackCountMax; i++) {
    if (poniterJsonDeserializationArray[i].key == _topicName) {  // 判断主题名称是否相同，如果相同则将该key赋值为空并且回调函数也赋值为NULL
      poniterJsonDeserializationArray[i].key = "";               // key赋值为NULL
      poniterJsonDeserializationArray[i].cb = NULL;              // 回调函数赋值为NULL
      return mqttClient.unsubscribe(_topicName.c_str());         // 取消订阅主题，返回取消订阅结果
    }
  }
  return 0;
}

/**
 * 函数功能：发送 double 类型数据到云平台，同时兼容float
 * 参数1：[_attributeName] [String] 属性名称
 * 参数2：[_data] [double] 数据，最外层不需要加花括号
 * 参数3：[_decimalPlaces] [uint8_t] 保留小数点后几位，默认为2位
 * 参数4：[_qos] [uint8_t] qos等级，默认为0
 * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
 * 注意事项：无
 */
uint16_t AliyunMqtt::sendAttributeData(String _attributeName, double _data, uint8_t _decimalPlaces, uint8_t _qos) {
  String data = "{\"" + _attributeName + "\":" + String(_data, (unsigned int)_decimalPlaces) + "}";
  char jsonBuffer[1024] = {0};
  sprintf(jsonBuffer, alinkDeviceReportAttributeFormat, data.c_str());
  return sendDataToCloud(OMCT_DeviceAttributeReportingFormat, jsonBuffer, _qos);
}

/**
 * 函数功能：发送 int32_t 类型数据到云平台，同时兼容其他整数类型和bool类型
 * 参数1：[_attributeName] [String] 属性名称
 * 参数2：[_data] [int32_t] 数据，最外层不需要加花括号
 * 参数3：[_qos] [uint8_t] qos等级，默认为0
 * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
 * 注意事项：无
 */
uint16_t AliyunMqtt::sendAttributeData(String _attributeName, int32_t _data, uint8_t _qos) {
  String data = "{\"" + _attributeName + "\":" + String(_data) + "}";
  char jsonBuffer[1024] = {0};
  sprintf(jsonBuffer, alinkDeviceReportAttributeFormat, data.c_str());
  return sendDataToCloud(OMCT_DeviceAttributeReportingFormat, jsonBuffer, _qos);
}

/**
 * 函数功能：发送 String 类型数据到云平台
 * 参数1：[_attributeName] [String] 属性名称
 * 参数2：[_data] [String] 数据，最外层不需要加花括号
 * 参数3：[_qos] [uint8_t] qos等级，默认为0
 * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
 * 注意事项：无
 */
uint16_t AliyunMqtt::sendAttributeData(String _attributeName, String _data, uint8_t _qos) {
  String data = "{\"" + _attributeName + "\":\"" + String(_data) + "\"}";
  char jsonBuffer[1024] = {0};
  sprintf(jsonBuffer, alinkDeviceReportAttributeFormat, data.c_str());
  return sendDataToCloud(OMCT_DeviceAttributeReportingFormat, jsonBuffer, _qos);
}

/**
 * 函数功能：发送多个数据到云平台
 * 参数1：[_data] [String] 数据内容，需要提前拼接好，_data格式为：{"name":"一叶遮天","age":22,"site":"blog.yyzt.site","height":182.34}，仿照这个格式根据实际情况改写，最外层需要加花括号
 * 参数2：[_qos] [uint8_t] qos等级，默认为0
 * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
 * 注意事项：无
 */
uint16_t AliyunMqtt::sendMultipleAttributeData(String _data, uint8_t _qos) {
  char jsonBuffer[1024] = {0};
  sprintf(jsonBuffer, alinkDeviceReportAttributeFormat, _data.c_str());
  return sendDataToCloud(OMCT_DeviceAttributeReportingFormat, jsonBuffer, _qos);
}

/**
 * 函数功能：发送事件到云平台
 * 参数1：[_eventId] [String] 事件ID，即标识符
 * 参数2：[_data] [String] 数据内容，默认为空则发送空数据，最外层需要加花括号
 * 参数3：[_qos] [uint8_t] qos等级，默认为0
 * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
 * 注意事项：无
 */
uint16_t AliyunMqtt::sendEventData(String _eventId, String _data, uint8_t _qos) {
  char topicKey[150] = {0};
  sprintf(topicKey, "%s/%s/post", OMCT_DeviceEventReportingFormat, _eventId.c_str());
  char jsonBuffer[1024] = {0};
  sprintf(jsonBuffer, alinkDeviceReportEventFormat, _data.c_str(), _eventId.c_str());
  return sendDataToCloud(topicKey, jsonBuffer, _qos);
}

/**
 * 函数功能：发送自定义数据格式和内容到云平台
 * 参数1：[_topicFormat] [String] 数据格式，即Topic类，在产品中的Topic类列表中查看
 * 参数2：[_data] [String] 数据内容，默认为空，最外层需要加花括号
 * 参数3：[_qos] [uint8_t] qos等级，默认为0
 * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
 * 注意事项：数据格式不能错误，否则会导致云平台接收或者解析不了，可以参考本代码中的OMCT_DeviceAttributeReportingFormat、OMCT_DeviceEventReportingFormat的写法
 */
uint16_t AliyunMqtt::sendCustomData(String _topicFormat, String _data, uint8_t _qos) {
  return sendDataToCloud(_topicFormat, _data, _qos);
}

/**
 * 函数功能：设置回调函数的最大数量
 * 参数1：[_number] [uint8_t] 数量
 * 返回值：无
 * 注意事项：无
 */
void AliyunMqtt::setCallbackCountMax(uint8_t _number) {
  callbackCountMax = _number;
  poniterJsonDeserializationArray = new poniterJsonDeserialization[callbackCountMax];
}

/**
 * 函数功能：设置调试打印，如果未设置，则不开启打印，如果传入串口，则开启打印
 * 参数1：[_serial] [Stream&] 串口
 * 返回值：无
 * 注意事项：无
 */
void AliyunMqtt::setDebug(Stream& _serial) {
  debugSerial = &_serial;
  debugState = true;
}

/**
 * 函数功能：订阅设备属性设置主题
 * 参数1：[_qos] [uint8_t] qos等级，默认为0
 * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
 * 注意事项：此功能仅订阅设备属性设置主题，即便不调用也不影响设备属性设置数据的下发与接收
 */
uint16_t AliyunMqtt::subscribeAttributeSetting(uint8_t _qos) {
  return mqttClient.subscribe(OMCT_DevicePropertySettingsFormat, _qos);
}

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
void AliyunMqtt::mqttReceiveCallback(char* _topic, char* _payload, AsyncMqttClientMessageProperties _properties, size_t _length, size_t _index, size_t _total) {
  if (_length < 1024) {        // 判断数组是否越界
    _payload[_length] = '\0';  // 将数据中有效数据长度的后一位置空
  }

  if (debugState) {
    debugSerial->println("[接收]到云平台的数据如下：");
    debugSerial->println("Topic为：[" + String(_topic) + "]");
    debugSerial->println("Payload为：[" + String((char*)_payload) + "]");
  }

  StaticJsonDocument<1024> doc;                                 // 定义一个能够存储1024字节的JSON对象
  DeserializationError error = deserializeJson(doc, _payload);  // 反序列化JSON数据
  if (error) {                                                  // 检查反序列化是否成功
    if (debugState) {
      debugSerial->println("Payload数据反序列化失败！");
    }
    return;
  }
  JsonVariant parm = doc.as<JsonVariant>();

  if (String(_topic) == OMCT_DevicePropertySettingsFormat) {  // 判断主题是否相等
    for (uint8_t i = 0; i < callbackCountMax; i++) {
      if (poniterJsonDeserializationArray[i].key && parm["params"].containsKey(poniterJsonDeserializationArray[i].key)) {  // 判断当前回调函数是否是空并且是否存在params键
        poniterJsonDeserializationArray[i].cb(parm["params"]);                                                             // 执行对应的回调函数
      }
    }
    return;
  }

  for (uint8_t i = 0; i < callbackCountMax; i++) {
    if (String(_topic) == poniterJsonDeserializationArray[i].key) {  // 判断主题是否相等
      poniterJsonDeserializationArray[i].cb(parm);                   // 执行对应的回调函数
      return;
    }
  }
}

/**
 * 函数功能：SHA256加密
 * 参数1：[_signContent] [const String&] 签名内容
 * 参数2：[_deviceSecret] [const String&] 设备密钥
 * 返回值：[String] 加密后的字符串
 * 注意事项：无
 */
String AliyunMqtt::hmac256(const String& _signContent, const String& _deviceSecret) {
  uint8_t hashCode[sha256HmacSize] = {0};
  SHA256 sha256;

  const char* key = _deviceSecret.c_str();
  size_t keySize = _deviceSecret.length();

  sha256.resetHMAC(key, keySize);
  sha256.update(_signContent.c_str(), _signContent.length());
  sha256.finalizeHMAC(key, keySize, hashCode, sizeof(hashCode));

  String sign = "";
  for (uint8_t i = 0; i < sha256HmacSize; i++) {
    sign += "0123456789ABCDEF"[hashCode[i] >> 4];
    sign += "0123456789ABCDEF"[hashCode[i] & 0xf];
  }

  return sign;
}

/**
 * 函数功能：发送数据到云平台
 * 参数1：[_topic] [String] 主题
 * 参数2：[_payload] [String] 数据
 * 参数3：[_qos] [uint8_t] qos等级，默认为0
 * 返回值：[uint16_t] 非0--发送成功，返回数据包ID（如果QoS为0，则返回1），为0--发送失败
 * 注意事项：无
 */
uint16_t AliyunMqtt::sendDataToCloud(String _topic, String _payload, uint8_t _qos) {
  if (!mqttClient.connected()) {  // 如果MQTT未连接
    return false;
  }

  if (debugState) {
    debugSerial->println("[发送]到云平台的数据如下：");
    debugSerial->println("Topic为：[" + String(_topic) + "]");
    debugSerial->println("Payload为：[" + _payload + "]");
  }
  return mqttClient.publish(_topic.c_str(), _qos, true, _payload.c_str());
}
