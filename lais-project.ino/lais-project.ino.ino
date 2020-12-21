//*********************************** Macros **************************************

// LCD
#define RS 7
#define EN 6
#define D4 5
#define D5 4
#define D6 3
#define D7 2

// Entradas
#define input_signal A0
#define adc1_address 0x48
#define adc2_address 0x49
#define BUTTON1 8
#define BUTTON2 9

// Saidas
#define CSADC 9
#define SS_SD 10

// DAC
#define ENDERECO  0x60
// Tensão máxima (0-5v) e Frequência de saída
#define VMAX    5
#define F       1000
#define PONTOS  512

// Valores que são calculados sozinhos (não modificar)    
#define AMPLITUDE   round(VMAX*(4095.0/5.0))
#define PASSO       round(2*AMPLITUDE/PONTOS)
#define PONTOS_REAL round(2*AMPLITUDE/PASSO)
#define DELAY       max(0, 2*round(1000000.0/((float)F*PONTOS_REAL) - 85.94))


// Importando Bilbiotecas
#include <LiquidCrystal.h>
#include <math.h>
#include <Wire.h>
#include <ADS1110.h>
#include <SPI.h>
#include <SD.h>



// Iniciando Objetos
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
ADS1110 concet_adc(adc1_address);
ADS1110 current_adc(adc2_address);


float get_concentracao() {
  double concentracao = 0.0;
  float leitura = concet_adc.getData();
  double tensao = 0;
  double resistencia = 0;

  tensao = 2.048 * leitura / 32767;
  resistencia = (-5000*((2*tensao)-5))/((2*tensao)+5); // Isolamos a resistencia na equação da ponte de wheatstone
  concentracao = (640.0*100.0)/(resistencia);  // Calculo encontrado em: http://ponce.sdsu.edu/onlinesalinity.php
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Concentracao:");
  lcd.setCursor(0, 1);
  lcd.print((String)concentracao + "  ug/m");
  delay(50);
}

void volt_ciclica(int tempo){
  // Inicializa o cartão sd
   Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(SS_SD)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");
  String dataString = "";

  File dataFile = SD.open("datalog.csv", FILE_WRITE);
  if(dataFile){
    dataFile.println("Tempo[ms],tensao[V],corrente[uA]");
    dataFile.close();
    Serial.println("tensao[V],corrente[uA]");
  }else{
    Serial.println("ERRO");
  }

  int ref_tempo = millis();
  while(true){
    int16_t i;
   
    // Variação de 0 à pi da onda
    for(i = 0; i <= AMPLITUDE; i+= PASSO*2){
     
      fast_mode(i, i+PASSO);
      write_values();
      if(DELAY < 16383){
        delayMicroseconds(DELAY);
      }else
      {
        delay(DELAY/1000);
      }
    }
  
    // Variação de pi à 2pi da onda
    for(i = AMPLITUDE; i > PASSO; i -= PASSO*2){
      fast_mode(i, i-PASSO);
      write_values();
      if(DELAY < 16383){
        delayMicroseconds(DELAY);
      }
      else
      {
        delay(DELAY/1000);
      }
    }
  }
}

void fast_mode(uint16_t output_1, uint16_t output_2){
  Wire.beginTransmission(ENDERECO);

  Wire.write(output_1 >> 8);
  Wire.write(output_1 & 0xFF);
 
  Wire.write(output_2 >> 8);
  Wire.write(output_2 & 0xFF);
  Wire.endTransmission();
}

void write_values(){
  float time_value = 0;
  int leitura_tensao = analogRead(input_signal);
  float leitura_corrente = current_adc.getData();
  float tensao = 0;
  double tensao_to_corrente = 0;
  double corrente_ua = 0;
  
  // Calculo da corrente
  tensao_to_corrente = 2.048 * leitura_corrente / 32767; // Tensao * bit
  corrente_ua = 1000*tensao_to_corrente;
  
  // Calculo da tensao
  tensao = 5.0 * leitura_tensao / 1023;

  // Pega o valor de do tempo
  time_value = millis();
  
  String dataString = (String)time_value + "," +(String)tensao + "," + String(corrente_ua);
  
  File dataFile = SD.open("datalog.csv", FILE_WRITE);
  
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tensao injetada:");
  lcd.setCursor(0, 1);
  lcd.print(tensao);
  delay(50);
}


void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  
  // put your setup code here, to run once:
  
  Wire.begin();
  SPI.begin();
  lcd.begin(16, 2);
  Wire.begin(ENDERECO);
  Wire.setClock(400000L);

  // Configuração do adc de leitura de concentração
  concet_adc.setVref(EXT_REF);
  concet_adc.setRes(RES_16);

  // Configuração do adc de leitura de corrente
  current_adc.setVref(INT_REF);
  current_adc.setRes(RES_16);

  // Menu
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("1-VOLT CICLICA");
  lcd.setCursor(0,1);
  lcd.print("2-MEDIDA DE SAL");
}

void loop() {
  if(digitalRead(BUTTON1)==HIGH){
    volt_ciclica(100);
  }
  if(digitalRead(BUTTON2)==HIGH){
    while(true){
      get_concentracao();
    }
  }
}
