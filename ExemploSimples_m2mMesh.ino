/**
 * Exemplo Simples - m2mMesh
 *
 * Este é o exemplo mais básico possível para testar a biblioteca m2mMesh.
 * Ideal para iniciantes e primeiros testes.
 *
 * O que este código faz:
 * - Conecta automaticamente à rede mesh
 * - Acende o LED quando conectado
 * - Envia uma mensagem "Olá!" a cada 15 segundos
 * - Exibe mensagens recebidas no Serial Monitor
 *
 * COMO USAR:
 * 1. Instale a biblioteca m2mMesh (veja GUIA_COMPILACAO.md)
 * 2. Carregue este código em pelo menos 2 dispositivos ESP8266/ESP32
 * 3. Abra o Monitor Serial (115200 baud)
 * 4. Aguarde os dispositivos se conectarem
 * 5. Observe as mensagens sendo trocadas!
 */

#include <m2mMesh.h>

// Variáveis simples
bool conectado = false;
unsigned long ultimaMensagem = 0;
int contador = 0;

void setup()
{
// Configurar LED integrado (se disponível)
#ifdef LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // LED apagado inicialmente
#endif

    // Inicializar comunicação serial
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== m2mMesh - Exemplo Simples ===");
    Serial.println("Inicializando...");

    // Dar um nome único para este dispositivo
    String meuNome = "ESP_" + String(random(100, 999));
    m2mMesh.setNodeName(meuNome);
    Serial.println("Meu nome: " + meuNome);

    // Inicializar a rede mesh
    if (m2mMesh.begin())
    {
        Serial.println("✓ Mesh iniciada com sucesso!");
        Serial.println("Procurando outros dispositivos...");
    }
    else
    {
        Serial.println("✗ ERRO: Não conseguiu iniciar a mesh");
        Serial.println("Verifique se você está usando ESP8266 ou ESP32");
    }
}

void loop()
{
    // MUITO IMPORTANTE: Esta função deve ser chamada sempre
    m2mMesh.housekeeping();

    // Verificar se conectou na rede
    verificarConexao();

    // Enviar mensagem de vez em quando
    enviarMensagem();

    // Verificar se chegou alguma mensagem
    verificarMensagensRecebidas();

    // Pequena pausa
    delay(100);
}

void verificarConexao()
{
    bool agora = m2mMesh.joined(); // Está conectado agora?

    if (!conectado && agora)
    {
        // Acabou de conectar!
        conectado = true;
#ifdef LED_BUILTIN
        digitalWrite(LED_BUILTIN, HIGH); // Acender LED
#endif
        Serial.println("🎉 CONECTADO à rede mesh!");
        Serial.println("Agora posso enviar e receber mensagens");
    }
    else if (conectado && !agora)
    {
        // Desconectou
        conectado = false;
#ifdef LED_BUILTIN
        digitalWrite(LED_BUILTIN, LOW); // Apagar LED
#endif
        Serial.println("😞 DESCONECTADO da rede mesh");
    }
}

void enviarMensagem()
{
    // Só enviar se estiver conectado
    if (!conectado)
        return;

    // Enviar a cada 15 segundos
    unsigned long agora = millis();
    if (agora - ultimaMensagem >= 15000)
    {
        ultimaMensagem = agora;
        contador++;

        // Criar a mensagem
        String mensagem = "Olá! Sou " + String(m2mMesh.getNodeName()) +
                          " - Mensagem #" + String(contador);

        // Limpar buffer anterior e adicionar nova mensagem
        m2mMesh.clearMessage();
        m2mMesh.add(mensagem);

        // Tentar enviar
        if (m2mMesh.send())
        {
            Serial.println("📤 Enviei: " + mensagem);
        }
        else
        {
            Serial.println("❌ Falhou ao enviar mensagem");
        }
    }
}

void verificarMensagensRecebidas()
{
    // Verificar se há mensagens esperando
    while (m2mMesh.messageWaiting())
    {

        // Descobrir quem enviou
        uint8_t remetente = m2mMesh.sourceId();
        Serial.print("📥 Mensagem do nó " + String(remetente) + ": ");

        // Ler o conteúdo (assumindo que é texto)
        if (m2mMesh.nextDataType() == m2mMesh.USR_DATA_STRING)
        {
            String texto = m2mMesh.retrieveString();
            Serial.println(texto);
        }
        else
        {
            Serial.println("(tipo de dado não reconhecido)");
        }

        // Marcar como lida
        m2mMesh.markMessageRead();
    }
}

/*
 * DICAS PARA TESTAR:
 *
 * 1. Carregue este código em 2 ou mais dispositivos ESP
 * 2. Abra o Monitor Serial de cada um
 * 3. Aguarde aparecer "CONECTADO à rede mesh!" em ambos
 * 4. Você verá as mensagens sendo enviadas entre eles
 * 5. O LED integrado acende quando conectado
 *
 * TROUBLESHOOTING:
 *
 * - Se não conectar: verifique se tem pelo menos 2 dispositivos ligados
 * - Se der erro de compilação: veja o arquivo GUIA_COMPILACAO.md
 * - Se não aparecer mensagens: aguarde até 30 segundos para primeira conexão
 *
 * PRÓXIMOS PASSOS:
 *
 * - Teste com 3+ dispositivos para ver o roteamento automático
 * - Experimente afastar os dispositivos para testar multi-hop
 * - Veja outros exemplos na pasta examples/ da biblioteca
 */
