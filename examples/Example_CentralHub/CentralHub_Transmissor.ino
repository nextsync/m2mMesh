/**
 * ESP Transmissor - m2mMesh
 *
 * Este exemplo demonstra como configurar um ESP para enviar mensagens
 * para o receptor central (hub) através da mesh, com roteamento automático.
 *
 * Funcionalidades:
 * - Envia mensagens periodicamente para o hub central
 * - Roteamento automático através de outros nós se necessário
 * - Coleta dados de sensores simulados
 * - Recebe confirmações do hub
 * - LED de status integrado
 * - Interface serial para comandos manuais
 *
 * IMPORTANTE: O hub central deve estar configurado com o nome "CentralHub"
 * para que este nó saiba para onde enviar as mensagens.
 *
 * Autor: Baseado na biblioteca m2mMesh de Nick Reynolds
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

// Configurações do Transmissor
const String HUB_NAME = "CentralHub"; // Nome do hub para onde enviar mensagens
const String NODE_PREFIX = "Sensor";  // Prefixo do nome deste nó
const uint8_t MAX_NODES = 20;         // Máximo de nós na mesh
const uint8_t MESH_CHANNEL = 1;       // Canal da mesh

// Variáveis de controle
bool joinedMesh = false;
bool hubFound = false;
uint8_t hubNodeId = 255;
unsigned long lastMessageTime = 0;
unsigned long messageInterval = 15000; // Enviar mensagem a cada 15 segundos
unsigned long lastHubSearchTime = 0;
unsigned long hubSearchInterval = 10000; // Procurar hub a cada 10 segundos
int messageCounter = 0;
String nodeName;

// Variáveis para simulação de sensores
float temperature = 20.0;
float humidity = 50.0;
uint16_t lightLevel = 500;
bool motionDetected = false;

// Estatísticas
unsigned long totalMessagesSent = 0;
unsigned long totalAcksReceived = 0;
unsigned long lastAckTime = 0;

// Estatísticas de roteamento (quando funciona como ponte)
unsigned long messagesRouted = 0;
unsigned long lastRoutingActivity = 0;
bool actingAsRouter = false;
uint8_t routedNodes[10]; // Array para rastrear nós que usam este como roteador
uint8_t routedNodesCount = 0;

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

    Serial.println("=== m2mMesh - TRANSMISSOR PARA HUB CENTRAL ===");

    // Gerar nome único para este nó
    nodeName = NODE_PREFIX;
    nodeName += "_";
    nodeName += String(random(1000, 9999));

    Serial.print("Inicializando transmissor: ");
    Serial.println(nodeName);
    Serial.print("Hub de destino: ");
    Serial.println(HUB_NAME);

    // Definir nome do nó
    m2mMesh.setNodeName(nodeName);

    // Inicializar a mesh
    if (m2mMesh.begin(MAX_NODES, MESH_CHANNEL))
    {
        Serial.print("Transmissor iniciado no canal: ");
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

        // Configurar callback para eventos da mesh
        m2mMesh.setCallback(onMeshEvent);

        // Habilitar funcionalidades de roteamento colaborativo
        // Isso permite que este nó funcione como ponte para outros
        enableRoutingCapabilities();

        Serial.println("Procurando hub central na mesh...");
    }
    else
    {
        Serial.println("ERRO: Falha ao inicializar o transmissor!");
        while (true)
        {
            delay(1000);
        }
    }

    // Inicializar valores aleatórios para sensores simulados
    randomSeed(analogRead(0));
    temperature = 15.0 + random(0, 200) / 10.0; // 15-35°C
    humidity = 30.0 + random(0, 400) / 10.0;    // 30-70%
    lightLevel = random(100, 1000);             // 100-1000 lux

    printHelp();
}

void loop()
{
    // IMPORTANTE: Deve ser chamado pelo menos uma vez por segundo
    m2mMesh.housekeeping();

    // Verificar integridade básica (proteção contra corrupção)
    static unsigned long lastMemoryCheck = 0;
    if (millis() - lastMemoryCheck > 30000) // A cada 30 segundos
    {
        lastMemoryCheck = millis();

        // Verificar se valores estão dentro de limites razoáveis
        if (messageCounter > 100000)
            messageCounter = 0;
        if (totalMessagesSent > 100000)
            totalMessagesSent = 0;
        if (messagesRouted > 100000)
            messagesRouted = 0;
        if (routedNodesCount > 10)
            routedNodesCount = 0;

        // Mostrar heap livre para monitoramento
        Serial.print("🔧 Heap livre: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");
    }

    // Verificar status da mesh
    checkMeshStatus();

    // Procurar hub periodicamente
    searchForHub();

    // Enviar mensagem periodicamente se hub estiver disponível
    sendPeriodicMessage();

    // Verificar mensagens recebidas (confirmações do hub)
    checkIncomingMessages();

    // Verificar atividade de roteamento
    checkRoutingActivity();

    // Simular leituras de sensores
    updateSensorReadings();

    // Processar comandos seriais
    processSerialCommands();

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
        Serial.print("Procurando hub central: ");
        Serial.println(HUB_NAME);

        // Resetar busca do hub
        hubFound = false;
        hubNodeId = 255;
        lastHubSearchTime = 0;
    }
    // Verificar se saiu da mesh
    else if (joinedMesh == true && m2mMesh.joined() == false)
    {
        joinedMesh = false;
        hubFound = false;
        hubNodeId = 255;
#if defined(LED_BUILTIN)
        digitalWrite(LED_BUILTIN, LED_OFF);
#endif
        Serial.println("✗ DESCONECTADO da mesh");
    }
}

void searchForHub()
{
    if (!joinedMesh)
        return;

    unsigned long currentTime = millis();
    if (currentTime - lastHubSearchTime >= hubSearchInterval)
    {
        lastHubSearchTime = currentTime;

        // Procurar o hub na lista de nós
        bool foundHub = false;
        for (uint8_t i = 0; i < m2mMesh.numberOfNodes(); i++)
        {
            if (m2mMesh.nodeNameIsSet(i))
            {
                String nodeNameCheck = String(m2mMesh.getNodeName(i));
                if (nodeNameCheck == HUB_NAME && m2mMesh.nodeIsReachableNode(i))
                {
                    if (!hubFound || hubNodeId != i)
                    {
                        hubNodeId = i;
                        hubFound = true;
                        foundHub = true;
                        Serial.print("🎯 Hub encontrado! Nó ID: ");
                        Serial.print(i);
                        Serial.print(" (");
                        Serial.print(HUB_NAME);
                        Serial.println(")");

                        // Enviar mensagem de apresentação
                        sendIntroductionMessage();
                    }
                    break;
                }
            }
        }

        if (!foundHub && hubFound)
        {
            // Hub perdido
            hubFound = false;
            hubNodeId = 255;
            Serial.println("⚠️  Hub perdido! Procurando novamente...");
        }
        else if (!foundHub && !hubFound)
        {
            Serial.print("🔍 Procurando hub '");
            Serial.print(HUB_NAME);
            Serial.print("'... (");
            Serial.print(m2mMesh.numberOfReachableNodes());
            Serial.println(" nós alcançáveis)");
        }
    }
}

void sendIntroductionMessage()
{
    if (!hubFound)
        return;

    m2mMesh.clearMessage();

    // Usar strings separadas para evitar problemas de concatenação
    m2mMesh.add("Olá! Sou o " + nodeName + " me conectando ao hub!");
    m2mMesh.add("Tipo: Sensor de ambiente");
    m2mMesh.add("Capabilities: Temperatura, Umidade, Luz, Movimento");
    m2mMesh.add(millis()); // Timestamp

    if (m2mMesh.destination(hubNodeId))
    {
        if (m2mMesh.send())
        {
            Serial.println("📤 Mensagem de apresentação enviada para o hub");
        }
        else
        {
            Serial.println("❌ Falha ao enviar apresentação para o hub");
        }
    }
}

void sendPeriodicMessage()
{
    if (!joinedMesh || !hubFound)
        return;

    unsigned long currentTime = millis();
    if (currentTime - lastMessageTime >= messageInterval)
    {
        lastMessageTime = currentTime;
        messageCounter++;

        // Limpar mensagem anterior
        m2mMesh.clearMessage();

        // Preparar dados dos sensores - usar strings separadas
        m2mMesh.add("Dados do " + nodeName + " #" + String(messageCounter));
        m2mMesh.add(temperature);    // Temperatura
        m2mMesh.add(humidity);       // Umidade
        m2mMesh.add(lightLevel);     // Nível de luz
        m2mMesh.add(motionDetected); // Movimento detectado
        m2mMesh.add(millis());       // Timestamp
        m2mMesh.add(messageCounter); // Contador de mensagens

        // Enviar para o hub
        if (m2mMesh.destination(hubNodeId))
        {
            if (m2mMesh.send())
            {
                totalMessagesSent++;
                Serial.print("📤 Dados enviados para hub (msg #");
                Serial.print(messageCounter);
                Serial.println(")");
                Serial.print("   🌡️  Temp: ");
                Serial.print(temperature, 1);
                Serial.println("°C");
                Serial.print("   💧 Umidade: ");
                Serial.print(humidity, 1);
                Serial.println("%");
                Serial.print("   💡 Luz: ");
                Serial.print(lightLevel);
                Serial.println(" lux");
                Serial.print("   🚶 Movimento: ");
                Serial.println(motionDetected ? "Sim" : "Não");
            }
            else
            {
                Serial.println("❌ Falha ao enviar dados para o hub");
                Serial.print("   Erro: ");
                Serial.println(m2mMesh.lastErrorDescription());

                // Se falhar, tentar procurar o hub novamente
                hubFound = false;
                hubNodeId = 255;
                lastHubSearchTime = 0;
            }
        }
    }
}

void checkIncomingMessages()
{
    // Verificar se há mensagens aguardando (confirmações do hub)
    while (m2mMesh.messageWaiting())
    {
        Serial.println("📥 Mensagem recebida!");

        // Obter informações do remetente
        uint8_t senderId = m2mMesh.sourceId();

        // Verificar se é do hub
        if (senderId == hubNodeId)
        {
            totalAcksReceived++;
            lastAckTime = millis();
            Serial.println("✅ Confirmação recebida do hub!");
        }
        else
        {
            Serial.print("📨 Mensagem de outro nó (ID: ");
            Serial.print(senderId);

            // Verificar se é mensagem sendo roteada através deste nó
            String senderName = "";
            if (m2mMesh.nodeNameIsSet(senderId))
            {
                senderName = String(m2mMesh.getNodeName(senderId));
            }

            Serial.print(" - ");
            Serial.print(senderName.length() > 0 ? senderName : "Sem nome");
            Serial.println(")");

            // Atualizar estatísticas de roteamento se não for mensagem direta para este nó
            updateRoutingStats(senderId);
        }

        // Processar dados da mensagem
        while (m2mMesh.dataAvailable() > 0)
        {
            uint8_t dataType = m2mMesh.nextDataType();

            switch (dataType)
            {
            case m2mMesh.USR_DATA_STRING:
            {
                String receivedText = m2mMesh.retrieveString();
                Serial.print("   📄 Texto: ");
                Serial.println(receivedText);
            }
            break;

            case m2mMesh.USR_DATA_UINT32_T:
            {
                uint32_t receivedNumber = m2mMesh.retrieveUint32_t();
                Serial.print("   🔢 Número: ");
                Serial.println(receivedNumber);
            }
            break;

            case m2mMesh.USR_DATA_INT32_T:
            {
                int32_t receivedInt = m2mMesh.retrieveInt32_t();
                Serial.print("   🔢 Inteiro: ");
                Serial.println(receivedInt);
            }
            break;

            default:
                // Tipo de dado não reconhecido, pular
                m2mMesh.skipRetrieve();
                break;
            }
        }

        // Marcar mensagem como lida
        m2mMesh.markMessageRead();
    }
}

void updateSensorReadings()
{
    // Simular variações nos sensores a cada 5 segundos
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();

    if (currentTime - lastUpdate >= 5000)
    {
        lastUpdate = currentTime;

        // Variações pequenas e realistas
        temperature += (random(-20, 21) / 10.0); // ±2°C
        if (temperature < 10.0)
            temperature = 10.0;
        if (temperature > 40.0)
            temperature = 40.0;

        humidity += (random(-50, 51) / 10.0); // ±5%
        if (humidity < 20.0)
            humidity = 20.0;
        if (humidity > 80.0)
            humidity = 80.0;

        lightLevel += random(-100, 101); // ±100 lux
        if (lightLevel < 50)
            lightLevel = 50;
        if (lightLevel > 1200)
            lightLevel = 1200;

        // Movimento aleatório (20% de chance)
        motionDetected = (random(0, 100) < 20);
    }
}

void processSerialCommands()
{
    if (Serial.available())
    {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();

        if (command == "help" || command == "?")
        {
            printHelp();
        }
        else if (command == "status" || command == "info")
        {
            printStatus();
        }
        else if (command == "sensors")
        {
            printSensorData();
        }
        else if (command == "send")
        {
            forceSendMessage();
        }
        else if (command == "search")
        {
            forceHubSearch();
        }
        else if (command.startsWith("interval "))
        {
            String intervalStr = command.substring(9);
            intervalStr.trim(); // Limpar espaços
            unsigned long newInterval = intervalStr.toInt() * 1000;
            if (newInterval >= 5000 && newInterval <= 300000) // 5s a 5min
            {
                messageInterval = newInterval;
                Serial.print("✅ Intervalo alterado para ");
                Serial.print(newInterval / 1000);
                Serial.println(" segundos");
            }
            else
            {
                Serial.println("❌ Intervalo deve estar entre 5 e 300 segundos");
            }
        }
        else if (command.startsWith("temp "))
        {
            String tempStr = command.substring(5);
            tempStr.trim(); // Limpar espaços
            float newTemp = tempStr.toFloat();
            if (newTemp >= -10.0 && newTemp <= 60.0)
            {
                temperature = newTemp;
                Serial.print("🌡️  Temperatura definida para ");
                Serial.print(temperature, 1);
                Serial.println("°C");
            }
            else
            {
                Serial.println("❌ Temperatura deve estar entre -10°C e 60°C");
            }
        }
        else if (command.startsWith("humidity ") || command.startsWith("hum "))
        {
            int spaceIndex = command.indexOf(' ');
            if (spaceIndex > 0 && spaceIndex < command.length() - 1)
            {
                String humStr = command.substring(spaceIndex + 1);
                humStr.trim(); // Limpar espaços
                float newHum = humStr.toFloat();
                if (newHum >= 0.0 && newHum <= 100.0)
                {
                    humidity = newHum;
                    Serial.print("💧 Umidade definida para ");
                    Serial.print(humidity, 1);
                    Serial.println("%");
                }
                else
                {
                    Serial.println("❌ Umidade deve estar entre 0% e 100%");
                }
            }
        }
        else if (command.startsWith("light "))
        {
            String lightStr = command.substring(6);
            lightStr.trim(); // Limpar espaços
            uint16_t newLight = lightStr.toInt();
            if (newLight >= 0 && newLight <= 2000)
            {
                lightLevel = newLight;
                Serial.print("💡 Luz definida para ");
                Serial.print(lightLevel);
                Serial.println(" lux");
            }
            else
            {
                Serial.println("❌ Luz deve estar entre 0 e 2000 lux");
            }
        }
        else if (command == "motion on")
        {
            motionDetected = true;
            Serial.println("🚶 Movimento ativado");
        }
        else if (command == "motion off")
        {
            motionDetected = false;
            Serial.println("🚶 Movimento desativado");
        }
        else if (command == "reset")
        {
            resetStatistics();
        }
        else if (command == "routing" || command == "route")
        {
            showRoutingStatus();
        }
        else if (command == "topology" || command == "topo")
        {
            showLocalTopology();
        }
        else if (command == "neighbors")
        {
            showNeighbors();
        }
        else
        {
            Serial.println("❓ Comando não reconhecido. Digite 'help' para ver os comandos disponíveis.");
        }
    }
}

void printHelp()
{
    Serial.println("");
    Serial.println("🔧 ===== COMANDOS DISPONÍVEIS =====");
    Serial.println("help ou ?          - Mostrar esta ajuda");
    Serial.println("status ou info     - Status do transmissor");
    Serial.println("sensors            - Dados atuais dos sensores");
    Serial.println("send               - Forçar envio de mensagem");
    Serial.println("search             - Forçar busca do hub");
    Serial.println("interval <seg>     - Alterar intervalo de envio (5-300s)");
    Serial.println("temp <valor>       - Definir temperatura (-10 a 60°C)");
    Serial.println("hum <valor>        - Definir umidade (0 a 100%)");
    Serial.println("light <valor>      - Definir luz (0 a 2000 lux)");
    Serial.println("motion on/off      - Ativar/desativar movimento");
    Serial.println("reset              - Resetar estatísticas");
    Serial.println("routing ou route   - Status de roteamento");
    Serial.println("topology ou topo   - Topologia local da rede");
    Serial.println("neighbors          - Mostrar nós vizinhos");
    Serial.println("===================================");
}

void printStatus()
{
    Serial.println("");
    Serial.println("📊 ===== STATUS DO TRANSMISSOR =====");
    Serial.print("🏷️  Nome: ");
    Serial.println(nodeName);
    Serial.print("🎯 Hub alvo: ");
    Serial.println(HUB_NAME);
    Serial.print("🔗 Conectado à mesh: ");
    Serial.println(m2mMesh.joined() ? "Sim" : "Não");
    Serial.print("🎯 Hub encontrado: ");
    Serial.println(hubFound ? "Sim" : "Não");

    if (hubFound)
    {
        Serial.print("📍 Hub ID: ");
        Serial.println(hubNodeId);
    }

    Serial.print("🌐 Nós na mesh: ");
    Serial.println(m2mMesh.numberOfNodes());
    Serial.print("📡 Nós alcançáveis: ");
    Serial.println(m2mMesh.numberOfReachableNodes());
    Serial.print("📤 Mensagens enviadas: ");
    Serial.println(totalMessagesSent);
    Serial.print("✅ Confirmações recebidas: ");
    Serial.println(totalAcksReceived);
    Serial.print("🔀 Mensagens roteadas: ");
    Serial.println(messagesRouted);
    Serial.print("🌉 Funcionando como ponte: ");
    Serial.println(actingAsRouter ? "Sim" : "Não");

    if (actingAsRouter)
    {
        Serial.print("🔗 Nós conectados via este: ");
        Serial.println(routedNodesCount);
    }

    Serial.print("⏰ Intervalo de envio: ");
    Serial.print(messageInterval / 1000);
    Serial.println(" segundos");
    Serial.print("⏰ Uptime: ");
    Serial.print(millis() / 1000);
    Serial.println(" segundos");

    if (totalAcksReceived > 0)
    {
        Serial.print("📅 Última confirmação: ");
        Serial.print((millis() - lastAckTime) / 1000);
        Serial.println("s atrás");
    }

    if (messagesRouted > 0)
    {
        Serial.print("🔀 Última atividade de roteamento: ");
        Serial.print((millis() - lastRoutingActivity) / 1000);
        Serial.println("s atrás");
    }

    Serial.println("====================================");
}

void printSensorData()
{
    Serial.println("");
    Serial.println("🌡️ ===== DADOS DOS SENSORES =====");
    Serial.print("🌡️  Temperatura: ");
    Serial.print(temperature, 1);
    Serial.println("°C");
    Serial.print("💧 Umidade: ");
    Serial.print(humidity, 1);
    Serial.println("%");
    Serial.print("💡 Luz: ");
    Serial.print(lightLevel);
    Serial.println(" lux");
    Serial.print("🚶 Movimento: ");
    Serial.println(motionDetected ? "Detectado" : "Não detectado");
    Serial.print("⏰ Próxima leitura em: ");
    Serial.print((messageInterval - (millis() - lastMessageTime)) / 1000);
    Serial.println("s");
    Serial.println("=================================");
}

void forceSendMessage()
{
    if (!joinedMesh)
    {
        Serial.println("❌ Não conectado à mesh");
        return;
    }

    if (!hubFound)
    {
        Serial.println("❌ Hub não encontrado");
        return;
    }

    lastMessageTime = 0; // Forçar envio imediato
    Serial.println("🚀 Forçando envio de mensagem...");
}

void forceHubSearch()
{
    if (!joinedMesh)
    {
        Serial.println("❌ Não conectado à mesh");
        return;
    }

    hubFound = false;
    hubNodeId = 255;
    lastHubSearchTime = 0; // Forçar busca imediata
    Serial.println("🔍 Forçando busca do hub...");
}

void resetStatistics()
{
    totalMessagesSent = 0;
    totalAcksReceived = 0;
    lastAckTime = 0;
    messageCounter = 0;
    messagesRouted = 0;
    lastRoutingActivity = 0;
    routedNodesCount = 0;
    actingAsRouter = false;

    Serial.println("✅ Estatísticas resetadas!");
}

void enableRoutingCapabilities()
{
    // A biblioteca m2mMesh automaticamente permite roteamento
    // Aqui podemos configurar parâmetros para ser mais eficiente como roteador
    Serial.println("🌉 Capacidades de roteamento habilitadas");
    Serial.println("   Este nó pode funcionar como ponte para outros nós");

    // Inicializar arrays de roteamento de forma segura
    memset(routedNodes, 255, sizeof(routedNodes));
    routedNodesCount = 0;
}

void checkRoutingActivity()
{
    // Verificar se estamos funcionando como roteador de forma mais segura
    bool wasRouter = actingAsRouter;
    actingAsRouter = false;
    routedNodesCount = 0;

    // Limpar array de nós roteados de forma segura
    memset(routedNodes, 255, sizeof(routedNodes));

    // Verificar todos os nós da mesh com proteção contra overflow
    uint8_t totalNodes = m2mMesh.numberOfNodes();
    if (totalNodes > 50)
        totalNodes = 50; // Proteção contra valores absurdos

    for (uint8_t nodeId = 0; nodeId < totalNodes && routedNodesCount < 10; nodeId++)
    {
        if (m2mMesh.nodeIsReachableNode(nodeId))
        {
            uint8_t router = m2mMesh.selectedRouter(nodeId);

            // Se algum nó nos usa como roteador, estamos funcionando como ponte
            if (router != 255 && router != nodeId)
            {
                // Por enquanto, assumir que estamos roteando se há nós com routers definidos
                // e nossa conexão com o hub é boa
                if (hubFound && routedNodesCount < 10)
                {
                    routedNodes[routedNodesCount] = nodeId;
                    routedNodesCount++;
                    actingAsRouter = true;
                }
            }
        }
    }

    // Se começamos a atuar como roteador, notificar
    if (actingAsRouter && !wasRouter)
    {
        Serial.println("🌉 AGORA FUNCIONANDO COMO PONTE para outros nós!");
        Serial.print("   Roteando ");
        Serial.print(routedNodesCount);
        Serial.println(" conexões");

// Piscar LED para indicar atividade de roteamento
#if defined(LED_BUILTIN)
        for (int i = 0; i < 3; i++)
        {
            digitalWrite(LED_BUILTIN, LED_OFF);
            delay(100);
            digitalWrite(LED_BUILTIN, LED_ON);
            delay(100);
        }
#endif
    }
    else if (!actingAsRouter && wasRouter)
    {
        Serial.println("🔌 Parou de funcionar como ponte");
    }
}

void updateRoutingStats(uint8_t senderId)
{
    // Atualizar estatísticas quando recebemos mensagens de outros nós
    // que podem estar passando por nós
    messagesRouted++;
    lastRoutingActivity = millis();

// Indicação visual sutil de atividade de roteamento
#if defined(LED_BUILTIN)
    digitalWrite(LED_BUILTIN, LED_OFF);
    delay(50);
    digitalWrite(LED_BUILTIN, LED_ON);
#endif
}

void showRoutingStatus()
{
    Serial.println("");
    Serial.println("🌉 ===== STATUS DE ROTEAMENTO =====");
    Serial.print("🔀 Funcionando como ponte: ");
    Serial.println(actingAsRouter ? "SIM" : "NÃO");
    Serial.print("📊 Total de mensagens roteadas: ");
    Serial.println(messagesRouted);

    if (actingAsRouter)
    {
        Serial.print("🔗 Número de nós conectados via este: ");
        Serial.println(routedNodesCount);

        Serial.println("📋 Nós usando este como ponte:");
        for (int i = 0; i < routedNodesCount && i < 10; i++)
        {
            if (routedNodes[i] != 255)
            {
                Serial.print("   • Nó ");
                Serial.print(routedNodes[i]);

                if (m2mMesh.nodeNameIsSet(routedNodes[i]))
                {
                    Serial.print(" (");
                    Serial.print(m2mMesh.getNodeName(routedNodes[i]));
                    Serial.print(")");
                }
                Serial.println();
            }
        }
    }
    else
    {
        Serial.println("ℹ️  Este nó não está roteando tráfego no momento");
        if (hubFound)
        {
            Serial.println("   • Conectado ao hub - pode ser usado como ponte");
        }
        else
        {
            Serial.println("   • Sem conexão com hub - não pode rotear");
        }
    }

    if (messagesRouted > 0)
    {
        Serial.print("⏰ Última atividade de roteamento: ");
        Serial.print((millis() - lastRoutingActivity) / 1000);
        Serial.println("s atrás");
    }

    Serial.println("==================================");
}

void showLocalTopology()
{
    Serial.println("");
    Serial.println("🌐 ===== TOPOLOGIA LOCAL =====");
    Serial.print("📍 Este nó: ");
    Serial.print(nodeName);
    Serial.println();

    // Mostrar conexão com o hub
    if (hubFound)
    {
        Serial.println("🎯 Conexão com Hub:");
        Serial.print("   ✅ ");
        Serial.print(nodeName);
        Serial.print(" → ");
        Serial.print(HUB_NAME);
        Serial.print(" (ID: ");
        Serial.print(hubNodeId);
        Serial.println(")");

        // Mostrar qualidade da conexão se disponível
        if (m2mMesh.nodeIsReachableNode(hubNodeId))
        {
            Serial.print("   📶 Qualidade: ");
            uint32_t tq = m2mMesh.globalTransmissionQuality(hubNodeId);
            Serial.print(tq, HEX);
            Serial.println(" (hex)");
        }
    }
    else
    {
        Serial.println("❌ Sem conexão com o hub");
    }

    // Mostrar nós que podem usar este como ponte
    Serial.println();
    Serial.println("🌉 Possíveis conexões através deste nó:");

    bool foundRoutedNodes = false;
    for (uint8_t nodeId = 0; nodeId < m2mMesh.numberOfNodes(); nodeId++)
    {
        if (nodeId != hubNodeId && m2mMesh.nodeIsReachableNode(nodeId))
        {
            String nodeName_check = "";
            if (m2mMesh.nodeNameIsSet(nodeId))
            {
                nodeName_check = String(m2mMesh.getNodeName(nodeId));
            }

            Serial.print("   🔗 Nó ");
            Serial.print(nodeId);
            if (nodeName_check.length() > 0)
            {
                Serial.print(" (");
                Serial.print(nodeName_check);
                Serial.print(")");
            }

            uint8_t router = m2mMesh.selectedRouter(nodeId);
            if (router != 255 && router != nodeId)
            {
                Serial.print(" → usa roteador ");
                Serial.println(router);
            }
            else
            {
                Serial.println(" → conexão direta");
            }
            foundRoutedNodes = true;
        }
    }

    if (!foundRoutedNodes)
    {
        Serial.println("   Nenhum outro nó visível");
    }

    Serial.println("=============================");
}

void showNeighbors()
{
    Serial.println("");
    Serial.println("👥 ===== NÓS VIZINHOS =====");

    uint8_t totalNodes = m2mMesh.numberOfNodes();
    uint8_t reachableNodes = m2mMesh.numberOfReachableNodes();

    // Proteção contra valores absurdos
    if (totalNodes > 50)
        totalNodes = 50;
    if (reachableNodes > 50)
        reachableNodes = 50;

    Serial.print("Total de nós na mesh: ");
    Serial.println(totalNodes);
    Serial.print("Nós alcançáveis: ");
    Serial.println(reachableNodes);
    Serial.println();

    if (totalNodes == 0)
    {
        Serial.println("Nenhum nó encontrado na mesh");
        Serial.println("===========================");
        return;
    }

    for (uint8_t nodeId = 0; nodeId < totalNodes; nodeId++)
    {
        Serial.print("📡 Nó ");
        Serial.print(nodeId);

        if (m2mMesh.nodeNameIsSet(nodeId))
        {
            String nodeName_check = String(m2mMesh.getNodeName(nodeId));
            if (nodeName_check.length() > 0 && nodeName_check.length() < 50) // Proteção
            {
                Serial.print(" (");
                Serial.print(nodeName_check);
                Serial.print(")");
            }
        }

        if (nodeId == hubNodeId)
        {
            Serial.print(" 🎯 HUB");
        }

        if (m2mMesh.nodeIsReachableNode(nodeId))
        {
            Serial.print(" ✅ ALCANÇÁVEL");

            uint32_t tq = m2mMesh.globalTransmissionQuality(nodeId);
            Serial.print(" - TQ: ");
            Serial.print(tq, HEX);

            uint8_t router = m2mMesh.selectedRouter(nodeId);
            if (router != 255 && router != nodeId && router < 50) // Proteção
            {
                Serial.print(" via ");
                Serial.print(router);
            }
        }
        else
        {
            Serial.print(" ❌ INACESSÍVEL");
        }

        Serial.println();
    }

    Serial.println("===========================");
}

// Função de callback para eventos da mesh
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
        // Aproveitar para procurar o hub quando a mesh estabilizar
        lastHubSearchTime = 0;
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

    case meshEvent::complete:
        Serial.println("🎯 Evento: Operação completa");
        break;

    default:
        Serial.print("❓ Evento desconhecido: ");
        Serial.println((int)event);
        break;
    }
}