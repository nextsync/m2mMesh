/**
 * Exemplo ESP8266 - m2mMesh
 *
 * Este exemplo foi otimizado especificamente para ESP8266
 * (NodeMCU, Wemos D1 Mini, etc.) pois é mais compatível
 * com a biblioteca m2mMesh.
 *
 * HARDWARE RECOMENDADO:
 * - NodeMCU v3 (ESP-12E)
 * - Wemos D1 Mini
 * - ESP8266 genérico
 *
 * CONFIGURAÇÃO DO ARDUINO IDE:
 * 1. Instale o suporte ESP8266:
 *    - Arquivo > Preferências
 *    - URLs Adicionais: http://arduino.esp8266.com/stable/package_esp8266com_index.json
 * 2. Ferramentas > Gerenciador de Placas > Instale "esp8266"
 * 3. Selecione sua placa:
 *    - NodeMCU 1.0 (ESP-12E Module)
 *    - LOLIN(WEMOS) D1 R2 & mini
 * 4. Configurações recomendadas:
 *    - CPU Frequency: 80 MHz
 *    - Flash Size: 4MB (FS:2MB OTA:~1019KB)
 *    - Upload Speed: 921600
 */

#include <m2mMesh.h>

// LED integrado no ESP8266 (geralmente GPIO2 ou GPIO16)
#define LED_PIN LED_BUILTIN
#define LED_ON LOW // No ESP8266, LED acende com LOW
#define LED_OFF HIGH

// Variáveis globais
bool meshConnected = false;
unsigned long lastMessage = 0;
unsigned long lastStatusCheck = 0;
int messageCount = 0;

void setup()
{
    // Configurar LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_OFF);

    // Configurar Serial
    Serial.begin(115200);
    delay(1000);

    // Limpar serial e mostrar informações
    Serial.println("\n\n" + String('=', 50));
    Serial.println("    m2mMesh ESP8266 - Exemplo Funcional");
    Serial.println(String('=', 50));

    // Mostrar informações do chip
    Serial.println("Informações do ESP8266:");
    Serial.printf("  Chip ID: %08X\n", ESP.getChipId());
    Serial.printf("  Frequência: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("  RAM livre: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  Flash: %d bytes\n", ESP.getFlashChipSize());

    // Criar nome único baseado no Chip ID
    String nodeName = "ESP8266_" + String(ESP.getChipId(), HEX);
    nodeName.toUpperCase();

    Serial.println("\nConfigurando m2mMesh...");
    Serial.println("Nome do nó: " + nodeName);

    // Configurar nome do nó
    if (m2mMesh.setNodeName(nodeName))
    {
        Serial.println("✓ Nome configurado com sucesso");
    }
    else
    {
        Serial.println("⚠ Aviso: Falha ao configurar nome");
    }

    // Inicializar mesh
    Serial.println("Iniciando mesh...");
    if (m2mMesh.begin(8, 1))
    { // Máximo 8 nós, canal 1
        Serial.println("✓ Mesh iniciada com sucesso!");
        Serial.printf("  Canal WiFi: %d\n", WiFi.channel());

        // Mostrar MAC address
        Serial.print("  MAC Address: ");
        Serial.println(WiFi.macAddress());

        // Configurar para crescimento dinâmico (opcional)
        m2mMesh.enableDynamicGrowth(2);
        Serial.println("✓ Crescimento dinâmico habilitado");
    }
    else
    {
        Serial.println("✗ ERRO: Falha ao inicializar mesh!");
        Serial.println("Verifique se você está usando ESP8266");
        Serial.println("Reiniciando em 5 segundos...");
        delay(5000);
        ESP.restart();
    }

    Serial.println("\nAguardando outros dispositivos...");
    Serial.println("Carregue este código em outros ESP8266 para formar a mesh");
    Serial.println(String('-', 50));
}

void loop()
{
    // CRÍTICO: Chamar housekeeping
    m2mMesh.housekeeping();

    // Verificar status da conexão
    checkMeshConnection();

    // Mostrar status periodicamente
    showPeriodicStatus();

    // Enviar mensagem automática
    sendAutomaticMessage();

    // Processar mensagens recebidas
    processReceivedMessages();

    // Yield para o ESP8266 (evita watchdog reset)
    yield();
    delay(50);
}

void checkMeshConnection()
{
    bool currentStatus = m2mMesh.joined();

    if (!meshConnected && currentStatus)
    {
        // Acabou de conectar
        meshConnected = true;
        digitalWrite(LED_PIN, LED_ON);

        Serial.println("\n🎉 CONECTADO à mesh!");
        Serial.println("LED aceso = conectado à mesh");
        Serial.println("Agora posso comunicar com outros dispositivos");
    }
    else if (meshConnected && !currentStatus)
    {
        // Desconectou
        meshConnected = false;
        digitalWrite(LED_PIN, LED_OFF);

        Serial.println("\n😞 DESCONECTADO da mesh");
        Serial.println("LED apagado = sem conexão");
    }
}

void showPeriodicStatus()
{
    unsigned long now = millis();

    // Mostrar status a cada 30 segundos
    if (now - lastStatusCheck >= 30000)
    {
        lastStatusCheck = now;

        Serial.println("\n" + String('-', 30));
        Serial.println("STATUS DA MESH:");
        Serial.printf("  Conectado: %s\n", meshConnected ? "SIM" : "NÃO");
        Serial.printf("  Nós total: %d\n", m2mMesh.numberOfNodes());
        Serial.printf("  Nós alcançáveis: %d\n", m2mMesh.numberOfReachableNodes());
        Serial.printf("  Mesh estável: %s\n", m2mMesh.stable() ? "SIM" : "NÃO");
        Serial.printf("  RAM livre: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("  Uptime: %lu segundos\n", millis() / 1000);
        Serial.println(String('-', 30));
    }
}

void sendAutomaticMessage()
{
    // Só enviar se conectado
    if (!meshConnected)
        return;

    unsigned long now = millis();

    // Enviar mensagem a cada 20 segundos
    if (now - lastMessage >= 20000)
    {
        lastMessage = now;
        messageCount++;

        // Criar mensagem
        String msg = "Olá! Sou " + String(m2mMesh.getNodeName()) +
                     " - Msg #" + String(messageCount) +
                     " - RAM: " + String(ESP.getFreeHeap()) + " bytes";

        // Limpar buffer e enviar
        m2mMesh.clearMessage();
        m2mMesh.add(msg);
        m2mMesh.add((uint32_t)messageCount);
        m2mMesh.add((uint32_t)ESP.getFreeHeap());

        if (m2mMesh.send())
        {
            Serial.println("📤 ENVIADO: " + msg);
        }
        else
        {
            Serial.println("❌ Falha ao enviar mensagem");
            Serial.print("   Erro: ");
            Serial.println(m2mMesh.lastErrorDescription());
        }
    }
}

void processReceivedMessages()
{
    while (m2mMesh.messageWaiting())
    {
        Serial.println("\n📥 MENSAGEM RECEBIDA:");

        // Informações do remetente
        uint8_t senderId = m2mMesh.sourceId();
        uint8_t senderMac[6];
        if (m2mMesh.sourceMacAddress(senderMac))
        {
            Serial.printf("   De: Nó %d (", senderId);
            for (int i = 0; i < 6; i++)
            {
                Serial.printf("%02X", senderMac[i]);
                if (i < 5)
                    Serial.print(":");
            }
            Serial.println(")");
        }

        // Processar dados
        int fieldCount = 0;
        while (m2mMesh.dataAvailable() > 0)
        {
            fieldCount++;
            uint8_t dataType = m2mMesh.nextDataType();

            Serial.printf("   Campo %d: ", fieldCount);

            switch (dataType)
            {
            case m2mMesh.USR_DATA_STRING:
            {
                String text = m2mMesh.retrieveString();
                Serial.println("\"" + text + "\"");
            }
            break;

            case m2mMesh.USR_DATA_UINT32_T:
            {
                uint32_t number = m2mMesh.retrieveUint32_t();
                Serial.println(String(number));
            }
            break;

            case m2mMesh.USR_DATA_INT32_T:
            {
                int32_t number = m2mMesh.retrieveInt32_t();
                Serial.println(String(number));
            }
            break;

            default:
                Serial.println("tipo não reconhecido");
                m2mMesh.skipRetrieve();
                break;
            }
        }

        m2mMesh.markMessageRead();
        Serial.println("   ✓ Processada");
    }
}

/*
 * DICAS PARA USAR:
 *
 * 1. Hardware:
 *    - Use ESP8266 (NodeMCU, Wemos D1 Mini)
 *    - ESP32 pode ter problemas com versões recentes
 *
 * 2. Configuração:
 *    - Placa: "NodeMCU 1.0 (ESP-12E Module)" ou "LOLIN(WEMOS) D1 R2 & mini"
 *    - Upload Speed: 921600
 *    - CPU Frequency: 80 MHz
 *
 * 3. Teste:
 *    - Carregue em pelo menos 2 ESP8266
 *    - Mantenha próximos (< 10 metros) inicialmente
 *    - Observe LED integrado acender quando conectar
 *    - Veja mensagens no Serial Monitor (115200 baud)
 *
 * 4. Troubleshooting:
 *    - Se não conectar: aguarde até 60 segundos
 *    - Se não compilar: verifique versão ESP8266 core
 *    - Se resetar constantemente: problema de alimentação
 *
 * 5. Expansão:
 *    - Adicione mais ESP8266 para rede maior
 *    - Teste range afastando dispositivos
 *    - Implemente aplicações específicas
 */
