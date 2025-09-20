/**
 * Exemplo Completo - m2mMesh
 *
 * Este exemplo demonstra como usar a biblioteca m2mMesh para criar
 * uma rede mesh entre dispositivos ESP8266/ESP32.
 *
 * Funcionalidades demonstradas:
 * - Inicialização da mesh
 * - Envio de mensagens
 * - Recebimento de mensagens
 * - Monitoramento do status da rede
 * - LED de status integrado
 *
 * Autor: Baseado na biblioteca m2mMesh de Nick Reynolds
 *
 * INSTRUÇÕES DE COMPILAÇÃO:
 * 1. Instale o Arduino IDE
 * 2. Adicione o suporte para ESP8266/ESP32:
 *    - Vá em Arquivo > Preferências
 *    - Em "URLs Adicionais do Gerenciador de Placas" adicione:
 *      ESP8266: http://arduino.esp8266.com/stable/package_esp8266com_index.json
 *      ESP32: https://dl.espressif.com/dl/package_esp32_index.json
 * 3. Vá em Ferramentas > Placa > Gerenciador de Placas
 * 4. Instale "esp8266" ou "ESP32" dependendo da sua placa
 * 5. Copie a pasta m2mMesh para Documentos/Arduino/libraries/
 * 6. Selecione sua placa em Ferramentas > Placa
 * 7. Compile e envie o código
 */

#include <m2mMesh.h>

// Configuração do LED integrado (varia por placa)
#if defined(LED_BUILTIN)
#if defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_ESP8266_WEMOS_D1MINIPRO) || defined(ARDUINO_Pocket32)
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif
#endif

// Variáveis de controle
bool joinedMesh = false;
uint8_t numberOfNodes = 0;
uint8_t numberOfReachableNodes = 0;
unsigned long lastMessageTime = 0;
unsigned long messageInterval = 10000; // Enviar mensagem a cada 10 segundos
int messageCounter = 0;

void setup()
{
// Configurar LED integrado
#if defined(LED_BUILTIN)
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_OFF);
#endif

    // Inicializar comunicação serial
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== m2mMesh - Exemplo Completo ===");
    Serial.println("Inicializando a rede mesh...");

    // Definir nome do nó (opcional, mas recomendado)
    String nodeName = "Node_" + String(random(1000, 9999));
    m2mMesh.setNodeName(nodeName);
    Serial.println("Nome do nó: " + nodeName);

    // Inicializar a mesh com até 16 nós no canal 1
    if (m2mMesh.begin(16, 1))
    {
        Serial.print("Mesh iniciada com sucesso no canal: ");
        Serial.println(WiFi.channel());
        Serial.print("Endereço MAC: ");

        // Exibir endereço MAC
        uint8_t *mac = m2mMesh.getMeshAddress();
        for (int i = 0; i < 6; i++)
        {
            if (mac[i] < 16)
                Serial.print("0");
            Serial.print(mac[i], HEX);
            if (i < 5)
                Serial.print(":");
        }
        Serial.println();

        // Habilitar crescimento dinâmico da mesh
        m2mMesh.enableDynamicGrowth(4);
        Serial.println("Crescimento dinâmico habilitado");

        // Configurar callback para eventos da mesh (opcional)
        m2mMesh.setCallback(onMeshEvent);
    }
    else
    {
        Serial.println("ERRO: Falha ao inicializar a mesh!");
        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("Aguardando conexão com outros nós...");
}

void loop()
{
    // IMPORTANTE: Deve ser chamado pelo menos uma vez por segundo
    m2mMesh.housekeeping();

    // Verificar se entrou ou saiu da mesh
    checkMeshStatus();

    // Verificar mudanças no número de nós
    checkNodeCount();

    // Enviar mensagem periodicamente
    sendPeriodicMessage();

    // Verificar mensagens recebidas
    checkIncomingMessages();

    // Pequeno delay para não sobrecarregar
    delay(100);
}

void checkMeshStatus()
{
    // Verificar se acabou de entrar na mesh
    if (joinedMesh == false && m2mMesh.joined() == true)
    {
        joinedMesh = true;
#if defined(LED_BUILTIN)
        digitalWrite(LED_BUILTIN, LED_ON);
#endif
        Serial.println("✓ CONECTADO à mesh!");
        Serial.println("Agora pode enviar e receber mensagens");
    }
    // Verificar se saiu da mesh
    else if (joinedMesh == true && m2mMesh.joined() == false)
    {
        joinedMesh = false;
#if defined(LED_BUILTIN)
        digitalWrite(LED_BUILTIN, LED_OFF);
#endif
        Serial.println("✗ DESCONECTADO da mesh");
    }
}

void checkNodeCount()
{
    uint8_t currentNodes = m2mMesh.numberOfNodes();
    uint8_t currentReachableNodes = m2mMesh.numberOfReachableNodes();

    if (numberOfNodes != currentNodes || numberOfReachableNodes != currentReachableNodes)
    {
        numberOfNodes = currentNodes;
        numberOfReachableNodes = currentReachableNodes;

        Serial.print("Nós na rede: ");
        Serial.print(numberOfNodes);
        Serial.print(" | Nós alcançáveis: ");
        Serial.print(numberOfReachableNodes);

        if (m2mMesh.stable())
        {
            Serial.println(" | Status: ESTÁVEL");
        }
        else
        {
            Serial.println(" | Status: MUDANDO");
        }
    }
}

void sendPeriodicMessage()
{
    // Só enviar se estiver conectado à mesh
    if (!joinedMesh)
        return;

    unsigned long currentTime = millis();
    if (currentTime - lastMessageTime >= messageInterval)
    {
        lastMessageTime = currentTime;
        messageCounter++;

        // Criar mensagem
        String message = "Olá da mesh! Mensagem #" + String(messageCounter);
        message += " de " + String(m2mMesh.getNodeName());

        // Limpar mensagem anterior (se houver)
        m2mMesh.clearMessage();

        // Adicionar dados à mensagem
        m2mMesh.add(message);
        m2mMesh.add(messageCounter); // Adicionar contador como inteiro
        m2mMesh.add(millis());       // Adicionar timestamp

        // Enviar para todos os nós (broadcast)
        // Para enviar para um nó específico, use: m2mMesh.destination(nodeId);

        if (m2mMesh.send())
        {
            Serial.println("📤 Mensagem enviada: " + message);
        }
        else
        {
            Serial.println("❌ Falha ao enviar mensagem");
            Serial.print("Erro: ");
            Serial.println(m2mMesh.lastErrorDescription());
        }
    }
}

void checkIncomingMessages()
{
    // Verificar se há mensagens aguardando
    while (m2mMesh.messageWaiting())
    {
        Serial.println("📥 Mensagem recebida!");

        // Obter informações do remetente
        uint8_t senderId = m2mMesh.sourceId();
        uint8_t senderMac[6];
        m2mMesh.sourceMacAddress(senderMac);

        Serial.print("   De: Nó ID ");
        Serial.print(senderId);
        Serial.print(" (");
        for (int i = 0; i < 6; i++)
        {
            if (senderMac[i] < 16)
                Serial.print("0");
            Serial.print(senderMac[i], HEX);
            if (i < 5)
                Serial.print(":");
        }
        Serial.println(")");

        // Processar dados da mensagem
        while (m2mMesh.dataAvailable() > 0)
        {
            uint8_t dataType = m2mMesh.nextDataType();

            switch (dataType)
            {
            case m2mMesh.USR_DATA_STRING:
            {
                String receivedText = m2mMesh.retrieveString();
                Serial.println("   Texto: " + receivedText);
            }
            break;

            case m2mMesh.USR_DATA_UINT32_T:
            {
                uint32_t receivedNumber = m2mMesh.retrieveUint32_t();
                Serial.println("   Número: " + String(receivedNumber));
            }
            break;

            case m2mMesh.USR_DATA_INT32_T:
            {
                int32_t receivedInt = m2mMesh.retrieveInt32_t();
                Serial.println("   Inteiro: " + String(receivedInt));
            }
            break;

            default:
                // Tipo de dado não reconhecido, pular
                m2mMesh.skipRetrieve();
                Serial.println("   Tipo de dado não reconhecido");
                break;
            }
        }

        // Marcar mensagem como lida
        m2mMesh.markMessageRead();
        Serial.println("   ✓ Mensagem processada");
    }
}

// Função de callback para eventos da mesh (opcional)
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
        Serial.println("📨 Evento: Nova mensagem disponível");
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
 * COMANDOS SERIAIS ÚTEIS (adicione no loop se desejar):
 *
 * - Digite "info" para ver informações da mesh
 * - Digite "send <mensagem>" para enviar uma mensagem específica
 * - Digite "trace <nodeId>" para fazer trace para um nó
 */

void processSerialCommands()
{
    if (Serial.available())
    {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command == "info")
        {
            printMeshInfo();
        }
        else if (command.startsWith("send "))
        {
            String msg = command.substring(5);
            m2mMesh.clearMessage();
            m2mMesh.add(msg);
            if (m2mMesh.send())
            {
                Serial.println("Mensagem enviada: " + msg);
            }
            else
            {
                Serial.println("Falha ao enviar");
            }
        }
        else if (command.startsWith("trace "))
        {
            uint8_t nodeId = command.substring(6).toInt();
            if (m2mMesh.trace(nodeId))
            {
                Serial.print("Trace para nó ");
                Serial.print(nodeId);
                Serial.print(": ");
                Serial.print(m2mMesh.traceTime());
                Serial.print("ms, ");
                Serial.print(m2mMesh.traceHops());
                Serial.println(" saltos");
            }
        }
    }
}

void printMeshInfo()
{
    Serial.println("=== Informações da Mesh ===");
    Serial.print("Nome do nó: ");
    Serial.println(m2mMesh.getNodeName());
    Serial.print("Conectado: ");
    Serial.println(m2mMesh.joined() ? "Sim" : "Não");
    Serial.print("Estável: ");
    Serial.println(m2mMesh.stable() ? "Sim" : "Não");
    Serial.print("Total de nós: ");
    Serial.println(m2mMesh.numberOfNodes());
    Serial.print("Nós alcançáveis: ");
    Serial.println(m2mMesh.numberOfReachableNodes());

#ifdef m2mMeshIncludeMeshInfoFeatures
    Serial.print("Tempo da mesh: ");
    Serial.print(m2mMesh.syncedMillis());
    Serial.println("ms");
    Serial.print("Sincronizado: ");
    Serial.println(m2mMesh.synced() ? "Sim" : "Não");
#endif

    Serial.println("========================");
}
