/**
 * Exemplo ESP32 Core 3.0+ - m2mMesh
 *
 * Este exemplo foi adaptado para funcionar com ESP32 Arduino Core 3.0+
 * Inclui as correções necessárias para a nova API do ESP-NOW.
 *
 * HARDWARE TESTADO:
 * - ESP32 DevKit v1
 * - ESP32 WROOM-32
 * - ESP32-S3
 * - ESP32-C3
 *
 * CONFIGURAÇÃO ARDUINO IDE:
 * 1. ESP32 Arduino Core 3.0.0 ou superior
 * 2. Placa: "ESP32 Dev Module" ou similar
 * 3. Configurações:
 *    - CPU Frequency: 240MHz
 *    - Flash Frequency: 80MHz
 *    - Flash Size: 4MB
 *    - Upload Speed: 921600
 *
 * ANTES DE USAR:
 * 1. Aplique o patch em m2mMesh.h e m2mMesh.cpp (veja m2mMesh_ESP32_v3_patch.h)
 * 2. Ou use ESP32 Core 2.0.17 (mais estável)
 * 3. Ou use ESP8266 (recomendado para iniciantes)
 */

#include <m2mMesh.h>

// Verificar se a biblioteca foi corrigida para ESP32 Core 3.0+
#if defined(ESP32) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
#ifndef M2M_MESH_ESP32_V3_COMPAT
#warning "ATENÇÃO: m2mMesh pode não funcionar corretamente com ESP32 Core 3.0+ sem o patch!"
#warning "Veja o arquivo m2mMesh_ESP32_v3_patch.h para instruções de correção."
#endif
#endif

// Configuração do LED integrado
#define LED_PIN LED_BUILTIN
#ifdef LED_BUILTIN
#define LED_ON HIGH
#define LED_OFF LOW
#else
#define LED_PIN 2 // GPIO2 na maioria dos ESP32
#define LED_ON HIGH
#define LED_OFF LOW
#endif

// Variáveis de controle
bool meshConnected = false;
unsigned long lastMessage = 0;
unsigned long lastStatus = 0;
int messageCounter = 0;

void setup()
{
  // Configurar LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);

  // Configurar Serial
  Serial.begin(115200);
  delay(2000); // ESP32 precisa de mais tempo para inicializar

  // Mostrar informações do sistema
  printSystemInfo();

  // Verificar versão do Arduino Core
  checkArduinoCoreVersion();

  // Configurar m2mMesh
  setupMesh();

  Serial.println("\n" + String('=', 50));
  Serial.println("Sistema iniciado - aguardando conexões...");
  Serial.println("Carregue este código em outros ESP32 para formar a mesh");
  Serial.println(String('=', 50));
}

void loop()
{
  // CRÍTICO: Chamar housekeeping
  m2mMesh.housekeeping();

  // Verificar status da conexão
  checkConnection();

  // Mostrar status periodicamente
  showStatus();

  // Enviar mensagem automática
  sendMessage();

  // Processar mensagens recebidas
  processMessages();

  // Delay para evitar sobrecarregar
  delay(100);
}

void printSystemInfo()
{
  Serial.println("\n" + String('=', 60));
  Serial.println("         m2mMesh ESP32 Core 3.0+ - Exemplo Adaptado");
  Serial.println(String('=', 60));
  Serial.println("Informações do Sistema:");

  // Informações do ESP32
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  Serial.printf("  Chip: %s\n", ESP.getChipModel());
  Serial.printf("  Revisão: %d\n", chip_info.revision);
  Serial.printf("  Núcleos: %d\n", chip_info.cores);
  Serial.printf("  CPU: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("  RAM: %d KB\n", ESP.getHeapSize() / 1024);
  Serial.printf("  RAM livre: %d KB\n", ESP.getFreeHeap() / 1024);
  Serial.printf("  Flash: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("  MAC WiFi: %s\n", WiFi.macAddress().c_str());
}

void checkArduinoCoreVersion()
{
  Serial.println("\nVersão do Arduino Core:");
  Serial.printf("  ESP32 Core: %d.%d.%d\n",
                ESP_ARDUINO_VERSION_MAJOR,
                ESP_ARDUINO_VERSION_MINOR,
                ESP_ARDUINO_VERSION_PATCH);

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  Serial.println("  ✓ ESP32 Core 3.0+ detectado");
#ifdef M2M_MESH_ESP32_V3_COMPAT
  Serial.println("  ✓ Patch m2mMesh aplicado - compatível!");
#else
  Serial.println("  ⚠ ATENÇÃO: Patch m2mMesh não detectado!");
  Serial.println("  → Veja m2mMesh_ESP32_v3_patch.h para instruções");
#endif
#else
  Serial.println("  ✓ ESP32 Core 2.x - deveria funcionar normalmente");
#endif
}

void setupMesh()
{
  Serial.println("\nConfigurando m2mMesh...");

  // Criar nome único baseado no MAC
  String macAddr = WiFi.macAddress();
  macAddr.replace(":", "");
  String nodeName = "ESP32_" + macAddr.substring(6); // Últimos 6 caracteres

  Serial.println("Nome do nó: " + nodeName);

  // Configurar nome
  if (m2mMesh.setNodeName(nodeName))
  {
    Serial.println("✓ Nome configurado");
  }
  else
  {
    Serial.println("⚠ Falha ao configurar nome");
  }

// Configurar debugging (opcional)
#ifdef m2mMeshIncludeDebugFeatures
  m2mMesh.enableDebugging(Serial, m2mMesh.MESH_UI_LOG_WARNINGS | m2mMesh.MESH_UI_LOG_ERRORS);
  Serial.println("✓ Debug habilitado");
#endif

  // Inicializar mesh
  Serial.println("Iniciando mesh...");
  if (m2mMesh.begin(12, 1))
  { // Máximo 12 nós, canal 1
    Serial.println("✓ Mesh iniciada com sucesso!");
    Serial.printf("  Canal: %d\n", WiFi.channel());

    // Configurações avançadas
    m2mMesh.enableDynamicGrowth(4);
    Serial.println("✓ Crescimento dinâmico habilitado");

    // Callback de eventos (opcional)
    m2mMesh.setCallback(onMeshEvent);
    Serial.println("✓ Callback configurado");
  }
  else
  {
    Serial.println("✗ ERRO: Falha ao inicializar mesh!");
    Serial.println("\nPossíveis causas:");
    Serial.println("1. Biblioteca m2mMesh não foi corrigida para ESP32 Core 3.0+");
    Serial.println("2. Problema de hardware");
    Serial.println("3. Configuração incorreta");
    Serial.println("\nSoluções:");
    Serial.println("1. Aplique o patch (veja m2mMesh_ESP32_v3_patch.h)");
    Serial.println("2. Use ESP32 Core 2.0.17");
    Serial.println("3. Use ESP8266 (mais estável)");

    // Piscar LED para indicar erro
    while (true)
    {
      digitalWrite(LED_PIN, LED_ON);
      delay(200);
      digitalWrite(LED_PIN, LED_OFF);
      delay(200);
    }
  }
}

void checkConnection()
{
  bool currentStatus = m2mMesh.joined();

  if (!meshConnected && currentStatus)
  {
    // Conectou
    meshConnected = true;
    digitalWrite(LED_PIN, LED_ON);

    Serial.println("\n🎉 CONECTADO à mesh!");
    Serial.println("✓ LED aceso = conectado");
    Serial.println("✓ Pronto para comunicar");
  }
  else if (meshConnected && !currentStatus)
  {
    // Desconectou
    meshConnected = false;
    digitalWrite(LED_PIN, LED_OFF);

    Serial.println("\n😞 DESCONECTADO da mesh");
    Serial.println("✗ LED apagado = sem conexão");
  }
}

void showStatus()
{
  unsigned long now = millis();

  if (now - lastStatus >= 25000)
  { // A cada 25 segundos
    lastStatus = now;

    Serial.println("\n" + String('-', 40));
    Serial.println("STATUS ATUAL:");
    Serial.printf("  Conectado: %s\n", meshConnected ? "SIM" : "NÃO");
    Serial.printf("  Nós total: %d\n", m2mMesh.numberOfNodes());
    Serial.printf("  Nós alcançáveis: %d\n", m2mMesh.numberOfReachableNodes());
    Serial.printf("  Mesh estável: %s\n", m2mMesh.stable() ? "SIM" : "NÃO");
    Serial.printf("  RAM livre: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("  Uptime: %lu s\n", millis() / 1000);

#ifdef m2mMeshIncludeMeshInfoFeatures
    if (m2mMesh.synced())
    {
      Serial.printf("  Tempo mesh: %lu ms\n", m2mMesh.syncedMillis());
    }
#endif

    Serial.println(String('-', 40));
  }
}

void sendMessage()
{
  if (!meshConnected)
    return;

  unsigned long now = millis();
  if (now - lastMessage >= 18000)
  { // A cada 18 segundos
    lastMessage = now;
    messageCounter++;

    // Criar mensagem rica em informações
    String msg = "ESP32 " + String(m2mMesh.getNodeName()) +
                 " - Msg #" + String(messageCounter);

    m2mMesh.clearMessage();
    m2mMesh.add(msg);
    m2mMesh.add((uint32_t)messageCounter);
    m2mMesh.add((uint32_t)ESP.getFreeHeap());
    m2mMesh.add((uint32_t)ESP.getCpuFreqMHz());

    if (m2mMesh.send())
    {
      Serial.println("📤 ENVIADO: " + msg);
    }
    else
    {
      Serial.println("❌ Falha ao enviar");
      Serial.println("   Erro: " + String(m2mMesh.lastErrorDescription()));
    }
  }
}

void processMessages()
{
  while (m2mMesh.messageWaiting())
  {
    Serial.println("\n📥 MENSAGEM RECEBIDA:");

    // Info do remetente
    uint8_t senderId = m2mMesh.sourceId();
    uint8_t senderMac[6];

    Serial.printf("   De: Nó %d", senderId);
    if (m2mMesh.sourceMacAddress(senderMac))
    {
      Serial.print(" (");
      for (int i = 0; i < 6; i++)
      {
        Serial.printf("%02X", senderMac[i]);
        if (i < 5)
          Serial.print(":");
      }
      Serial.println(")");
    }

    // Processar dados
    int fieldNum = 0;
    while (m2mMesh.dataAvailable() > 0)
    {
      fieldNum++;
      uint8_t dataType = m2mMesh.nextDataType();

      Serial.printf("   [%d] ", fieldNum);

      switch (dataType)
      {
      case m2mMesh.USR_DATA_STRING:
      {
        String text = m2mMesh.retrieveString();
        Serial.println("Texto: \"" + text + "\"");
      }
      break;

      case m2mMesh.USR_DATA_UINT32_T:
      {
        uint32_t num = m2mMesh.retrieveUint32_t();
        Serial.printf("Número: %lu\n", num);
      }
      break;

      default:
        Serial.println("Tipo desconhecido");
        m2mMesh.skipRetrieve();
        break;
      }
    }

    m2mMesh.markMessageRead();
    Serial.println("   ✓ Mensagem processada");
  }
}

// Callback para eventos da mesh
void onMeshEvent(meshEvent event)
{
  switch (event)
  {
  case meshEvent::joined:
    Serial.println("🔗 Evento: Entrou na mesh");
    break;

  case meshEvent::left:
    Serial.println("🔌 Evento: Saiu da mesh");
    break;

  case meshEvent::stable:
    Serial.println("✅ Evento: Mesh estabilizada");
    break;

  case meshEvent::changing:
    Serial.println("🔄 Evento: Mesh mudando");
    break;

  case meshEvent::message:
    Serial.println("📨 Evento: Nova mensagem");
    break;

  case meshEvent::synced:
    Serial.println("⏰ Evento: Tempo sincronizado");
    break;

  default:
    Serial.println("❓ Evento desconhecido");
    break;
  }
}

/*
 * INSTRUÇÕES IMPORTANTES:
 *
 * 1. ANTES DE COMPILAR:
 *    - Aplique o patch em m2mMesh (veja m2mMesh_ESP32_v3_patch.h)
 *    - Ou use ESP32 Core 2.0.17 (mais fácil)
 *    - Ou use ESP8266 (mais estável)
 *
 * 2. CONFIGURAÇÃO RECOMENDADA:
 *    - ESP32 Dev Module
 *    - CPU: 240MHz
 *    - Flash: 4MB
 *    - Upload Speed: 921600
 *
 * 3. TESTE:
 *    - Carregue em 2+ ESP32
 *    - Monitor Serial: 115200 baud
 *    - LED acende quando conecta
 *    - Mensagens automáticas a cada 18s
 *
 * 4. TROUBLESHOOTING:
 *    - Se não compilar: patch não aplicado
 *    - Se não conectar: aguarde 60s
 *    - Se resetar: problema alimentação
 *    - Se instável: use ESP8266
 */
