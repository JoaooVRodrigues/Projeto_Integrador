#include <ESP8266WiFi.h>  // Biblioteca do Esp
#include "projeto.h"      // Este ficheiro inclui o código da página web

// Configurações WiFi
const char* ssid      = "NOME_DA_REDE";
const char* password  = "PASSWORD";

// Declaração de variáveis de leitura
float cond_1 = 0;
float cond_2 = 0;
float agua_1 = 0;
float ambi_1 = 0;
float vent_1 = 0;
float bomb_1 = 0;
float comp_1 = 0;
bool maq_obstruida = false;
bool vent_queimado = false;
bool comp_queimado = false;
bool bomb_avariada = false;
bool maq_ineficiente = false;
bool maq_fuga = false;

// Configurações da página html
String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

// Definição da porta de comunicação
WiFiServer server(80);

String Request = "";

void setup(){
  // Abre o serial com o arduino/USB
  Serial.begin(9600);
  while(!Serial){
    ;
  }

  // Liga o WiFi
  Serial.printf("A ligar à rede: %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi ligado");

  // O website fica disponível no IP ligado
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Abre o servidor
  server.begin();
}

void loop(){
  WiFiClient client = server.available();
  // Verifica se existe cliente à espera
  if (client){
      Serial.println("\n[Cliente conectado]");
      Request = client.readStringUntil('\r');
      Serial.println(Request);

      if (Request.indexOf("getValores") > 0){
          Serial.println("Atualização de Página");
          String temp = header + cond_1 + "|" + cond_2 + "|" + agua_1 + "|" + ambi_1 + "|" + vent_1 + "|" + bomb_1 + "|" + comp_1 + "|" + maq_obstruida + "|" + vent_queimado + "|" + comp_queimado + "|" + bomb_avariada + "|" + maq_ineficiente + "|" + maq_fuga;
          client.print(temp);
          Serial.println("Valores novos enviados");
      } else {
          Serial.println("Pedido de nova página");
          String temp = header + htmlPage;
          client.print(temp);
          Serial.println("Código da nova página HTML enviado");
      }
      client.stop();
      Serial.println("[Cliente desconectado]");
  }

  // Verifica se existem novas leituras não lidas
  if (Serial.available() > 0){
      delay(1000);
      cond_1 = Serial.parseFloat();
      cond_2 = Serial.parseFloat();
      agua_1 = Serial.parseFloat();
      ambi_1 = Serial.parseFloat();
      vent_1 = Serial.parseFloat();
      bomb_1 = Serial.parseFloat();
      comp_1 = Serial.parseFloat();
      maq_obstruida   = Serial.parseInt();
      vent_queimado   = Serial.parseInt();
      comp_queimado   = Serial.parseInt();
      bomb_avariada   = Serial.parseInt();
      maq_ineficiente = Serial.parseInt();
      maq_fuga        = Serial.parseInt();
      // Esvazia o serial
      while(Serial.available() > 0){
          Serial.read();
      }
      
      Serial.println("Valores Atualizados!!!");
      Serial.print("Cond_1: "); Serial.println(cond_1);
      Serial.print("Cond_2: "); Serial.println(cond_2);
      Serial.print("Agua_1: "); Serial.println(agua_1);
      Serial.print("Ambi_1: "); Serial.println(ambi_1);
      Serial.print("Vent_1: "); Serial.println(vent_1);
      Serial.print("Bomb_1: "); Serial.println(bomb_1);
      Serial.print("Comp_1: "); Serial.println(comp_1);
      if (maq_obstruida)    Serial.println("Maquina Obstruida");
      if (vent_queimado)    Serial.println("Ventilador Queimado");
      if (comp_queimado)    Serial.println("Compressor Queimado");
      if (bomb_avariada)    Serial.println("Bomba Avariada");
      if (maq_ineficiente)  Serial.println("Máquina Ineficiente");
      if (maq_fuga)         Serial.println("Fuga na Máquina");
  }
}
