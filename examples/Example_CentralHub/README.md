# Sistema Hub Centralizado - m2mMesh

Este exemplo demonstra como criar um sistema onde um ESP funciona como receptor central (hub) e outros ESPs enviam mensagens para ele através da rede mesh, com roteamento automático quando necessário.

## 📋 Descrição

O sistema consiste em dois tipos de dispositivos:

1. **Hub Central (Receptor)**: Um ESP que recebe todas as mensagens e funciona como ponto central de coleta
2. **Transmissores**: ESPs que enviam dados periodicamente para o hub central

A rede mesh automaticamente roteia as mensagens através de outros nós quando necessário, garantindo que as mensagens cheguem ao hub mesmo quando os transmissores não estão em alcance direto.

## 📁 Arquivos

- `CentralHub_Receptor.ino` - Código para o ESP que funciona como hub central
- `CentralHub_Transmissor.ino` - Código para os ESPs transmissores
- `README.md` - Este arquivo de documentação

## 🔧 Configuração

### Requisitos

- Arduino IDE
- Biblioteca m2mMesh instalada
- Placas ESP8266 ou ESP32
- Suporte para ESP8266/ESP32 no Arduino IDE

### Instalação da Biblioteca

1. Copie a pasta `m2mMesh` para `Documentos/Arduino/libraries/`
2. Reinicie o Arduino IDE

### Configuração do Hub Central

1. Abra `CentralHub_Receptor.ino` no Arduino IDE
2. Selecione sua placa ESP8266/ESP32
3. Compile e envie para um ESP que funcionará como hub central
4. Abra o monitor serial (115200 baud) para ver as mensagens recebidas

### Configuração dos Transmissores

1. Abra `CentralHub_Transmissor.ino` no Arduino IDE
2. Compile e envie para os ESPs que funcionarão como transmissores
3. Abra o monitor serial para acompanhar o envio de mensagens

**⚠️ IMPORTANTE**: O hub deve estar ligado e conectado à mesh antes dos transmissores para que seja encontrado corretamente.

## 🚀 Como Usar

### 1. Ligar o Hub Central

- Ligue o ESP com o código do hub central
- Aguarde aparecer "Hub pronto para receber mensagens!"
- O LED integrado acenderá quando conectado à mesh

### 2. Ligar os Transmissores

- Ligue os ESPs com o código dos transmissores
- Cada transmissor irá:
  - Gerar um nome único (ex: "Sensor_1234")
  - Procurar pelo hub "CentralHub"
  - Começar a enviar dados simulados de sensores

### 3. Monitoramento

**No Hub Central:**
- Recebe e exibe todas as mensagens dos transmissores
- Mostra estatísticas por nó
- Envia confirmações de recebimento
- Comandos disponíveis via serial (digite `help`)

**Nos Transmissores:**
- Enviam dados de sensores simulados a cada 15 segundos
- Mostram confirmações recebidas do hub
- Comandos disponíveis via serial (digite `help`)

## 📊 Funcionalidades

### Hub Central

- ✅ Recebe mensagens de todos os nós
- ✅ Mantém estatísticas detalhadas por transmissor
- ✅ Interface serial interativa com comandos
- ✅ Envio de confirmações automáticas
- ✅ LED de status integrado
- ✅ Monitoramento da saúde da mesh

### Transmissores

- ✅ Busca automática do hub na mesh
- ✅ Envio periódico de dados simulados
- ✅ Simulação de sensores (temperatura, umidade, luz, movimento)
- ✅ Roteamento automático através da mesh
- ✅ Recebimento de confirmações do hub
- ✅ Interface serial para controle manual

## 🎛️ Comandos Seriais

### Hub Central

```
help          - Mostrar ajuda
status        - Status detalhado do hub
stats         - Estatísticas por nó
nodes         - Listar nós conectados
reset         - Resetar estatísticas
send <msg>    - Enviar mensagem broadcast
msg <id> <msg> - Enviar para nó específico
```

### Transmissores

```
help               - Mostrar ajuda
status             - Status do transmissor
sensors            - Dados atuais dos sensores
send               - Forçar envio de mensagem
search             - Forçar busca do hub
interval <seg>     - Alterar intervalo (5-300s)
temp <valor>       - Definir temperatura
hum <valor>        - Definir umidade
light <valor>      - Definir luz
motion on/off      - Controlar movimento
reset              - Resetar estatísticas
```

## 🔄 Roteamento Automático

A biblioteca m2mMesh automaticamente:

1. **Descobre rotas**: Encontra o melhor caminho para o hub
2. **Roteia mensagens**: Passa mensagens através de nós intermediários quando necessário
3. **Adapta-se a mudanças**: Recalcula rotas quando nós saem ou entram na rede
4. **Otimiza qualidade**: Escolhe rotas com melhor qualidade de sinal

### Exemplo de Roteamento

```
Transmissor A → Nó Intermediário B → Hub Central
```

Se o Transmissor A não conseguir alcançar o Hub diretamente, a mesh automaticamente enviará a mensagem através do Nó B.

## 📱 Dados dos Sensores Simulados

Cada transmissor simula os seguintes sensores:

- 🌡️ **Temperatura**: 15-35°C (varia ±2°C)
- 💧 **Umidade**: 30-70% (varia ±5%)
- 💡 **Luz**: 100-1000 lux (varia ±100)
- 🚶 **Movimento**: Detecção aleatória (20% chance)

Os valores mudam automaticamente para simular condições reais.

## 🔧 Configurações Avançadas

### Personalizar Nomes

**Hub Central:**
```cpp
const String HUB_NAME = "MeuHub";  // Altere aqui
```

**Transmissores:**
```cpp
const String HUB_NAME = "MeuHub";        // Deve ser igual ao hub
const String NODE_PREFIX = "Sensor";     // Prefixo dos transmissores
```

### Ajustar Intervalos

```cpp
unsigned long messageInterval = 15000;     // Intervalo de envio (ms)
unsigned long hubSearchInterval = 10000;   // Intervalo de busca do hub (ms)
```

### Alterar Canal e Tamanho da Mesh

```cpp
const uint8_t MAX_NODES = 20;      // Máximo de nós
const uint8_t MESH_CHANNEL = 1;    // Canal WiFi (1-13)
```

## 🐛 Resolução de Problemas

### Hub não encontrado

1. Verifique se o hub está ligado e conectado
2. Confirme que ambos usam o mesmo `HUB_NAME`
3. Verifique se estão no mesmo canal
4. Use o comando `search` no transmissor

### Mensagens não chegam

1. Verifique a qualidade do sinal
2. Use o comando `status` para ver a conectividade
3. Aproxime os dispositivos temporariamente
4. Verifique se há nós intermediários suficientes

### Performance baixa

1. Reduza o número máximo de nós se não necessário
2. Ajuste os intervalos de envio
3. Verifique interferências WiFi no canal
4. Use canais menos congestionados

## 📊 Exemplo de Output

### Hub Central
```
📥 ===== MENSAGEM RECEBIDA =====
📍 Remetente: Nó ID 2 (Sensor_1234) - MAC: AA:BB:CC:DD:EE:FF
📦 Conteúdo da mensagem:
   [1] Texto: Dados do Sensor_1234 #15
   [2] Float: 24.50
   [3] Float: 65.20
   [4] Número: 750
   [5] Boolean: false
⏰ Timestamp: 45230ms
📊 Total de mensagens recebidas: 15
✅ Mensagem processada com sucesso
📤 Confirmação enviada para nó 2 (Sensor_1234)
```

### Transmissor
```
📤 Dados enviados para hub (msg #15)
   🌡️  Temp: 24.5°C
   💧 Umidade: 65.2%
   💡 Luz: 750 lux
   🚶 Movimento: Não
✅ Confirmação recebida do hub!
   📄 Texto: ACK: Mensagem recebida pelo hub CentralHub
```

## 🔗 Próximos Passos

1. **Sensores Reais**: Substitua os dados simulados por sensores reais
2. **Banco de Dados**: Conecte o hub a um banco de dados
3. **Interface Web**: Crie uma interface web para visualizar os dados
4. **MQTT**: Integre com broker MQTT para IoT
5. **Alertas**: Implemente sistema de alertas baseado nos dados

## 📚 Referências

- [Documentação m2mMesh](../README.md)
- [Outros Exemplos](../examples/)
- [ESP-NOW Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)