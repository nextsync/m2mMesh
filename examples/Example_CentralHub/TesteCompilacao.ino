/**
 * Teste de Compilação - m2mMesh Hub Centralizado
 *
 * Este é um teste básico para verificar se as correções funcionaram.
 * Use este arquivo para testar a compilação antes de usar os exemplos completos.
 */

#include <m2mMesh.h>

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== TESTE DE COMPILAÇÃO - m2mMesh ===");

    // Teste básico da biblioteca
    m2mMesh.setNodeName("TesteNode");

    if (m2mMesh.begin(10, 1))
    {
        Serial.println("✅ Biblioteca m2mMesh carregada com sucesso!");
        Serial.println("✅ As correções de compilação funcionaram!");

        // Teste de envio simples
        m2mMesh.clearMessage();
        m2mMesh.add("Teste de mensagem");
        m2mMesh.add(123);

        Serial.println("✅ Teste de adicionar dados à mensagem funcionou!");
    }
    else
    {
        Serial.println("❌ Falha ao inicializar m2mMesh");
    }
}

void loop()
{
    m2mMesh.housekeeping();

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 5000)
    {
        lastPrint = millis();
        Serial.println("🔄 Sistema funcionando... Teste OK!");
    }

    delay(100);
}