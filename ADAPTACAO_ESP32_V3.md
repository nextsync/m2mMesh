# 🚀 Adaptando m2mMesh para ESP32 Core 3.0+

## 📋 Mudanças Necessárias

A principal diferença é que a API do ESP-NOW mudou significativamente no ESP32 Arduino Core 3.0+. Vou mostrar exatamente quais mudanças fazer.

## 🔧 Arquivo 1: Atualizar m2mMesh.cpp

### Problema 1: Callback Functions
As funções de callback do ESP-NOW mudaram suas assinaturas.

**LOCALIZAÇÃO:** `/src/m2mMesh.cpp` - linhas 10-35

**CÓDIGO ATUAL:**
```cpp
#ifdef ESP8266
void IRAM_ATTR espNowSendCallbackWrapper(uint8_t* a, uint8_t b)
#elif defined(ESP32)
void IRAM_ATTR espNowSendCallbackWrapper(const uint8_t *a, esp_now_send_status_t b)
#endif

#ifdef ESP8266
	void IRAM_ATTR espNowReceiveCallbackWrapper(uint8_t *a, uint8_t *b, uint8_t c)
#elif defined(ESP32)
	void IRAM_ATTR espNowReceiveCallbackWrapper(const uint8_t *a, const uint8_t *b, int32_t c)
#endif
```

**CÓDIGO ATUALIZADO:**
```cpp
#ifdef ESP8266
void IRAM_ATTR espNowSendCallbackWrapper(uint8_t* a, uint8_t b)
#elif defined(ESP32)
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    // ESP32 Core 3.0+ - Nova API
    void IRAM_ATTR espNowSendCallbackWrapper(const wifi_tx_info_t *tx_info, esp_now_send_status_t status)
  #else
    // ESP32 Core 2.x - API antiga
    void IRAM_ATTR espNowSendCallbackWrapper(const uint8_t *a, esp_now_send_status_t b)
  #endif
#endif
{
    if (m2mMeshPointer) {
      #if defined(ESP8266)
        m2mMeshPointer->espNowSendCallback(a, b);
      #elif defined(ESP32)
        #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
          // Para ESP32 Core 3.0+, extrair MAC do tx_info
          m2mMeshPointer->espNowSendCallback(tx_info->mac, status);
        #else
          m2mMeshPointer->espNowSendCallback(a, b);
        #endif
      #endif
    }
}

#ifdef ESP8266
  void IRAM_ATTR espNowReceiveCallbackWrapper(uint8_t *a, uint8_t *b, uint8_t c)
#elif defined(ESP32)
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    // ESP32 Core 3.0+ - Nova API
    void IRAM_ATTR espNowReceiveCallbackWrapper(const esp_now_recv_info *recv_info, const uint8_t *data, int data_len)
  #else
    // ESP32 Core 2.x - API antiga
    void IRAM_ATTR espNowReceiveCallbackWrapper(const uint8_t *a, const uint8_t *b, int32_t c)
  #endif
#endif
{
    if (m2mMeshPointer) {
      #if defined(ESP8266)
        m2mMeshPointer->espNowReceiveCallback(a, b, c);
      #elif defined(ESP32)
        #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
          // Para ESP32 Core 3.0+, extrair MAC do recv_info
          m2mMeshPointer->espNowReceiveCallback(recv_info->src_addr, data, data_len);
        #else
          m2mMeshPointer->espNowReceiveCallback(a, b, c);
        #endif
      #endif
    }
}
```

### Problema 2: Headers
**LOCALIZAÇÃO:** `/src/m2mMesh.h` - linha 37

**CÓDIGO ATUAL:**
```cpp
extern "C" {
#include <esp_now.h>
}
```

**CÓDIGO ATUALIZADO:**
```cpp
extern "C" {
#include <esp_now.h>
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
#include <esp_wifi.h>
#endif
}
```

### Problema 3: Callback Method Signatures
**LOCALIZAÇÃO:** `/src/m2mMesh.h` - linhas nas declarações públicas

**CÓDIGO ATUAL:**
```cpp
#ifdef ESP8266
void espNowReceiveCallback(uint8_t*, uint8_t*, uint8_t);
void espNowSendCallback(uint8_t*, uint8_t);
#elif defined(ESP32)
void espNowReceiveCallback(const uint8_t* , const uint8_t*, int32_t);
void espNowSendCallback(const uint8_t*, esp_now_send_status_t);
#endif
```

**CÓDIGO ATUALIZADO:**
```cpp
#ifdef ESP8266
void espNowReceiveCallback(uint8_t*, uint8_t*, uint8_t);
void espNowSendCallback(uint8_t*, uint8_t);
#elif defined(ESP32)
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    // ESP32 Core 3.0+ - métodos que aceitam dados extraídos dos structs
    void espNowReceiveCallback(const uint8_t*, const uint8_t*, int);
    void espNowSendCallback(const uint8_t*, esp_now_send_status_t);
  #else
    // ESP32 Core 2.x - API antiga
    void espNowReceiveCallback(const uint8_t*, const uint8_t*, int32_t);
    void espNowSendCallback(const uint8_t*, esp_now_send_status_t);
  #endif
#endif
```

### Problema 4: Implementação dos Métodos
**LOCALIZAÇÃO:** `/src/m2mMesh.cpp` - onde estão implementados os métodos de callback

**CÓDIGO ATUAL:**
```cpp
#ifdef ESP8266
void IRAM_ATTR m2mMeshClass::espNowSendCallback(uint8_t* macAddress, uint8_t status)
#elif defined(ESP32)
void IRAM_ATTR m2mMeshClass::espNowSendCallback(const uint8_t* macAddress, esp_now_send_status_t status)
#endif

#ifdef ESP8266
void IRAM_ATTR m2mMeshClass::espNowReceiveCallback(uint8_t* macAddress, uint8_t* data, uint8_t dataLength)
#elif defined(ESP32)
void IRAM_ATTR m2mMeshClass::espNowReceiveCallback(const uint8_t* macAddress, const uint8_t* data, int32_t dataLength)
#endif
```

**CÓDIGO ATUALIZADO:**
```cpp
#ifdef ESP8266
void IRAM_ATTR m2mMeshClass::espNowSendCallback(uint8_t* macAddress, uint8_t status)
#elif defined(ESP32)
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    void IRAM_ATTR m2mMeshClass::espNowSendCallback(const uint8_t* macAddress, esp_now_send_status_t status)
  #else
    void IRAM_ATTR m2mMeshClass::espNowSendCallback(const uint8_t* macAddress, esp_now_send_status_t status)
  #endif
#endif

#ifdef ESP8266
void IRAM_ATTR m2mMeshClass::espNowReceiveCallback(uint8_t* macAddress, uint8_t* data, uint8_t dataLength)
#elif defined(ESP32)
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    void IRAM_ATTR m2mMeshClass::espNowReceiveCallback(const uint8_t* macAddress, const uint8_t* data, int dataLength)
  #else
    void IRAM_ATTR m2mMeshClass::espNowReceiveCallback(const uint8_t* macAddress, const uint8_t* data, int32_t dataLength)
  #endif
#endif
```

## 🛠️ Arquivo Patch Completo

Vou criar um arquivo com todas as mudanças necessárias:

### m2mMesh_ESP32_v3_patch.h
```cpp
/*
 * Patch para ESP32 Arduino Core 3.0+
 * Adicione este include no início de m2mMesh.h
 */
#if defined(ESP32) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  #define M2M_MESH_ESP32_V3_COMPAT
  #include <esp_wifi.h>
#endif

// No m2mMesh.cpp, substitua as funções wrapper por:
#ifdef ESP8266
void IRAM_ATTR espNowSendCallbackWrapper(uint8_t* a, uint8_t b)
#elif defined(ESP32)
  #ifdef M2M_MESH_ESP32_V3_COMPAT
    void IRAM_ATTR espNowSendCallbackWrapper(const wifi_tx_info_t *tx_info, esp_now_send_status_t status)
  #else
    void IRAM_ATTR espNowSendCallbackWrapper(const uint8_t *a, esp_now_send_status_t b)
  #endif
#endif
{
    if (m2mMeshPointer) {
      #ifdef ESP8266
        m2mMeshPointer->espNowSendCallback(a, b);
      #elif defined(ESP32)
        #ifdef M2M_MESH_ESP32_V3_COMPAT
          m2mMeshPointer->espNowSendCallback(tx_info->mac, status);
        #else
          m2mMeshPointer->espNowSendCallback(a, b);
        #endif
      #endif
    }
}

#ifdef ESP8266
  void IRAM_ATTR espNowReceiveCallbackWrapper(uint8_t *a, uint8_t *b, uint8_t c)
#elif defined(ESP32)
  #ifdef M2M_MESH_ESP32_V3_COMPAT
    void IRAM_ATTR espNowReceiveCallbackWrapper(const esp_now_recv_info *recv_info, const uint8_t *data, int data_len)
  #else
    void IRAM_ATTR espNowReceiveCallbackWrapper(const uint8_t *a, const uint8_t *b, int32_t c)
  #endif
#endif
{
    if (m2mMeshPointer) {
      #ifdef ESP8266
        m2mMeshPointer->espNowReceiveCallback(a, b, c);
      #elif defined(ESP32)
        #ifdef M2M_MESH_ESP32_V3_COMPAT
          m2mMeshPointer->espNowReceiveCallback(recv_info->src_addr, data, data_len);
        #else
          m2mMeshPointer->espNowReceiveCallback(a, b, c);
        #endif
      #endif
    }
}
```

## 🎯 Resumo das Mudanças

### 1. **API Changes** (Mais Crítico)
- **ESP32 Core 3.0+** usa `wifi_tx_info_t*` e `esp_now_recv_info*` nos callbacks
- **ESP32 Core 2.x** usa `const uint8_t*` diretamente

### 2. **Headers** 
- Adicionar `#include <esp_wifi.h>` para ESP32 Core 3.0+

### 3. **Conditional Compilation**
- Usar `ESP_ARDUINO_VERSION` para detectar versão
- Compilação condicional para ambas as APIs

### 4. **Data Extraction**
- ESP32 3.0+: extrair MAC de `tx_info->mac` e `recv_info->src_addr`
- ESP32 2.x: usar ponteiros diretamente

## 🚀 Implementação Prática

Para implementar essas mudanças:

1. **Backup** dos arquivos originais
2. **Aplicar** as mudanças nos arquivos de source
3. **Testar** com ESP32 Core 3.0+
4. **Verificar** compatibilidade reversa com 2.x

Essas são as principais mudanças necessárias. A complexidade vem do fato de que a API mudou substancialmente na estrutura dos callbacks, mas com essas modificações o código deve funcionar tanto nas versões antigas quanto nas novas!
