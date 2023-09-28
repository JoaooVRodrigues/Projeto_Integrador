#include <OneWire.h>                  // Biblioteca utilizada para o Sensor de Temperatura
#include <DallasTemperature.h>        // Biblioteca utilizada para o Sensor de Temperatura
#include <SoftwareSerial.h>           // Biblioteca utilizada para comunicação Serial com o Esp

#define Ativar_Esp          false     // (=true) Ativa a comunicação com o Esp8266 para ligação ao servidor
#define delay_Inicial       60000     // (ms) Corresponde ao delay inicial para iniciar o sistema
#define corr_tempo_leitura  10000     // (ms) Corresponde ao tempo de leitura das correntes
#define Limite_Avarias      5         // Corresponde ao número de avarias consecutivas detetadas para gerar um alerta
#define delay_Amostra       10000     // (ms) Corresponde ao delay entre a leitura da próxima amostra

// Inicialização de Pinos
byte corr_vent_1 = A0;                // Define o pino analógico utilizado pelo sensor de corrente no ventilador
byte corr_bomb_1 = A4;                // Define o pino analógico utilizado pelo sensor de corrente na bomba
byte corr_comp_1 = A2;                // Define o pino analógico utilizado pelo sensor de corrente no compressor
byte sensors_pin =  2;                // Define o pino digital utilizado pelos sensores de temperatura

// Seleciona os sensores de temperatura
// Os endereços abaixo são determinados através de um programa auxiliar
uint8_t sensor_cond1[8] = {0x28, 0x88, 0xF0, 0x76, 0xE0, 0x01, 0x3C, 0x4C};
uint8_t sensor_ambi[8]  = {0x28, 0x53, 0xA3, 0x01, 0x00, 0x00, 0x00, 0x9B};
uint8_t sensor_cond2[8] = {0x28, 0x29, 0x00, 0x76, 0xE0, 0x01, 0x3C, 0x81};
uint8_t sensor_agua[8]  = {0x28, 0x55, 0x2F, 0x03, 0x00, 0x00, 0x00, 0xC4};
  
// Configura o pino de leitura das temperaturas
OneWire wire_sensor(sensors_pin);
DallasTemperature sensors(&wire_sensor);

// Configura os pinos de comunicação com o Esp
SoftwareSerial espSerial(5, 6);

// Inicialização de Limites
int Vent_Corr_Max   = 270;            // Define o valor máximo para a corrente no ventilador
int Vent_Corr_Min   = 180;            // Define o valor mínimo para a corrente no ventilador
int Bomb_Corr_Max   = 470;            // Define o valor máximo para a corrente na bomba
int Bomb_Corr_Min   = 180;            // Define o valor mínimo para a corrente na bomba
int Comp_Corr_Max   = 4500;           // Define o valor máximo para a corrente no compressor
int Comp_Corr_Min   = 2000;           // Define o valor mínimo para a corrente no compressor
int Cond_1_Temp_Max = 75;             // Define o valor máximo para a temperatura na entrada do condensador
int Cond_1_Temp_Min = 0;              // Define o valor mínimo para a temperatura na entrada do condensador (NAO É NECESSÁRIO)
int Cond_2_Temp_Max = 55;             // Define o valor máximo para a temperatura na saída do condensador
int Cond_2_Temp_Min = 0;              // Define o valor mínimo para a temperatura na saída do condensador 
int Cond_Dif_Min    = 5;              // Define o valor mínimo da diferença da temperatura entre a entrada e a saída do condensador
int Cond_Dif_Aviso  = 10;             // Define o valor mínimo da diferença da temperatura entre a entrada e a saída do condensador para aviso
int Agua_Temp_Max   = 1;              // Define o valor máximo para a temperatura na água
int Agua_Temp_Min   = -1;             // Define o valor mínimo para a temperatura na água
int Ambi_Temp_Ins   = 20;             // Define o valor normal da temperatura ambiente

// Inicialização das Variaveis de Controlo //
/* 
  Sensores de Corrente (Bomb, Vent, Comp, Cond[0,1]):
    0   - OK      - Valor dentro dos valores normais
    1   - Acima   - Valor acima dos valores normais
    -1  - Abaixo  - Valor abaixo dos valores normais

  Cond[2]:
    0 - OK
    1 - Aviso
    -1 - Abaixo do limite
*/
int Bomb=0, Vent=0, Comp=0, Agua=0, Ambi=0, Cond[3]={0,0,0};

// Variáveis de deteção de avarias
bool maq_obstruida=false, vent_queimado=false, comp_queimado=false, bomb_avariada=false, maq_ineficiente=false, maq_fuga=false;
bool arranque=true;

// Contadores de avarias consecutivas
int c_maq=0, c_vent=0, c_comp=0, c_bomb=0, c_inef=0, c_fuga=0;


void setup(){
  // Abre o serial USB
  Serial.begin(9600);
  while(!Serial){
    ;
  }
  
  // Abre as comunicações com o Esp
  if (Ativar_Esp){
    espSerial.begin(9600);
    while(!espSerial){}
  }
  
  // Inicialização de pinos
  pinMode(corr_bomb_1, INPUT);
  pinMode(corr_comp_1, INPUT);
  pinMode(corr_vent_1, INPUT);
  
  // Inicialização dos sensores de Temperatura
  sensors.begin();
  
  // Delay Inicial
  unsigned long restante;
  // Escreve no serial "Delay Inicial...\n --h--min--s
  Serial.println("Delay Inicial...");
  Serial.print(delay_Inicial/1000/3600);
  Serial.print("h");
  restante = delay_Inicial/1000 - (delay_Inicial/1000/3600)*3600;
  Serial.print(restante/60);
  Serial.print("min");
  restante = restante - (restante/60)*60;
  Serial.print(restante);
  Serial.println("s");

  // Aplica o delay, que pode ser interrompido pelo "pressionar tecla enter"
  unsigned long Inicial = millis();
  Serial.println("Pressione Enter para continuar...");
  while((millis()-Inicial) < delay_Inicial){
    if (Serial.available()){
      while (Serial.available())
          Serial.read();
      break;
    }
    // Se a temperatura da agua estiverem abaixo do seu valor máximo, o sistema de deteção inicia
    sensors.requestTemperatures();
    float agua = sensors.getTempC(sensor_agua);
    if (agua < Agua_Temp_Max)
      break;
    delay(100);
  }
  Serial.println("Configuração completa");
  Serial.print("Sistema iniciará em ");
  Serial.print(corr_tempo_leitura/1000);
  Serial.println("s...");
}

void loop(){
  // Leitura de Temperaturas
  sensors.requestTemperatures();
  float cond_1 = sensors.getTempC(sensor_cond1);
  float cond_2 = sensors.getTempC(sensor_cond2);
  float agua_1 = sensors.getTempC(sensor_agua);
  float ambi_1 = sensors.getTempC(sensor_ambi);
  
  // Leitura de Correntes
  float vent_1, bomb_1, comp_1;
  leCorrentes(&vent_1, &bomb_1, &comp_1);
  
  // Verifica os parâmetros lidos
  check(vent_1, comp_1, bomb_1, cond_1, cond_2, agua_1, ambi_1);
  
  // Escreve no serial os valores lidos
  for(int i = 30; i != 0; i--)
    Serial.println();
  serialTemperatura(cond_1, cond_2, agua_1, ambi_1);
  serialCorrente(vent_1, comp_1, bomb_1);
      
  // Escreve no serial as avarias existentes
  serialAvaria();
  
  // Envia as leituras para o Esp
  if (Ativar_Esp)
    espLeituras(cond_1, cond_2, agua_1, ambi_1, vent_1, bomb_1, comp_1);

  // Delay entre amostras
  Serial.print((delay_Amostra+corr_tempo_leitura)/1000);
  Serial.println("s para a próxima amostra...");
  delay(delay_Amostra);
}

/******************************* Funções de Cálculo *******************************/

// Lê as correntes de todos os sensores de corrente
void leCorrentes(float* vent, float* bomb, float* comp){
    int i = 0;
    float soma_vent = 0, soma_bomb = 0, soma_comp = 0;

    uint32_t tempo = millis();
    while((millis()-tempo) < corr_tempo_leitura){
        float vent_1 = analogRead(corr_vent_1);
        float bomb_1 = analogRead(corr_bomb_1);
        float comp_1 = analogRead(corr_comp_1);

        vent_1 = ((vent_1 *(5.0/1024.0))-2.5)/0.185;
        bomb_1 = ((bomb_1 *(5.0/1024.0))-2.5)/0.185;
        comp_1 = ((comp_1 *(5.0/1024.0))-2.5)/0.185;
        
        soma_vent += sq(vent_1);
        soma_bomb += sq(bomb_1);
        soma_comp += sq(comp_1);
        
        i+=1;
        delay(1);
    }
    
    float vent_rms = soma_vent/(float)i;
    float bomb_rms = soma_bomb/(float)i;
    float comp_rms = soma_comp/(float)i;

    vent_rms = sqrt(vent_rms);
    bomb_rms = sqrt(bomb_rms);
    comp_rms = sqrt(comp_rms);

    *vent = vent_rms;
    *bomb = bomb_rms;
    *comp = comp_rms;

    return;
}

/**************************** Verificação de Parâmetros ***************************/

// Verifica todos os parâmetros
void check(float vent_1, float comp_1, float bomb_1, float cond_1, float cond_2, float agua_1, float ambi_1){
  checkVent(vent_1);
  checkComp(comp_1);
  checkBomb(bomb_1);
  checkCond(cond_1 , cond_2);
  checkAgua(agua_1);
  checkAmbi(ambi_1);

  Avarias();
  return;
}

// Verifica corrente do Ventilador
void checkVent(float vent_1){
  if(vent_1 <= Vent_Corr_Min) {
      Vent = -1; 
  }else if(vent_1 >= Vent_Corr_Max) {
      Vent = 1;
  }else {
      Vent = 0;
  }
  return;
}

// Verifica Corrente da Bomba
void checkBomb(float bomb_1){
  if (bomb_1 <= Bomb_Corr_Min){
      Bomb = -1;
  }else if (bomb_1 >= Bomb_Corr_Max){
      Bomb = 1;
  }else{
      Bomb = 0;
  }
  return;
}

// Verifica Corrente do Compressor
void checkComp(float comp_1){
  if(comp_1 <= Comp_Corr_Min){
      Comp = -1;
  }else if(comp_1 >= Comp_Corr_Max){
      Comp = 1;
  }else{
      Comp = 0;
  }
  return;
}

// Verifica Temperatura do Condensador
void checkCond(float cond_1, float cond_2){
  float T_Dif = cond_1 - cond_2;
  // Limites da temperatura do condensador no inicio
  if(cond_1 < Cond_1_Temp_Min){
      Cond[0] = -1;
  }else if(cond_1 > Cond_1_Temp_Max){
      Cond[0] = 1;
  }else{ 
      Cond[0] = 0;
  }
  
  // Limites da temperatura do condensador no fim
  if(cond_2 < Cond_2_Temp_Min){
      Cond[1] = -1;
  }else if(cond_2 > Cond_2_Temp_Max){
      Cond[1] = 1;
  }else{
      Cond[1] = 0;
  }
  
  // Diferença entre temperaturas dos termometros dos condensadores
  if (T_Dif < Cond_Dif_Aviso){
      if (T_Dif < Cond_Dif_Min){
          Cond[2] = -1;
      } else{
          Cond[2] = 1;
      }
  } else {
      Cond[2] = 0;
  }
  return;
}

// Verifica Temperatura da Água
void checkAgua(float agua_1){
  if(agua_1 <= Agua_Temp_Min){
      Agua = -1;
  }else if(agua_1 >= Agua_Temp_Max){
      Agua = 1;
  }else{
      Agua = 0;
  }
  return;
}

// Verifica Temperatura Ambiente (não utilizado)
void checkAmbi(float ambi_1){
  if(ambi_1 > Ambi_Temp_Ins){
      Ambi = 1;
  }else{
      Ambi = 0;
  }
  return;
}

// Calcula a existência de avarias
void Avarias(){
  // Maquina_Obstruida ----------------------------------------------------
  if(Cond[2]==-1 && Vent==0 && Bomb==0){
      c_maq += 1;
      if(c_maq >= Limite_Avarias){ // limite de erros seguidos atingidos
         maq_obstruida = true; //ALERTAR maquina obstruida
      }
  }else{
      c_maq = 0;
      maq_obstruida = false;
  }
  
  // Ventilador_Queimado ----------------------------------------------------
  if(Cond[2]==-1 && Vent==-1){
      c_vent += 1;
      if(c_vent >= Limite_Avarias){ // limite de erros seguidos atingidos
         vent_queimado = true; //ALERTAR ventilador queimado
      }
  }else{
      c_vent = 0; 
      vent_queimado = false;    
  }
  
  // Compressor_Queimado ----------------------------------------------------
  if(Cond[2]==-1 && Comp==-1){
      c_comp += 1;
      if(c_comp >= Limite_Avarias){ // limite de erros seguidos atingidos
         comp_queimado = true; //ALERTAR compressor queimado
      }
  }else{
      c_comp = 0;
      comp_queimado = false;    
  }
  
  // Bomba Queimada ----------------------------------------------------------
  if(Agua!=0 && Bomb==-1){
      c_bomb += 1;
      if(c_bomb >= Limite_Avarias){
          bomb_avariada = true; //ALERTAR bomba avariada
      }
  }
  else{
    c_bomb = 0;
    bomb_avariada = false;
  }
  
  // Ineficiência da Máquina ------------------------------------------------
  if (Agua==1 && Vent==0 && Comp==0 && Bomb==0){
      c_inef += 1;
      if (c_inef >= Limite_Avarias){
        maq_ineficiente = true; // ALERTAR ineficiência da máquina (possível acumulação de lixo)
      }
  }
  else{
      c_inef = 0;
      maq_ineficiente = false;
  }
  
  // Fuga na máquina --------------------------------------------------------
  if (false){
    c_fuga += 1;
    if (c_fuga >= Limite_Avarias){
      maq_fuga = true; // ALERTAR fuga na máquina
    }
  }
  else{
    c_fuga = 0;
    maq_fuga = false;
  }
}

/******************************** Escritas no Esp *********************************/

// Escreve no Esp os valores de temperatura e corrente lidos
void  espLeituras(float cond_1, float cond_2, float agua_1, float ambi_1, float vent_1, float bomb_1, float comp_1){
  espSerial.println(cond_1);
  espSerial.println(cond_2);
  espSerial.println(agua_1);
  espSerial.println(ambi_1);
  espSerial.println(vent_1);
  espSerial.println(bomb_1);
  espSerial.println(comp_1);
  
  espSerial.println(maq_obstruida);
  espSerial.println(vent_queimado);
  espSerial.println(comp_queimado);
  espSerial.println(bomb_avariada);
  espSerial.println(maq_ineficiente);
  espSerial.println(maq_fuga);
  return;
}

/******************************* Escritas no Serial *******************************/

// Escreve as temperaturas lidas no serial
void serialTemperatura(float Cond_1, float Cond_2, float Agua_1, float Ambi_1){
  Serial.println(F("***********************************************************************"));
  Serial.println(F("                             TEMPERATURAS                              "));
  Serial.print(F("Cond_ent : "));
  Serial.print(Cond_1,2);
  Serial.print((char)176);
  Serial.println("C");
  Serial.print(F("Cond_sai : "));
  Serial.print(Cond_2,2);
  Serial.print((char)176);
  Serial.println("C");
  Serial.print(F("Agua     : "));
  Serial.print(Agua_1,2);
  Serial.print((char)176);
  Serial.println("C");
  Serial.print(F("Ambi     : "));
  Serial.print(Ambi_1,2);
  Serial.print((char)176);
  Serial.println("C");
  Serial.println();
  return;
}

// Escreve as correntes lidas no serial
void serialCorrente(float Vent_1, float Comp_1, float Bomb_1){
  Serial.println(F("************************************************************************"));
  Serial.println(F("                               CORRENTES                                "));
  Serial.print(F("Vent     : "));
  Serial.print(Vent_1, 2);
  Serial.println(F(" mA"));
  Serial.print(F("Comp     : "));
  Serial.print(Comp_1, 2);
  Serial.println(F(" mA"));
  Serial.print(F("Bomb     : "));
  Serial.print(Bomb_1, 2);
  Serial.println(F(" mA"));
  Serial.println();
  return;
}

// Escreve as avarias no Serial
void serialAvaria(){
  Serial.println(F("************************************************************************"));
  Serial.println(F("                              Anonalias                                 "));
  if (Bomb == 0 && Vent == 0 && Comp == 0 && Agua == 0 && Ambi == 0 && Cond[0] == 0 && Cond[1] == 0 && Cond[2]==0){
      Serial.println(F("Sem anomalias a relatar..."));
  }
  else{
      serialBombCorr();
      serialVentCorr();
      serialCompCorr();
      serialAguaTemp();
      serialAmbiTemp();
      serialCondTemp();
  }
  Serial.println(F("************************************************************************"));
  Serial.println(F("                                Avarias                                 "));
  if (maq_obstruida)
    Serial.println("Máquina Obstruida");
  if (vent_queimado)
    Serial.println("Ventilador Queimado");
  if (comp_queimado)
    Serial.println("Compressor Queimado");
  if (bomb_avariada)
    Serial.println("Bomba Avariada");
  if (maq_ineficiente)
    Serial.println("Máquina Ineficiente");
  if (maq_fuga)
    Serial.println("Fuga na Máquina");
  Serial.println();
  Serial.println(F("************************************************************************"));
  return;
}

// Escreve no Serial a verificaçao da corrente na Bomba
void serialBombCorr(){
  if (Bomb==1){
      Serial.println(F("Corrente da bomba demasiado alta!"));
  }
  else if (Bomb==-1){
      Serial.println(F("Corrente da bomba demasiado baixa!"));
  }
  return;
}
// Escreve no Serial a verificaçao da corrente no Ventilador
void serialVentCorr(){
  if (Vent==1){
      Serial.println(F("Corrente do Ventilador demasiado alta!"));
  }
  else if (Vent==-1){
      Serial.println(F("Corrente do Ventilador demasiado baixa!"));
  }
  return;
}
// Escreve no Serial a verificaçao da corrente no Compressor
void serialCompCorr(){
  if (Comp==1){
      Serial.println(F("Corrente do Compressor demasiado alta!"));
  }
  else if (Comp==-1){
      Serial.println(F("Corrente do Compressor demasiado baixa!"));
  }
  return;
}
// Escreve no Serial a verificaçao da temperatura da Água
void serialAguaTemp(){
  if (Agua==1){
      Serial.println(F("Temperatura da Água demasiado alta!"));
  }
  else if (Agua==-1){
      Serial.println(F("Temperatura da Água demasiado baixa!"));
  }
  return;
}
// Escreve no Serial a verificaçao da temperatura do condensador
void serialCondTemp(){
  if (Cond[0]==1){
      Serial.println(F("Temperatura da Entrada do Condensador demasiado alta!"));
  }
  else if (Cond[0]==-1){
      Serial.println(F("Temperatura da Entrada do Condensador demasiado baixa!"));
  }

  if (Cond[1]==1){
      Serial.println(F("Temperatura da Saída do Condensador demasiado alta!"));
  }
  else if (Cond[1]==-1){
      Serial.println(F("Temperatura da Saída do Condensador demasiado baixa!"));
  }

  if (Cond[2]==1){
      Serial.println(F("Diferença de Temperatura no Condensador baixa - Baixo rendimento!"));
  }
  else if (Cond[2]==-1){
      Serial.println(F("Diferença de Temperatura no Condensador demasiado baixa!"));
  }
  return;
}
// Escreve no Serial a verificaçao da temperatura Ambiente
void serialAmbiTemp(){
  if (Ambi==1){
      Serial.println(F("Temperatura Ambiente acima do normal!"));
  }
  else if (Ambi==-1){
      Serial.println(F("Temperatura Ambiente abaixo do normal!"));
  }
  return;
}

// Escrito dos dados no Serial para análise em Excel
void serialExcel(float vent_1, float comp_1, float bomb_1, float cond_1, float cond_2, float agua_1, float ambi_1){
  Serial.print(F("Entrada: "));
  Serial.println(cond_1);
  Serial.print(F("Saida: "));
  Serial.println(cond_2);
  Serial.print(F("Bomba: "));
  Serial.println(bomb_1);
  Serial.print(F("Ventilador: "));
  Serial.println(vent_1);
  Serial.print(F("Compressor: "));
  Serial.println(comp_1);
  Serial.print(F("Agua: "));
  Serial.println(agua_1);
  Serial.print(F("Ambiente: "));
  Serial.println(ambi_1);
  return;
}
