#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// Configurações do Ecrã OLED (I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define OLED_SDA 21
#define OLED_SCL 23

// Configurações do Sensor DHT11
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Configuração dos Pinos de Segurança Física
#define PIR_PIN 15
#define BUZZER_PIN 19

// Variáveis de Relógio Interno
unsigned long segundoAnterior = 0;
int horas = 12;
int minutos = 0;
int segundos = 0;

// Variáveis de Estado da Cibersegurança
unsigned long tempoTransicao = 0;
int estadoSimulado = 0;
String estadoAtual = "SEGURO";

void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);
  dht.begin();
  
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.println("SHIELD CENTRAL MASTER");
  display.setCursor(25, 40);
  display.println("BOOTING SYSTEM...");
  display.display();
  delay(2000);
}

void processarComandosSerial() {
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    
    // Comando recebido do Host Central (PC) para acertar a hora (Ex: TIME:14:30:00)
    if (comando.startsWith("TIME:")) {
      horas = comando.substring(5, 7).toInt();
      minutos = comando.substring(8, 10).toInt();
      segundos = comando.substring(11, 13).toInt();
    }
    // Comando para simular ataque forçado via PC
    else if (comando.startsWith("ATTACK:")) {
      estadoAtual = comando.substring(7);
    }
  }
}

void loop() {
  unsigned long tempoAtual = millis();
  
  // 1. Incremento do Relógio Interno (1 segundo)
  if (tempoAtual - segundoAnterior >= 1000) {
    segundoAnterior = tempoAtual;
    segundos++;
    if (segundos >= 60) { segundos = 0; minutos++; }
    if (minutos >= 60)  { minutos = 0; horas++; }
    if (horas >= 24)    { horas = 0; }
    
    // Envia telemetria limpa em formato JSON para a porta Serial do PC Central
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    Serial.printf("{\"hora\":\"%02d:%02d:%02d\",\"estado\":\"%s\",\"temp\":%.1f,\"hum\":%.1f}\n", 
                  horas, minutos, segundos, estadoAtual.c_str(), isnan(t)?0:t, isnan(h)?0:h);
  }

  // 2. Transições Automáticas de Teste (a cada 15 segundos)
  if (tempoAtual - tempoTransicao >= 15000) {
    tempoTransicao = tempoAtual;
    estadoSimulado = (estadoSimulado + 1) % 3;
    if (estadoSimulado == 0) estadoAtual = "SEGURO";
    else if (estadoSimulado == 1) estadoAtual = "CIBER_ATAQUE";
    else if (estadoSimulado == 2) estadoAtual = "FALSO_ALERTA";
  }

  // 3. Deteção Física pelo Sensor PIR (Prioridade Alta)
  if (digitalRead(PIR_PIN) == HIGH) {
    estadoAtual = "INTRUSAO";
  }

  // 4. Leitura de Comandos da Porta Serial (PC)
  processarComandosSerial();

  // 5. Atualização do Ecrã OLED
  atualizarInterfaceOLED();
}

void atualizarInterfaceOLED() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  // Exibição do Relógio
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("%02d:%02d:%02d", horas, minutos, segundos);
  
  // Exibição dos dados do Sensor DHT11
  display.setCursor(65, 0);
  if (!isnan(t)) display.printf("%.1fC ", t);
  if (!isnan(h)) display.printf("%.0f%%", h);
  display.drawLine(0, 10, 128, 10, WHITE);
  
  // Renderização por Estado de Segurança
  if (estadoAtual == "SEGURO") {
    digitalWrite(BUZZER_PIN, LOW);
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println("STATUS: SEGURO");
    display.setCursor(0, 40);
    display.println("PERIMETRO OK");
  } 
  else if (estadoAtual == "INTRUSAO") {
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println("INTRUSAO!");
    display.setTextSize(1);
    display.setCursor(0, 45);
    display.println("MOVIMENTO DETETADO");
    
    digitalWrite(BUZZER_PIN, HIGH); delay(80);
    digitalWrite(BUZZER_PIN, LOW);  delay(80);
  }
  else if (estadoAtual == "CIBER_ATAQUE") {
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println("ALERTA: CIBER ATAQUE");
    display.setCursor(0, 40);
    display.println("FIREWALL BREACHED!");
    
    digitalWrite(BUZZER_PIN, HIGH); delay(150);
    digitalWrite(BUZZER_PIN, LOW);  delay(150);
  }
  else if (estadoAtual == "FALSO_ALERTA") {
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println("ERR: SYSTEM");
    display.setCursor(0, 40);
    display.println("FALSO ALERTA TESTE");
  }
  
  display.display();
}
