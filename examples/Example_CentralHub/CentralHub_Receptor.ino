/**
 * ESP Receptor Central (Hub) - m2mMesh
 *
 * Este exemplo demonstra como configurar um ESP para funcionar como
 * receptor central de todas as mensagens da mesh.
 *
 * Funcionalidades:
 * - Recebe mensagens de todos os nós da mesh
 * - Mantém estatísticas de mensagens recebidas
 * - Interface serial para monitoramento
 * - LED de status integrado
 * - Resposta automática para nós que enviam mensagens
 *
 * IMPORTANTE: Este ESP deve ser configurado com um nome específico
 * para que os outros nós saibam para onde enviar as mensagens.
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

// Configurações do Hub Central
const String HUB_NAME = "CentralHub"; // Nome fixo do hub para identificação
const uint8_t MAX_NODES = 20;         // Máximo de nós na mesh
const uint8_t MESH_CHANNEL = 1;       // Canal da mesh

// Variáveis de controle
bool joinedMesh = false;
uint8_t numberOfNodes = 0;
uint8_t numberOfReachableNodes = 0;
unsigned long lastStatusTime = 0;
unsigned long statusInterval = 30000; // Status a cada 30 segundos

// Estatísticas
unsigned long totalMessagesReceived = 0;
unsigned long lastMessageTime = 0;
uint8_t lastSenderId = 255;

// Estrutura para armazenar estatísticas por nó
struct NodeStats
{
    String nodeName;
    uint8_t nodeId;
    unsigned long messagesReceived;
    unsigned long lastMessageTime;
    bool active;
};

NodeStats nodeStats[20]; // Array para estatísticas dos nós
uint8_t totalNodesTracked = 0;

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

    Serial.println("=== m2mMesh - RECEPTOR CENTRAL (HUB) ===");
    Serial.println("Inicializando como hub central da mesh...");

    // Definir nome fixo do hub
    m2mMesh.setNodeName(HUB_NAME);
    Serial.println("Nome do hub: " + HUB_NAME);

    // Inicializar a mesh
    if (m2mMesh.begin(MAX_NODES, MESH_CHANNEL))
    {
        Serial.print("Hub iniciado com sucesso no canal: ");
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

        Serial.println("Hub pronto para receber mensagens!");
    }
    else
    {
        Serial.println("ERRO: Falha ao inicializar o hub!");
        while (true)
        {
            delay(1000);
        }
    }

    // Inicializar array de estatísticas
    for (int i = 0; i < 20; i++)
    {
        nodeStats[i].nodeId = 255;
        nodeStats[i].messagesReceived = 0;
        nodeStats[i].lastMessageTime = 0;
        nodeStats[i].active = false;
    }

    Serial.println("Aguardando conexão com nós transmissores...");
    printHelp();
}

void loop()
{
    // IMPORTANTE: Deve ser chamado pelo menos uma vez por segundo
    m2mMesh.housekeeping();

    // Verificar status da mesh
    checkMeshStatus();

    // Verificar mudanças no número de nós
    checkNodeCount();

    // Verificar mensagens recebidas
    checkIncomingMessages();

    // Enviar status periodicamente
    sendPeriodicStatus();

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
        Serial.println("✓ HUB CONECTADO à mesh!");
        Serial.println("Aguardando mensagens dos nós transmissores...");
    }
    // Verificar se saiu da mesh
    else if (joinedMesh == true && m2mMesh.joined() == false)
    {
        joinedMesh = false;
#if defined(LED_BUILTIN)
        digitalWrite(LED_BUILTIN, LED_OFF);
#endif
        Serial.println("✗ HUB DESCONECTADO da mesh");
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

        Serial.print("🌐 Nós na rede: ");
        Serial.print(numberOfNodes);
        Serial.print(" | Nós alcançáveis: ");
        Serial.print(numberOfReachableNodes);

        if (m2mMesh.stable())
        {
            Serial.println(" | Status: ESTÁVEL");

            // Mostrar mini topologia quando a rede estabiliza
            Serial.println("");
            Serial.println("📊 Mini Topologia (digite 'topology' para versão completa):");
            Serial.print("   🏢 HUB ← ");

            if (numberOfNodes == 0)
            {
                Serial.println("❌ Nenhum nó conectado");
            }
            else
            {
                Serial.print("🔗 ");
                Serial.print(numberOfReachableNodes);
                Serial.print(" nós ativos");
                if (numberOfReachableNodes < numberOfNodes)
                {
                    Serial.print(" (");
                    Serial.print(numberOfNodes - numberOfReachableNodes);
                    Serial.print(" offline)");
                }
                Serial.println();
            }
        }
        else
        {
            Serial.println(" | Status: MUDANDO");
        }
    }
}

void checkIncomingMessages()
{
    // Verificar se há mensagens aguardando
    while (m2mMesh.messageWaiting())
    {
        totalMessagesReceived++;
        lastMessageTime = millis();

        Serial.println("");
        Serial.println("📥 ===== MENSAGEM RECEBIDA =====");

        // Obter informações do remetente
        uint8_t senderId = m2mMesh.sourceId();
        lastSenderId = senderId;
        uint8_t senderMac[6];
        m2mMesh.sourceMacAddress(senderMac);

        Serial.print("📍 Remetente: Nó ID ");
        Serial.print(senderId);

        // Verificar se conhecemos o nome do nó
        String senderName = "";
        if (m2mMesh.nodeNameIsSet(senderId))
        {
            senderName = String(m2mMesh.getNodeName(senderId));
            Serial.print(" (");
            Serial.print(senderName);
            Serial.print(")");
        }

        Serial.print(" - MAC: ");
        for (int i = 0; i < 6; i++)
        {
            if (senderMac[i] < 16)
                Serial.print("0");
            Serial.print(senderMac[i], HEX);
            if (i < 5)
                Serial.print(":");
        }
        Serial.println();

        // Atualizar estatísticas do nó
        updateNodeStats(senderId, senderName);

        // Processar dados da mensagem
        Serial.println("📦 Conteúdo da mensagem:");
        int dataCount = 0;
        while (m2mMesh.dataAvailable() > 0)
        {
            dataCount++;
            uint8_t dataType = m2mMesh.nextDataType();

            Serial.print("   [");
            Serial.print(dataCount);
            Serial.print("] ");

            switch (dataType)
            {
            case m2mMesh.USR_DATA_STRING:
            {
                String receivedText = m2mMesh.retrieveString();
                Serial.print("Texto: ");
                Serial.println(receivedText);
            }
            break;

            case m2mMesh.USR_DATA_UINT32_T:
            {
                uint32_t receivedNumber = m2mMesh.retrieveUint32_t();
                Serial.print("Número: ");
                Serial.println(receivedNumber);
            }
            break;

            case m2mMesh.USR_DATA_INT32_T:
            {
                int32_t receivedInt = m2mMesh.retrieveInt32_t();
                Serial.print("Inteiro: ");
                Serial.println(receivedInt);
            }
            break;

            case m2mMesh.USR_DATA_FLOAT:
            {
                float receivedFloat = m2mMesh.retrieveFloat();
                Serial.print("Float: ");
                Serial.println(receivedFloat, 2);
            }
            break;

            case m2mMesh.USR_DATA_BOOL:
            {
                bool receivedBool = m2mMesh.retrieveBool();
                Serial.print("Boolean: ");
                Serial.println(receivedBool ? "true" : "false");
            }
            break;

            default:
                // Tipo de dado não reconhecido, pular
                m2mMesh.skipRetrieve();
                Serial.print("Tipo de dado não reconhecido (");
                Serial.print(dataType);
                Serial.println(")");
                break;
            }
        }

        // Marcar mensagem como lida
        m2mMesh.markMessageRead();

        Serial.print("⏰ Timestamp: ");
        Serial.print(millis());
        Serial.println("ms");
        Serial.print("📊 Total de mensagens recebidas: ");
        Serial.println(totalMessagesReceived);
        Serial.println("✅ Mensagem processada com sucesso");
        Serial.println("===============================");

        // Enviar confirmação de recebimento para o remetente
        sendAcknowledgment(senderId, senderName);
    }
}

void updateNodeStats(uint8_t nodeId, String nodeName)
{
    // Procurar se já temos estatísticas para este nó
    int nodeIndex = -1;
    for (int i = 0; i < totalNodesTracked; i++)
    {
        if (nodeStats[i].nodeId == nodeId)
        {
            nodeIndex = i;
            break;
        }
    }

    // Se não encontrou, criar nova entrada
    if (nodeIndex == -1 && totalNodesTracked < 20)
    {
        nodeIndex = totalNodesTracked;
        totalNodesTracked++;
        nodeStats[nodeIndex].nodeId = nodeId;
        nodeStats[nodeIndex].messagesReceived = 0;
    }

    // Atualizar estatísticas
    if (nodeIndex != -1)
    {
        nodeStats[nodeIndex].nodeName = nodeName;
        nodeStats[nodeIndex].messagesReceived++;
        nodeStats[nodeIndex].lastMessageTime = millis();
        nodeStats[nodeIndex].active = true;
    }
}

void sendAcknowledgment(uint8_t targetNodeId, String targetNodeName)
{
    // Enviar confirmação de recebimento
    m2mMesh.clearMessage();

    String ackMessage = "ACK: Mensagem recebida pelo hub ";
    ackMessage += HUB_NAME;
    m2mMesh.add(ackMessage);

    String timestampMsg = "Timestamp: ";
    timestampMsg += String(millis());
    m2mMesh.add(timestampMsg);
    m2mMesh.add(totalMessagesReceived); // Número total de mensagens recebidas

    // Definir destino (enviar de volta para o remetente)
    if (m2mMesh.destination(targetNodeId))
    {
        if (m2mMesh.send())
        {
            Serial.print("📤 Confirmação enviada para nó ");
            Serial.print(targetNodeId);
            if (targetNodeName.length() > 0)
            {
                Serial.print(" (");
                Serial.print(targetNodeName);
                Serial.print(")");
            }
            Serial.println();
        }
        else
        {
            Serial.print("❌ Falha ao enviar confirmação para nó ");
            Serial.println(targetNodeId);
        }
    }
}

void sendPeriodicStatus()
{
    unsigned long currentTime = millis();
    if (currentTime - lastStatusTime >= statusInterval)
    {
        lastStatusTime = currentTime;

        Serial.println("");
        Serial.println("📊 ===== STATUS DO HUB =====");
        Serial.print("⏰ Uptime: ");
        Serial.print(currentTime / 1000);
        Serial.println(" segundos");
        Serial.print("📨 Total de mensagens: ");
        Serial.println(totalMessagesReceived);
        Serial.print("🌐 Nós conectados: ");
        Serial.println(numberOfNodes);
        Serial.print("📡 Nós alcançáveis: ");
        Serial.println(numberOfReachableNodes);
        Serial.print("🔄 Status da mesh: ");
        Serial.println(m2mMesh.stable() ? "ESTÁVEL" : "MUDANDO");

        if (totalMessagesReceived > 0)
        {
            Serial.print("📅 Última mensagem: ");
            Serial.print((currentTime - lastMessageTime) / 1000);
            Serial.println("s atrás");
            Serial.print("📍 Último remetente: Nó ");
            Serial.println(lastSenderId);
        }

        // Mostrar estatísticas por nó
        if (totalNodesTracked > 0)
        {
            Serial.println("📈 Estatísticas por nó:");
            for (int i = 0; i < totalNodesTracked; i++)
            {
                if (nodeStats[i].active)
                {
                    Serial.print("   Nó ");
                    Serial.print(nodeStats[i].nodeId);
                    if (nodeStats[i].nodeName.length() > 0)
                    {
                        Serial.print(" (");
                        Serial.print(nodeStats[i].nodeName);
                        Serial.print(")");
                    }
                    Serial.print(": ");
                    Serial.print(nodeStats[i].messagesReceived);
                    Serial.print(" msgs - Último: ");
                    Serial.print((currentTime - nodeStats[i].lastMessageTime) / 1000);
                    Serial.println("s atrás");
                }
            }
        }

        Serial.println("============================");
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
            printDetailedStatus();
        }
        else if (command == "stats")
        {
            printNodeStatistics();
        }
        else if (command == "reset")
        {
            resetStatistics();
        }
        else if (command.startsWith("send "))
        {
            String msg = command.substring(5);
            sendBroadcastMessage(msg);
        }
        else if (command.startsWith("msg "))
        {
            // Formato: msg <nodeId> <mensagem>
            int spaceIndex = command.indexOf(' ', 4);
            if (spaceIndex > 0)
            {
                uint8_t nodeId = command.substring(4, spaceIndex).toInt();
                String msg = command.substring(spaceIndex + 1);
                sendDirectMessage(nodeId, msg);
            }
            else
            {
                Serial.println("❌ Formato incorreto. Use: msg <nodeId> <mensagem>");
            }
        }
        else if (command == "nodes")
        {
            listConnectedNodes();
        }
        else if (command == "topology" || command == "topo")
        {
            showNetworkTopology();
        }
        else if (command == "routes")
        {
            showRoutingTable();
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
    Serial.println("help ou ?        - Mostrar esta ajuda");
    Serial.println("status ou info   - Status detalhado do hub");
    Serial.println("stats            - Estatísticas por nó");
    Serial.println("nodes            - Listar nós conectados");
    Serial.println("topology ou topo - Desenhar topologia da rede");
    Serial.println("routes           - Mostrar tabela de roteamento");
    Serial.println("reset            - Resetar estatísticas");
    Serial.println("send <msg>       - Enviar mensagem broadcast");
    Serial.println("msg <id> <msg>   - Enviar mensagem para nó específico");
    Serial.println("===================================");
}

void printDetailedStatus()
{
    Serial.println("");
    Serial.println("📊 ===== STATUS DETALHADO =====");
    Serial.print("🏷️  Nome do hub: ");
    Serial.println(HUB_NAME);
    Serial.print("🔗 Conectado à mesh: ");
    Serial.println(m2mMesh.joined() ? "Sim" : "Não");
    Serial.print("⚖️  Mesh estável: ");
    Serial.println(m2mMesh.stable() ? "Sim" : "Não");
    Serial.print("🌐 Total de nós: ");
    Serial.println(m2mMesh.numberOfNodes());
    Serial.print("📡 Nós alcançáveis: ");
    Serial.println(m2mMesh.numberOfReachableNodes());
    Serial.print("📨 Mensagens recebidas: ");
    Serial.println(totalMessagesReceived);
    Serial.print("⏰ Uptime: ");
    Serial.print(millis() / 1000);
    Serial.println(" segundos");

    if (m2mMesh.synced())
    {
        Serial.print("🕐 Tempo da mesh: ");
        Serial.print(m2mMesh.syncedMillis());
        Serial.println("ms");
        Serial.print("🎯 Servidor de sync: Nó ");
        Serial.println(m2mMesh.syncServer());
    }

    Serial.println("===============================");
}

void printNodeStatistics()
{
    Serial.println("");
    Serial.println("📈 ===== ESTATÍSTICAS POR NÓ =====");

    if (totalNodesTracked == 0)
    {
        Serial.println("Nenhum nó rastreado ainda.");
    }
    else
    {
        for (int i = 0; i < totalNodesTracked; i++)
        {
            if (nodeStats[i].active)
            {
                Serial.print("📍 Nó ");
                Serial.print(nodeStats[i].nodeId);
                if (nodeStats[i].nodeName.length() > 0)
                {
                    Serial.print(" (");
                    Serial.print(nodeStats[i].nodeName);
                    Serial.print(")");
                }
                Serial.println();
                Serial.print("   📨 Mensagens: ");
                Serial.println(nodeStats[i].messagesReceived);
                Serial.print("   ⏰ Última msg: ");
                Serial.print((millis() - nodeStats[i].lastMessageTime) / 1000);
                Serial.println("s atrás");
                Serial.print("   🔄 Ativo: ");
                Serial.println(nodeStats[i].active ? "Sim" : "Não");
                Serial.println();
            }
        }
    }

    Serial.println("==================================");
}

void resetStatistics()
{
    totalMessagesReceived = 0;
    lastMessageTime = 0;
    lastSenderId = 255;
    totalNodesTracked = 0;

    for (int i = 0; i < 20; i++)
    {
        nodeStats[i].nodeId = 255;
        nodeStats[i].messagesReceived = 0;
        nodeStats[i].lastMessageTime = 0;
        nodeStats[i].active = false;
        nodeStats[i].nodeName = "";
    }

    Serial.println("✅ Estatísticas resetadas com sucesso!");
}

void sendBroadcastMessage(String message)
{
    m2mMesh.clearMessage();
    String hubMessage = "Mensagem do hub: ";
    hubMessage += message;
    m2mMesh.add(hubMessage);
    m2mMesh.add(millis());

    if (m2mMesh.send())
    {
        Serial.print("📤 Mensagem broadcast enviada: ");
        Serial.println(message);
    }
    else
    {
        Serial.println("❌ Falha ao enviar mensagem broadcast");
    }
}

void sendDirectMessage(uint8_t nodeId, String message)
{
    m2mMesh.clearMessage();
    String directMessage = "Mensagem direta do hub: ";
    directMessage += message;
    m2mMesh.add(directMessage);
    m2mMesh.add(millis());

    if (m2mMesh.destination(nodeId))
    {
        if (m2mMesh.send())
        {
            Serial.print("📤 Mensagem enviada para nó ");
            Serial.print(nodeId);
            Serial.print(": ");
            Serial.println(message);
        }
        else
        {
            Serial.print("❌ Falha ao enviar mensagem para nó ");
            Serial.println(nodeId);
        }
    }
    else
    {
        Serial.print("❌ Nó ");
        Serial.print(nodeId);
        Serial.println(" não encontrado ou não alcançável");
    }
}

void listConnectedNodes()
{
    Serial.println("");
    Serial.println("🌐 ===== NÓS CONECTADOS =====");
    Serial.print("Total de nós: ");
    Serial.println(m2mMesh.numberOfNodes());
    Serial.print("Nós alcançáveis: ");
    Serial.println(m2mMesh.numberOfReachableNodes());

    for (uint8_t i = 0; i < m2mMesh.numberOfNodes(); i++)
    {
        if (m2mMesh.nodeIsReachableNode(i))
        {
            Serial.print("📍 Nó ");
            Serial.print(i);

            if (m2mMesh.nodeNameIsSet(i))
            {
                Serial.print(" (");
                Serial.print(m2mMesh.getNodeName(i));
                Serial.print(")");
            }

            uint8_t *mac = m2mMesh.getMeshAddress(i);
            Serial.print(" - MAC: ");
            for (int j = 0; j < 6; j++)
            {
                if (mac[j] < 16)
                    Serial.print("0");
                Serial.print(mac[j], HEX);
                if (j < 5)
                    Serial.print(":");
            }

            Serial.println(" - Status: ALCANÇÁVEL");
        }
    }

    Serial.println("=============================");
}

void showNetworkTopology()
{
    Serial.println("");
    Serial.println("🌐 ===== TOPOLOGIA DA REDE =====");

    uint8_t totalNodes = m2mMesh.numberOfNodes();
    uint8_t reachableNodes = m2mMesh.numberOfReachableNodes();

    Serial.print("Total de nós: ");
    Serial.print(totalNodes + 1); // +1 para incluir o hub
    Serial.print(" (");
    Serial.print(totalNodes);
    Serial.println(" transmissores + 1 hub)");
    Serial.print("Nós alcançáveis: ");
    Serial.println(reachableNodes);
    Serial.println();

    // Desenhar o hub central
    Serial.println("                    ┌─────────────────┐");
    Serial.println("                    │   🏢 HUB CENTRAL │");
    Serial.print("                    │   ");
    Serial.print(HUB_NAME);
    for (int i = HUB_NAME.length(); i < 12; i++)
        Serial.print(" ");
    Serial.println("│");
    Serial.println("                    │   (ID: LOCAL)   │");
    Serial.println("                    └─────────┬───────┘");
    Serial.println("                              │");

    if (totalNodes == 0)
    {
        Serial.println("                         ❌ Nenhum nó conectado");
        Serial.println("===============================");
        return;
    }

    // Analisar conexões reais usando as funções da biblioteca
    Serial.println("                              │");
    Serial.println("              ┌───────────────┼───────────────┐");

    // Listar cada nó e sua conexão REAL
    for (uint8_t nodeId = 0; nodeId < totalNodes; nodeId++)
    {
        String nodeInfo = "📡 Nó ";
        nodeInfo += String(nodeId);

        if (m2mMesh.nodeNameIsSet(nodeId))
        {
            nodeInfo += " (";
            nodeInfo += String(m2mMesh.getNodeName(nodeId));
            nodeInfo += ")";
        }

        // Verificar se o nó é alcançável
        bool isReachable = m2mMesh.nodeIsReachableNode(nodeId);

        if (isReachable)
        {
            // Usar selectedRouter para determinar se é conexão direta ou roteada
            uint8_t router = m2mMesh.selectedRouter(nodeId);

            // Se router é 255 (MESH_ORIGINATOR_NOT_FOUND) ou igual ao próprio nodeId,
            // é conexão direta. Caso contrário, é roteada.
            bool isDirectConnection = (router == 255 || router == nodeId);

            // Obter qualidade de transmissão para análise adicional
            uint32_t localTQ = m2mMesh.localTransmissionQuality(nodeId);
            uint32_t globalTQ = m2mMesh.globalTransmissionQuality(nodeId);

            Serial.println("              │");

            if (isDirectConnection)
            {
                Serial.println("              ├─── 🟢 DIRETO ───┐");
                Serial.print("              │                 ");
                Serial.println(nodeInfo);
                Serial.print("              │                 Status: VIZINHO DIRETO");
                Serial.println();
                Serial.print("              │                 TQ Local: ");
                Serial.print(localTQ, HEX);
                Serial.print(" | Global: ");
                Serial.println(globalTQ, HEX);
            }
            else
            {
                Serial.println("              ├─── 🟡 ROTEADO ──┐");
                Serial.print("              │                 ");
                Serial.println(nodeInfo);
                Serial.print("              │                 Status: VIA NÓ ");
                Serial.println(router);
                Serial.print("              │                 Rota: HUB → Nó");
                Serial.print(router);
                Serial.print(" → Nó");
                Serial.println(nodeId);
                Serial.print("              │                 TQ Local: ");
                Serial.print(localTQ, HEX);
                Serial.print(" | Global: ");
                Serial.println(globalTQ, HEX);
            }
        }
        else
        {
            Serial.println("              │");
            Serial.println("              ├─── 🔴 OFFLINE ──┐");
            Serial.print("              │                 ");
            Serial.println(nodeInfo);
            Serial.print("              │                 Status: INACESSÍVEL");
            Serial.println();
        }

        if (nodeId < totalNodes - 1)
        {
            Serial.println("              │");
        }
    }

    Serial.println();
    Serial.println("Legenda:");
    Serial.println("🟢 DIRETO     - Nó conectado diretamente ao hub (1 salto)");
    Serial.println("🟡 ROTEADO    - Nó conectado através de outros nós (multi-hop)");
    Serial.println("🔴 OFFLINE    - Nó não alcançável no momento");
    Serial.println("TQ            - Transmission Quality (hex): maior = melhor");
    Serial.println();

    // Análise avançada da topologia
    int directConnections = 0;
    int routedConnections = 0;

    for (uint8_t nodeId = 0; nodeId < totalNodes; nodeId++)
    {
        if (m2mMesh.nodeIsReachableNode(nodeId))
        {
            uint8_t router = m2mMesh.selectedRouter(nodeId);
            if (router == 255 || router == nodeId)
            {
                directConnections++;
            }
            else
            {
                routedConnections++;
            }
        }
    }

    // Mostrar estatísticas de conectividade
    float connectivityRate = (totalNodes > 0) ? (float)reachableNodes / totalNodes * 100 : 0;
    Serial.print("Taxa de conectividade: ");
    Serial.print(connectivityRate, 1);
    Serial.println("%");

    Serial.print("Conexões diretas: ");
    Serial.print(directConnections);
    Serial.print(" | Conexões roteadas: ");
    Serial.println(routedConnections);

    if (m2mMesh.stable())
    {
        Serial.println("Status da rede: ✅ ESTÁVEL");
    }
    else
    {
        Serial.println("Status da rede: 🔄 MUDANDO (aguarde estabilização)");
    }

    Serial.println("===============================");
}

void showRoutingTable()
{
    Serial.println("");
    Serial.println("📋 ===== TABELA DE ROTEAMENTO =====");

    uint8_t totalNodes = m2mMesh.numberOfNodes();

    if (totalNodes == 0)
    {
        Serial.println("Nenhum nó na rede para mostrar rotas.");
        Serial.println("===================================");
        return;
    }

    Serial.println("┌─────┬─────────────────┬─────────────┬─────────────────┬─────────┐");
    Serial.println("│ ID  │      NOME       │   STATUS    │      ROTA       │   TQ    │");
    Serial.println("├─────┼─────────────────┼─────────────┼─────────────────┼─────────┤");

    for (uint8_t nodeId = 0; nodeId < totalNodes; nodeId++)
    {
        Serial.print("│ ");
        if (nodeId < 10)
            Serial.print(" ");
        Serial.print(nodeId);
        Serial.print(" │ ");

        // Nome do nó
        String nodeName = "";
        if (m2mMesh.nodeNameIsSet(nodeId))
        {
            nodeName = String(m2mMesh.getNodeName(nodeId));
        }
        else
        {
            nodeName = "Sem nome";
        }

        // Limitar nome a 15 caracteres
        if (nodeName.length() > 15)
        {
            nodeName = nodeName.substring(0, 12) + "...";
        }

        Serial.print(nodeName);
        for (int i = nodeName.length(); i < 15; i++)
            Serial.print(" ");
        Serial.print(" │ ");

        // Status
        if (m2mMesh.nodeIsReachableNode(nodeId))
        {
            Serial.print("🟢 ALCANÇA  ");
        }
        else
        {
            Serial.print("🔴 OFFLINE  ");
        }
        Serial.print(" │ ");

        // Informação de rota REAL
        if (m2mMesh.nodeIsReachableNode(nodeId))
        {
            uint8_t router = m2mMesh.selectedRouter(nodeId);

            if (router == 255 || router == nodeId)
            {
                // Conexão direta
                Serial.print("HUB → Nó");
                if (nodeId < 10)
                    Serial.print(" ");
                Serial.print(nodeId);
                Serial.print("     ");
            }
            else
            {
                // Conexão roteada
                Serial.print("HUB→");
                Serial.print(router);
                Serial.print("→");
                Serial.print(nodeId);
                for (int i = 0; i < 8; i++)
                    Serial.print(" "); // Padding
            }
        }
        else
        {
            Serial.print("N/A             ");
        }
        Serial.print(" │ ");

        // Transmission Quality
        if (m2mMesh.nodeIsReachableNode(nodeId))
        {
            uint32_t globalTQ = m2mMesh.globalTransmissionQuality(nodeId);
            Serial.print(globalTQ, HEX);
            // Padding para manter alinhamento
            String tqStr = String(globalTQ, HEX);
            for (int i = tqStr.length(); i < 7; i++)
                Serial.print(" ");
        }
        else
        {
            Serial.print("N/A    ");
        }
        Serial.println(" │");
    }

    Serial.println("└─────┴─────────────────┴─────────────┴─────────────────┴─────────┘");
    Serial.println();

    // Informações adicionais sobre o roteamento
    Serial.println("ℹ️  Análise de Roteamento:");

    int directConnections = 0;
    int routedConnections = 0;
    int offlineNodes = 0;

    for (uint8_t nodeId = 0; nodeId < totalNodes; nodeId++)
    {
        if (m2mMesh.nodeIsReachableNode(nodeId))
        {
            uint8_t router = m2mMesh.selectedRouter(nodeId);
            if (router == 255 || router == nodeId)
            {
                directConnections++;
            }
            else
            {
                routedConnections++;
            }
        }
        else
        {
            offlineNodes++;
        }
    }

    Serial.print("   • Conexões diretas: ");
    Serial.print(directConnections);
    Serial.print(" | Roteadas: ");
    Serial.print(routedConnections);
    Serial.print(" | Offline: ");
    Serial.println(offlineNodes);

    Serial.print("   • Taxa de conectividade: ");
    if (totalNodes > 0)
    {
        float connectivityRate = (float)(directConnections + routedConnections) / totalNodes * 100;
        Serial.print(connectivityRate, 1);
        Serial.println("%");
    }
    else
    {
        Serial.println("0%");
    }

    Serial.println("   • Protocolo: B.A.T.M.A.N. Advanced sobre ESP-NOW");

    if (m2mMesh.stable())
    {
        Serial.println("   • Status: Tabela estável ✅");
    }
    else
    {
        Serial.println("   • Status: Atualizando rotas 🔄");
    }

    Serial.println();
    Serial.println("💡 TQ = Transmission Quality (hexadecimal)");
    Serial.println("💡 Maior TQ = Melhor qualidade de conexão");
    Serial.println("💡 Use 'topology' para diagrama visual da rede");
    Serial.println("===================================");
}

// Função de callback para eventos da mesh
void onMeshEvent(meshEvent event)
{
    switch (event)
    {
    case meshEvent::joined:
        Serial.println("🔗 Evento: Hub entrou na mesh");
        break;

    case meshEvent::left:
        Serial.println("🔌 Evento: Hub saiu da mesh");
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

    case meshEvent::complete:
        Serial.println("🎯 Evento: Operação completa");
        break;

    default:
        Serial.print("❓ Evento desconhecido: ");
        Serial.println((int)event);
        break;
    }
}