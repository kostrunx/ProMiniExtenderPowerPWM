/* verzija 2.1.1
 *  sprememba dne 02.01.2021
 *  dodana definicija pinov DIO7 in DIO8 kot neprikljucenih pinov, ker gresta na ESP12 in 14. Definirana kot DI s pullup, da ne povzroci prevelikega toka skozi DIO.
 *  
 *  kreirana dne 30.08.2020
 *  popravek zacetnega PWM ob vklopu napajanja iz 30 na 0
 *  kompleten redesign
 *  * dodajanje moznosti readback vrednosti PWM
 *  * PWM se sedaj spreminja zvezno od trenutne do zeljene vrednosti
 *  * imena spremenljivk so poenotena s 4 ch impulzni rele MOSFET
 *  
 */

//ProMiniExtender zacetek
#include <Wire.h>
#define I2C_MSG_IN_SIZE    4
#define I2C_MSG_OUT_SIZE   4
#define CMD_DIGITAL_WRITE  1
#define CMD_DIGITAL_READ   2
#define CMD_ANALOG_WRITE   3
#define CMD_ANALOG_READ    4

volatile uint8_t sendBuffer[I2C_MSG_OUT_SIZE];
//ProMiniExtender konec

//moja koda zacetek
//definiranje digitalnih spremenljivk
boolean TickDown = false;         //spremenljivka za nov cikel zmanjsevanja izhodnega PWM
boolean TickUp = false;           //spremenljivka za nov cikel povecevanja izhodnega PWM
boolean OutEnable = false;        //vsi izhodi so onemogoceni
boolean SpremeniOut1 = false;     //zastavica za spreminjanje stanja izhoda 1
boolean SpremeniOut2 = false;     //zastavica za spreminjanje stanja izhoda 2
boolean ZmanjsajOut1 = false;     //zastavica za spreminjanje stanja izhoda 1
boolean ZmanjsajOut2 = false;     //zastavica za spreminjanje stanja izhoda 2
boolean PovecajOut1 = false;      //zastavica za spreminjanje stanja izhoda 1
boolean PovecajOut2 = false;      //zastavica za spreminjanje stanja izhoda 2

//definiranje numericnih spremenljivk
int DelayUp = 9;                  //zacetna hitrost povecevanja PWM
int DelayDown = 39;               //zacetna hitrost zmanjsevanja PWM  ** za škarpo je 79 (20 sekund za max to 0), za ostale pa 39 (10 sekund za max to 0)**
int PWMOut1Zeljena = 0;           //zacetni zeljeni PWM izhoda1      
int PWMOut2Zeljena = 0;           //zacetni zeljeni PWM izhoda2
int PWMOut1Dejanska = 0;          //zacetni PWM izhoda1
int PWMOut2Dejanska = 0;          //zacetni PWM izhoda2
int LEDStanje = LOW;
int valueRead = 0;                //zacetna vrednost spremenljivke za branje AIN
int UtripanjeLED = 500;           //vrednost casovnika za utripanje LED diode

unsigned long TimerDown = 0;      //casovnik za ms cikel za zmanjsevanje izhodnega PWM
unsigned long TimerUp = 0;        //casovnik za ms cikel za povecevanje izhodnega PWM
unsigned long LEDTimer = 0;       //casovnik za utripanje LED na PRO mini

//definiranje pinov
const int LEDPin = 13;            //LED na PRO mini
const int Out1Pin = 5;            //DIO5 je OUT1
const int Out2Pin = 6;            //DIO6 je OUT2
const int NC1Pin = 7;             //neporabljen pin 1
const int NC2Pin = 8;             //neporabljen pin 2
//moja koda konec

void setup() {
  //ProMiniExtender zacetek
  Wire.begin(0x7f);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  //ProMiniExtender konec

  //moja koda zacetek
  pinMode(LEDPin, OUTPUT);
  pinMode(Out1Pin, OUTPUT);
  pinMode(Out2Pin, OUTPUT);
  pinMode(NC1Pin, INPUT_PULLUP);
  pinMode(NC2Pin, INPUT_PULLUP);
    analogWrite(Out1Pin,PWMOut1Dejanska);     //postavi zacetno stanje izhoda 1
  analogWrite(Out2Pin,PWMOut2Dejanska);     //postavi zacetno stanje izhoda 2
  //moja koda konec
}

void loop() {

  //moja koda zacetek
  if (millis() < TimerDown) {
    TimerDown = 0;                          //reset stevca TimerDown ob prelivu funkcije millis()
  }
  if (millis() < TimerUp) {
    TimerUp = 0;                            //reset stevca TimerUp ob prelivu funkcije millis()
  }
  if (millis() > (TimerDown + DelayDown)) { //ce je cas za zmanjsevanje PWM ze minil
    TimerDown = millis();                   //potem postavi casovnik in pricni nov cikel
    TickDown = true;                        //postavi zastavico TickDown, da se izvrsi nov programski cikel
  }
  if (millis() > (TimerUp + DelayUp)) {     //ce je cas za povecevanje PWM ze minil
    TimerUp = millis();                     //potem postavi casovnik in pricni nov cikel
    TickUp = true;                          //postavi zastavico TickUp, da se izvrsi nov programski cikel
  }
  if (SpremeniOut1 == true) {               //ce je prislo do zahteve po novi analogni vrednosti za PWM1
    if (PWMOut1Zeljena < PWMOut1Dejanska) { //in ce je zeljeni PWM manjsi od dejanskega
      ZmanjsajOut1 = true;                  //potem postavi zastavico za zmanjsanje izhodnega PWM
      PovecajOut1 = false;                  //in pobrisi zastavico za povecevanje izhodnega PWM
      SpremeniOut1 = false;                 //in pobrisi zastavico za zahtevo po spreminjanju izhoda
    }
    if (PWMOut1Zeljena > PWMOut1Dejanska) { //ce pa je zeljeni PWM vecji od dejanskega
      ZmanjsajOut1 = false;                 //potem pobrisi zastavico za zmanjsanje izhodnega PWM
      PovecajOut1 = true;                   //in postavi zastavico za povecevanje izhodnega PWM
      SpremeniOut1 = false;                 //in pobrisi zastavico za zahtevo po spreminjanju izhoda
    }
  }
  if (SpremeniOut2 == true) {               //ce je prislo do zahteve po novi analogni vrednosti za PWM2
    if (PWMOut2Zeljena < PWMOut2Dejanska) { //in ce je zeljeni PWM manjsi od dejanskega
      ZmanjsajOut2 = true;                  //potem postavi zastavico za zmanjsanje izhodnega PWM
      PovecajOut2 = false;                  //in pobrisi zastavico za povecevanje izhodnega PWM
      SpremeniOut2 = false;                 //in pobrisi zastavico za zahtevo po spreminjanju izhoda
    }
    if (PWMOut2Zeljena > PWMOut2Dejanska) { //ce pa je zeljeni PWM vecji od dejanskega
      ZmanjsajOut2 = false;                 //potem pobrisi zastavico za zmanjsanje izhodnega PWM
      PovecajOut2 = true;                   //in postavi zastavico za povecevanje izhodnega PWM
      SpremeniOut2 = false;                 //in pobrisi zastavico za zahtevo po spreminjanju izhoda
    }
  }
  if (TickDown == true) {
    if (ZmanjsajOut1 == true) {
      SubZmanjsajOut1();                    //pojdi na subrutino za spreminjanje izhoda 1
    }
    if (ZmanjsajOut2 == true) {
      SubZmanjsajOut2();                    //pojdi na subrutino za spreminjanje izhoda 2
    }
    TickDown = false;                       //na koncu cikla pobrisi zastavico, da se pricne naslednji 7ms cikel
  }
  if (TickUp == true) {
    if (PovecajOut1 == true) {
      SubPovecajOut1();                     //pojdi na subrutino za spreminjanje izhoda 1
    }
    if (PovecajOut2 == true) {
      SubPovecajOut2();                     //pojdi na subrutino za spreminjanje izhoda 2
    }
    SubRazno();                             //pojdi na subrutino za ostale aktivnosti
    TickUp = false;                         //na koncu cikla pobrisi zastavico, da se pricne naslednji 7ms cikel
  }
}

void SubRazno() {
  if (millis() < LEDTimer) {
    LEDTimer = 0;                                   //reset stevca LEDTimer ob prelivu funkcije millis()
  }
  if (millis() > (LEDTimer + UtripanjeLED)) {       //ce je minilo ze dovolj casa
    LEDTimer = millis();                            //spremeni vrednost stevca
    LEDStanje = !LEDStanje;                         //negiraj stanje LED diode
    digitalWrite(LEDPin, LEDStanje);                //postavi izhod na novo stanje
  }
}

void SubZmanjsajOut1() {
  if (PWMOut1Zeljena < PWMOut1Dejanska) {       //ce je zeljeni PWM manjsi od dejanskega
    PWMOut1Dejanska = PWMOut1Dejanska - 1;      //potem zmanjsaj dejanski PWM
  }
  if (PWMOut1Zeljena == PWMOut1Dejanska) {      //ce je zeljeni PWM enak dejanskemu
    ZmanjsajOut1 = false;                       //potem prepreci nadaljnje spreminjanje dejanskega PWM
  }
  analogWrite(Out1Pin, PWMOut1Dejanska);        //zapisi dejanski PWM na izhod 1
}

void SubPovecajOut1() {
  if (PWMOut1Zeljena > PWMOut1Dejanska) {       //ce je zeljeni PWM vecji od dejanskega
    PWMOut1Dejanska = PWMOut1Dejanska + 1;      //potem povecaj dejanski PWM
  }
  if (PWMOut1Zeljena == PWMOut1Dejanska) {      //ce je zeljeni PWM enak dejanskemu
    PovecajOut1 = false;                        //potem prepreci nadaljnje spreminjanje dejanskega PWM
  }
  analogWrite(Out1Pin, PWMOut1Dejanska);        //zapisi dejanski PWM na izhod 1
}

void SubZmanjsajOut2() {
  if (PWMOut2Zeljena < PWMOut2Dejanska) {       //ce je zeljeni PWM manjsi od dejanskega
    PWMOut2Dejanska = PWMOut2Dejanska - 1;      //potem zmanjsaj dejanski PWM
  }
  if (PWMOut2Zeljena == PWMOut2Dejanska) {      //ce je zeljeni PWM enak dejanskemu
    ZmanjsajOut2 = false;                       //potem prepreci nadaljnje spreminjanje dejanskega PWM
  }
  analogWrite(Out2Pin, PWMOut2Dejanska);        //zapisi dejanski PWM na izhod 1
}

void SubPovecajOut2() {
  if (PWMOut2Zeljena > PWMOut2Dejanska) {       //ce je zeljeni PWM nekoliko vecji od dejanskega
    PWMOut2Dejanska = PWMOut2Dejanska + 1;      //potem povecaj dejanski PWM za majhen korak
  }
  if (PWMOut2Zeljena == PWMOut2Dejanska) {      //ce je zeljeni PWM enak dejanskemu
    PovecajOut2 = false;                        //potem prepreci nadaljnje spreminjanje dejanskega PWM
  }
  analogWrite(Out2Pin, PWMOut2Dejanska);        //zapisi dejanski PWM na izhod 2
}
//moja koda konec

//ProMiniExtender zacetek
void receiveEvent(int count) {
  if (count == I2C_MSG_IN_SIZE) {
    byte cmd = Wire.read();
    byte port = Wire.read();
    int value = Wire.read();
    value += Wire.read()*256;
    switch(cmd) {
      case CMD_DIGITAL_WRITE:
        digitalWrite(port,value);
      break;
      case CMD_DIGITAL_READ:
        clearSendBuffer();
        sendBuffer[0] = digitalRead(port);
      break;
      case CMD_ANALOG_WRITE:
        if(value > 255) value = 255;            //ce je vrednost value pomotoma prevelika, jo omeji na max
        if(value < 0) value = 0;                //ce je vrednost value pomotoma premajhna, jo omeji na min
        switch (port) {                         //glede na zahtevan izhod
          case 1:                               //ko nastavljam PWM na izhodu 1
            PWMOut1Zeljena = value;             //priredi zeljeni PWM sprejetemu po komunikaciji
            SpremeniOut1 = true;                //in omogoci spreminjanje izhodnega PWM
          break;
          case 2:                               //ko nastavljam PWM na izhodu 2
            PWMOut2Zeljena = value;             //priredi zeljeni PWM sprejetemu po komunikaciji
            SpremeniOut2 = true;                //in omogoci spreminjanje izhodnega PWM
          break;
          case 200:                             //ko nastavljam hitrost povecevanja PWM
            DelayUp = value;                    //priredi hitrost povecevanja PWM sprejetemu po komunikaciji
          break;
          case 201:                             //ko nastavljam hitrost zmanjsevanja PWM
            DelayDown = value;                  //priredi hitrost zmanjsevanja PWM sprejetemu po komunikaciji
          break;
          }
        break;
      case CMD_ANALOG_READ:
        clearSendBuffer();
        switch (port) {
          case 221:                             //ce se bere port 221
            valueRead = PWMOut1Dejanska;        //poslji nazaj PWM vrednost izhoda 1
          break;
          case 222:                             //ce se bere port 222
            valueRead = PWMOut2Dejanska;        //poslji nazaj PWM vrednost izhoda 2
          break;
        }
        sendBuffer[0] = valueRead & 0xff;
        sendBuffer[1] = valueRead >> 8;
      break;
    }
  }  
}

void clearSendBuffer() {
  for(byte x=0; x < sizeof(sendBuffer); x++)
    sendBuffer[x]=0;
}

void requestEvent() {
  Wire.write((const uint8_t*)sendBuffer,sizeof(sendBuffer));
}
//ProMiniExtender konec
