# Guia Completo - Como Compilar m2mMesh no Arduino IDE

## 📋 Pré-requisitos

### Hardware Necessário
- **ESP8266** (NodeMCU, Wemos D1 Mini, etc.) OU **ESP32** (ESP32 DevKit, etc.)
- Pelo menos **2 dispositivos** para formar uma rede mesh
- Cabo USB para programação

### Software Necessário
- **Arduino IDE** (versão 1.8.19 ou superior)
- **Suporte para ESP8266/ESP32** no Arduino IDE

## 🔧 Passo 1: Instalar Arduino IDE

1. Baixe o Arduino IDE em: https://www.arduino.cc/en/software
2. Instale seguindo as instruções do seu sistema operacional

## 🌐 Passo 2: Adicionar Suporte para ESP8266/ESP32

### Para ESP8266:
1. Abra o Arduino IDE
2. Vá em **Arquivo** → **Preferências**
3. No campo **"URLs Adicionais do Gerenciador de Placas"** adicione:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```

### Para ESP32:
1. No mesmo campo **"URLs Adicionais do Gerenciador de Placas"** adicione (separar com vírgula se já tiver ESP8266):
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```

### Instalar os Pacotes:
1. Vá em **Ferramentas** → **Placa** → **Gerenciador de Placas**
2. Procure por **"esp8266"** e instale o pacote **"esp8266 by ESP8266 Community"**
3. Procure por **"ESP32"** e instale o pacote **"ESP32 by Espressif Systems"**

## 📚 Passo 3: Instalar a Biblioteca m2mMesh

### Método 1: Instalação Manual (Recomendado)
1. Baixe ou clone este repositório
2. Copie toda a pasta `m2mMesh` para:
   - **Windows**: `Documentos\Arduino\libraries\`
   - **macOS**: `~/Documents/Arduino/libraries/`
   - **Linux**: `~/Arduino/libraries/`

3. A estrutura deve ficar assim:
   ```
   Arduino/libraries/m2mMesh/
   ├── src/
   │   ├── m2mMesh.h
   │   └── m2mMesh.cpp
   ├── examples/
   ├── library.properties
   └── ...
   ```

### Método 2: Via ZIP
1. Faça download do repositório como ZIP
2. No Arduino IDE, vá em **Sketch** → **Incluir Biblioteca** → **Adicionar biblioteca .ZIP**
3. Selecione o arquivo ZIP baixado

## ⚙️ Passo 4: Configurar o Hardware

### Para ESP8266 (NodeMCU, Wemos D1 Mini):
1. Em **Ferramentas** → **Placa**, selecione sua placa específica:
   - **NodeMCU 1.0 (ESP-12E Module)**
   - **LOLIN(WEMOS) D1 R2 & mini**
   - etc.

### Para ESP32:
1. Em **Ferramentas** → **Placa**, selecione:
   - **ESP32 Dev Module**
   - **ESP32 DEVKIT V1**
   - etc.

### Configurações Importantes:
- **Velocidade de Upload**: 921600 ou 115200
- **Tamanho do Flash**: Conforme sua placa (geralmente 4MB)
- **Porta**: Selecione a porta COM onde está conectado

## 📝 Passo 5: Exemplo Básico

Crie um novo sketch e cole o código abaixo:

```cpp
#include <m2mMesh.h>

// Configuração do LED integrado
#if defined(LED_BUILTIN)
  #if defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_ESP8266_WEMOS_D1MINIPRO)
    #define LED_ON LOW
    #define LED_OFF HIGH
  #else
    #define LED_ON HIGH
    #define LED_OFF LOW
  #endif
#endif

bool joinedMesh = false;

void setup() {
  // Configurar LED
  #if defined(LED_BUILTIN)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);
  #endif
  
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Iniciando m2mMesh...");
  
  // Definir nome do nó
  m2mMesh.setNodeName("MeuNo");
  
  // Inicializar mesh (máx 16 nós, canal 1)
  if(m2mMesh.begin(16, 1)) {
    Serial.println("Mesh iniciada com sucesso!");
  } else {
    Serial.println("Erro ao iniciar mesh!");
  }
}

void loop() {
  // OBRIGATÓRIO: chamar pelo menos uma vez por segundo
  m2mMesh.housekeeping();
  
  // Verificar status da conexão
  if(joinedMesh == false && m2mMesh.joined() == true) {
    joinedMesh = true;
    #if defined(LED_BUILTIN)
    digitalWrite(LED_BUILTIN, LED_ON);
    #endif
    Serial.println("Conectado à mesh!");
  }
  
  // Verificar mensagens recebidas
  if(m2mMesh.messageWaiting()) {
    if(m2mMesh.nextDataType() == m2mMesh.USR_DATA_STRING) {
      Serial.print("Mensagem recebida: ");
      Serial.println(m2mMesh.retrieveString());
    }
    m2mMesh.markMessageRead();
  }
  
  delay(100);
}
```

## 🚀 Passo 6: Compilar e Enviar

1. **Conecte** o ESP8266/ESP32 ao computador via USB
2. **Selecione** a porta correta em **Ferramentas** → **Porta**
3. Clique no botão **Verificar** (✓) para compilar
4. Se não houver erros, clique em **Carregar** (→) para enviar o código

## 📊 Passo 7: Testar a Rede Mesh

### Teste com 2 Dispositivos:
1. Envie o mesmo código para 2 dispositivos ESP
2. Abra o **Monitor Serial** (Ctrl+Shift+M) para cada um
3. Configure a velocidade para **115200 baud**
4. Aguarde os dispositivos se conectarem automaticamente

### Exemplo de Envio de Mensagens:
```cpp
// Adicionar no loop() para enviar mensagem a cada 10 segundos
static unsigned long lastSend = 0;
if(millis() - lastSend > 10000 && joinedMesh) {
  lastSend = millis();
  
  m2mMesh.clearMessage();
  m2mMesh.add("Olá da mesh!");
  if(m2mMesh.send()) {
    Serial.println("Mensagem enviada!");
  }
}
```

## 🔧 Solução de Problemas

### Erro de Compilação:
- **"m2mMesh.h: No such file"**: Biblioteca não instalada corretamente
- **"ESP8266WiFi.h: No such file"**: Pacote ESP8266 não instalado
- **"WiFi.h: No such file"**: Para ESP32, verifique se o pacote ESP32 está instalado

### Problemas de Conexão:
- **"Mesh failed to start"**: Verifique se a placa está corretamente selecionada
- **Não conecta à mesh**: Certifique-se de que há pelo menos 2 dispositivos próximos
- **LED não acende**: Verifique as definições de LED_ON/LED_OFF para sua placa

### Problemas de Upload:
- **"Failed to connect"**: Pressione o botão RESET na placa durante o upload
- **Porta não aparece**: Instale os drivers USB-Serial da sua placa
- **Permission denied** (Linux): Execute `sudo chmod 666 /dev/ttyUSB*`

## 📈 Recursos Avançados

### Funcionalidades Disponíveis:
- ✅ **Rede mesh auto-organizável**
- ✅ **Roteamento automático de pacotes**
- ✅ **Suporte a múltiplos tipos de dados**
- ✅ **Sincronização de tempo**
- ✅ **Monitoramento de saúde da rede**
- ✅ **Crescimento dinâmico da rede**
- ✅ **Trace de rota**
- ✅ **Debug detalhado**

### Tipos de Dados Suportados:
- `String` e `char*`
- `bool`, `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`
- `int8_t`, `int16_t`, `int32_t`, `int64_t`
- `float`, `double`
- Arrays de qualquer tipo acima

## 📖 Exemplos Inclusos

A biblioteca inclui vários exemplos prontos:

1. **Example01_joinMesh**: Conectar à mesh básico
2. **Example02_helloMesh**: Envio de mensagens simples
3. **Example03_sendRandomDataTypes**: Diferentes tipos de dados
4. **Example04_displayReceivedData**: Processamento de dados recebidos
5. **Example05_setNodeName**: Configuração de nomes
6. **Example15_m2mMeshInfo01**: Monitor avançado da rede

## 🎯 Próximos Passos

1. **Teste o exemplo básico** com 2 dispositivos
2. **Experimente os exemplos inclusos** na biblioteca
3. **Adicione mais nós** para formar uma rede maior
4. **Desenvolva sua aplicação** usando a API da biblioteca

## 📚 Documentação Adicional

- **README.md**: Documentação principal
- **docs/**: Documentação técnica detalhada
- **examples/**: Exemplos práticos
- **Código fonte**: `src/m2mMesh.h` para referência da API

---

**🎉 Parabéns!** Agora você tem uma rede mesh ESP-NOW funcionando!
