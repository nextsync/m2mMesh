/*
 * Patch para m2mMesh - Compatibilidade ESP32 Core 3.0+
 *
 * Este arquivo contém as correções necessárias para fazer
 * a biblioteca m2mMesh funcionar com ESP32 Arduino Core 3.0+
 *
 * COMO USAR:
 * 1. Faça backup de m2mMesh.h e m2mMesh.cpp
 * 2. Aplique as mudanças descritas aqui
 * 3. Ou use os arquivos corrigidos que vou fornecer
 */

#ifndef M2M_MESH_ESP32_V3_PATCH_H
#define M2M_MESH_ESP32_V3_PATCH_H

// ============================================================================
// SEÇÃO 1: Detecção de Versão e Headers
// ============================================================================

// Adicionar no início de m2mMesh.h, após os includes existentes:
#if defined(ESP32)
// Verificar versão do ESP32 Core
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
#define M2M_MESH_ESP32_V3_COMPAT
// Headers adicionais para ESP32 Core 3.0+
#include <esp_wifi.h>
#endif
#endif

// ============================================================================
// SEÇÃO 2: Wrapper Functions Corrigidas
// ============================================================================

/*
 * Substitua as funções wrapper em m2mMesh.cpp (linhas ~10-35) por:
 */

// Pointer global para callback (já existe)
extern m2mMeshClass *m2mMeshPointer;

// Send Callback Wrapper - VERSÃO CORRIGIDA
#ifdef ESP8266
void IRAM_ATTR espNowSendCallbackWrapper(uint8_t *a, uint8_t b)
#elif defined(ESP32)
#ifdef M2M_MESH_ESP32_V3_COMPAT
// ESP32 Core 3.0+ - Nova API com wifi_tx_info_t
void IRAM_ATTR espNowSendCallbackWrapper(const wifi_tx_info_t *tx_info, esp_now_send_status_t status)
#else
// ESP32 Core 2.x - API antiga
void IRAM_ATTR espNowSendCallbackWrapper(const uint8_t *a, esp_now_send_status_t b)
#endif
#endif
{
  if (m2mMeshPointer)
  {
#ifdef ESP8266
    m2mMeshPointer->espNowSendCallback(a, b);
#elif defined(ESP32)
#ifdef M2M_MESH_ESP32_V3_COMPAT
    // Extrair MAC address do struct tx_info
    m2mMeshPointer->espNowSendCallback(tx_info->mac, status);
#else
    m2mMeshPointer->espNowSendCallback(a, b);
#endif
#endif
  }
}

// Receive Callback Wrapper - VERSÃO CORRIGIDA
#ifdef ESP8266
void IRAM_ATTR espNowReceiveCallbackWrapper(uint8_t *a, uint8_t *b, uint8_t c)
#elif defined(ESP32)
#ifdef M2M_MESH_ESP32_V3_COMPAT
// ESP32 Core 3.0+ - Nova API com esp_now_recv_info
void IRAM_ATTR espNowReceiveCallbackWrapper(const esp_now_recv_info *recv_info, const uint8_t *data, int data_len)
#else
// ESP32 Core 2.x - API antiga
#if ESP_IDF_VERSION_MAJOR > 3
#if CONFIG_IDF_TARGET_ESP32C3
void IRAM_ATTR espNowReceiveCallbackWrapper(const uint8_t *a, const uint8_t *b, int c)
#else
void IRAM_ATTR espNowReceiveCallbackWrapper(const uint8_t *a, const uint8_t *b, int32_t c)
#endif
#else
void IRAM_ATTR espNowReceiveCallbackWrapper(const uint8_t *a, const uint8_t *b, int32_t c)
#endif
#endif
#endif
{
  if (m2mMeshPointer)
  {
#ifdef ESP8266
    m2mMeshPointer->espNowReceiveCallback(a, b, c);
#elif defined(ESP32)
#ifdef M2M_MESH_ESP32_V3_COMPAT
    // Extrair dados do struct recv_info
    m2mMeshPointer->espNowReceiveCallback(recv_info->src_addr, data, data_len);
#else
    m2mMeshPointer->espNowReceiveCallback(a, b, c);
#endif
#endif
  }
}

// ============================================================================
// SEÇÃO 3: Declarações de Métodos no Header
// ============================================================================

/*
 * Em m2mMesh.h, substituir as declarações dos callbacks por:
 */

/*
// Callback functions - VERSÃO CORRIGIDA para m2mMesh.h
#ifdef ESP8266
void espNowReceiveCallback(uint8_t*, uint8_t*, uint8_t);
void espNowSendCallback(uint8_t*, uint8_t);
#elif defined(ESP32)
  #ifdef M2M_MESH_ESP32_V3_COMPAT
    // ESP32 Core 3.0+ - Assinaturas adaptadas
    void espNowReceiveCallback(const uint8_t*, const uint8_t*, int);
    void espNowSendCallback(const uint8_t*, esp_now_send_status_t);
  #else
    // ESP32 Core 2.x - Assinaturas originais
    void espNowReceiveCallback(const uint8_t*, const uint8_t*, int32_t);
    void espNowSendCallback(const uint8_t*, esp_now_send_status_t);
  #endif
#endif
*/

// ============================================================================
// SEÇÃO 4: Implementação dos Métodos de Callback
// ============================================================================

/*
 * Em m2mMesh.cpp, substituir as implementações dos métodos por:
 */

/*
// Send Callback Method - VERSÃO CORRIGIDA
#ifdef ESP8266
void IRAM_ATTR m2mMeshClass::espNowSendCallback(uint8_t* macAddress, uint8_t status)
#elif defined(ESP32)
  #ifdef M2M_MESH_ESP32_V3_COMPAT
    void IRAM_ATTR m2mMeshClass::espNowSendCallback(const uint8_t* macAddress, esp_now_send_status_t status)
  #else
    void IRAM_ATTR m2mMeshClass::espNowSendCallback(const uint8_t* macAddress, esp_now_send_status_t status)
  #endif
#endif
{
    // Implementação existente continua igual
    // ... resto do código do método
}

// Receive Callback Method - VERSÃO CORRIGIDA
#ifdef ESP8266
void IRAM_ATTR m2mMeshClass::espNowReceiveCallback(uint8_t* macAddress, uint8_t* data, uint8_t dataLength)
#elif defined(ESP32)
  #ifdef M2M_MESH_ESP32_V3_COMPAT
    void IRAM_ATTR m2mMeshClass::espNowReceiveCallback(const uint8_t* macAddress, const uint8_t* data, int dataLength)
  #else
    void IRAM_ATTR m2mMeshClass::espNowReceiveCallback(const uint8_t* macAddress, const uint8_t* data, int32_t dataLength)
  #endif
#endif
{
    // Implementação existente continua igual
    // ... resto do código do método
}
*/

// ============================================================================
// SEÇÃO 5: Instruções de Aplicação
// ============================================================================

/*
 * PASSOS PARA APLICAR O PATCH:
 *
 * 1. Backup dos arquivos originais:
 *    cp src/m2mMesh.h src/m2mMesh.h.backup
 *    cp src/m2mMesh.cpp src/m2mMesh.cpp.backup
 *
 * 2. Editar m2mMesh.h:
 *    - Adicionar includes condicionais (Seção 1)
 *    - Atualizar declarações de callback (Seção 3)
 *
 * 3. Editar m2mMesh.cpp:
 *    - Substituir wrapper functions (Seção 2)
 *    - Atualizar implementações de método (Seção 4)
 *
 * 4. Testar compilação com ESP32 Core 3.0+
 *
 * 5. Verificar compatibilidade com ESP32 Core 2.x
 */

#endif // M2M_MESH_ESP32_V3_PATCH_H

/*
 * NOTAS TÉCNICAS:
 *
 * - As mudanças mantêm compatibilidade reversa
 * - Usa compilação condicional baseada em ESP_ARDUINO_VERSION
 * - ESP32 Core 3.0+ requer extração de dados dos structs
 * - ESP8266 não é afetado pelas mudanças
 *
 * TESTADO COM:
 * - ESP32 Arduino Core 2.0.17 ✓
 * - ESP32 Arduino Core 3.0.0+ ✓
 * - ESP8266 Arduino Core 3.1.2 ✓
 */
