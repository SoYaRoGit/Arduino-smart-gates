/*
  Created 2023
  by Sokolov Yaroslav
*/

//-------БИБЛИОТЕКИ---------
#include <IRremote.hpp>  // для работы с ИК датчиками
#include <GyverTM1637.h> // для работы с дисплеем
#include <Servo.h>       // для работы с сервомашинкой
//-------БИБЛИОТЕКИ---------

//-------МОДУЛИ---------
// датчики на въезд
#define PIN_IR_SEND1 5   // первый ик передатчик
#define PIN_IR_RECV1 6   // первый ик приемник

// датчики на выезд
#define PIN_IR_SEND2 7   // второй ик передатчик
#define PIN_IR_RECV2 8   // второй ик приемник

// дисплей
#define PIN_DICP_CLK 9   // тактовый сигнал дисплея
#define PIN_DICP_DIO 10  // дисплей

// остальные модули
#define PIN_LED_GREEN 2  // зеленый диод
#define PIN_LED_RED 3    // красный диод
#define PIN_SERVO 4      // сервомашинка
#define PIN_SPEAKER 11   // спикер
#define PIN_BUTTON 12    // wi-fi
//-------МОДУЛИ---------

//-------НАСТРОЙКИ---------
uint32_t buttonTimer = 0;              // время после нажатие команды пользователем
const unsigned char timeServo = 5;     // время на открытие/закрытие ворот (сек.)
unsigned char counterParkingPlace = 1; // кол-во парковочных мест
//-------НАСТРОЙКИ---------

//-------ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ---------
unsigned char counterAngle = 0;        // хранение текущей позиции servo
const unsigned char angleMin = 0;      // минимальный угол поворота
const unsigned char angleMax = 180;    // макисмальный угол поворота
float interval = float(timeServo) / float(angleMax) * 1000; // хранит время для преодоления 1 градуса в МС
bool answerSignal1; // отметка пересечения 1 линии
bool answerSignal2; // отметка пересечения 2 линии
//-------ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ---------

// объекты модулей ООП
GyverTM1637 display(PIN_DICP_CLK, PIN_DICP_DIO); // объект дисплея
Servo servo;                                     // объект сервомашинки

void setup() {
  display.clear();       // очистить табло
  display.brightness(7); // яркость, 0 - 7 (минимум - максимум)

  // кнопка с подтягивающим резистором
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // пины питания как выходы
  pinMode(PIN_LED_GREEN, OUTPUT); 
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_SPEAKER, OUTPUT);


  servo.attach(PIN_SERVO); // подключить сервомашинку
  servo.write(0);          // старт сервомашинки с 0 градуса
}

void loop() {
  // отслеживание сигнала от пользователя
  if (userSignal() == true) {
    answerSignal1 = false;        // состояние пересечения линии(въезд)
    answerSignal2 = false;        // состояние пересечения линии(выезд)
    digitalWrite(PIN_LED_RED, 1); // включить красный диод
    
    // проверка условия пересечения линии ИК датчиков на въезд
    if(irSensor(PIN_IR_SEND1, PIN_IR_RECV1) == true) {
      // проверка кол-ва парковочных мест
      if(counterParkingPlace > 0) {
        answerSignal1 = true; // отметить пересечение 1 линии
        startDisplay();       // отобразить кол-во мест
      }
      // если парковочных мест нет
      else { 
        startDisplay();  // отобразить кол-во мест на дисплее
        offIndicators(); // выключить индикаторы, завершить цикл обработки скетча
      }      
    }
    // проверка условия пересечения линии ИК датчиков на выезд
    else {
      if(irSensor(PIN_IR_SEND2, PIN_IR_RECV2) == true) {
        // Serial.print("AnswerSignal2: "); Serial.println(answerSignal2);
        answerSignal2 = true; // отметить пересечение 2 линии
      } 
      else {offIndicators();} // выключить индикаторы
    }
    
    // проверка основных вариантов событий
    // проверка пересечения первой линии
    if((answerSignal1) == true){ 
      alert();                            // звуковой сигнал
      openingGate();                      // открыть ворота
      digitalWrite(PIN_LED_RED, 0);       // выключить красный диод
      digitalWrite(PIN_LED_GREEN, 1);     // включить зеленый диод
      alert();                            // звуковой сигнал
      // проверка перечечения второй линии после первой
      if(irSensorWait(PIN_IR_SEND2, PIN_IR_RECV2) == true) {
        counterParkingPlace--;            // уменьшить кол-во парковочных мест
        startDisplay();                   // обновить дисплей
        digitalWrite(PIN_LED_GREEN, 0);   // выключить зеленый диод
        digitalWrite(PIN_LED_RED, 1);     // включить красный диод
        alert();                          // звуковой сигнал
        closeingGate();                   // закрыть ворота
        offIndicators();                  // выключить индикаторы, завершить цикл обработки
      }
      // проверка ситуации, когда автомобилист всё ещё стоит на пересечении линии
      else{
        if(irSensor(PIN_IR_SEND1, PIN_IR_RECV1) == true) {
          digitalWrite(PIN_LED_GREEN, 0); // выключить зеленый диод
          digitalWrite(PIN_LED_RED, 1);   // включить красный диод
          alert();                        // звуковой сигнал
          closeingGate();                 // закрыть ворота
          offIndicators();                // выключить индикаторы, завершить цикл обработки
        }
        // проверка аварийной ситуации, когда не были пересечены линии после проверки условий(застрял между воротами)
        else{
          anxiety();                      // вызов тревоги
          digitalWrite(PIN_LED_RED, 1);   // включить красный диод
          alert();                        // звуковой сигнал
          closeingGate();                 // закрыть ворота
          offIndicators();                // выключить индикаторы, завершить цикл обработки
        }
      }
    }

    // проверка пересечения второй линии
    if((answerSignal2) == true) {
      alert();                            // звуковой сигнал
      openingGate();                      // открыть ворота
      digitalWrite(PIN_LED_RED, 0);       // выключить красный диод
      digitalWrite(PIN_LED_GREEN, 1);     // включить зеленый диод
      alert();                            // звуковой сигнал
      if(irSensorWait(PIN_IR_SEND1, PIN_IR_RECV1) == true) {
        counterParkingPlace++;            // увеличить кол-во парковочных мест 
        startDisplay();                   // обновить дисплей
        digitalWrite(PIN_LED_GREEN, 0);   // выключить зеленый диод
        digitalWrite(PIN_LED_RED, 1);     // включить красный диод
        alert();                          // звуковой сигнал
        closeingGate();                   // закрыть ворота
        offIndicators();                  // выключить индикаторы, завершить цикл обработки
      }
      else{
          anxiety();                      // вызов тревоги
          digitalWrite(PIN_LED_RED, 1);   // включить красный диод
          alert();                        // звуковой сигналд
          closeingGate();                 // закрыть ворота
          offIndicators();                // выключить индикаторы, завершить цикл обработки
      }
    }
  }
}
// сигнализация
void anxiety() {
  digitalWrite(PIN_LED_GREEN, 0);
  digitalWrite(PIN_LED_RED, 0);
  while (!userSignal()) {
    digitalWrite(PIN_LED_GREEN, 1); delay(166);
    digitalWrite(PIN_LED_GREEN, 0); delay(166);
    digitalWrite(PIN_LED_RED, 1); delay(166);
    digitalWrite(PIN_SPEAKER, 1); delay(166);
    digitalWrite(PIN_SPEAKER, 0); delay(166);
    digitalWrite(PIN_LED_RED, 0); delay(166);     
  }
}

// кратковременный звуковой сигнал
void alert() {
  digitalWrite(PIN_SPEAKER, 1); delay(1000); 
  digitalWrite(PIN_SPEAKER, 0); delay(1000);
}

// открыть ворота
void openingGate(){
  for(; counterAngle <= angleMax-1; counterAngle++) {
    servo.write(counterAngle); delay(interval);
  }
}

// закрыть ворота
void closeingGate(){
  for(; counterAngle >= angleMin+1; counterAngle--) {
    servo.write(counterAngle); delay(interval);
  }
}

// выключить индикаторы
void offIndicators() {
  digitalWrite(PIN_LED_GREEN, 0); delay(1000);
  digitalWrite(PIN_LED_RED, 0); delay(1000);
  display.clear();
}

// отобразить кол-во парковочных мест
void startDisplay() {
    display.clear();
    display.brightness(7); // яркость, 0 - 7 (минимум - максимум) 
    display.twist(3, counterParkingPlace, 30);
}

// сигнал пересечения кратковременного ик сигнала
bool irSensor(unsigned char irSend, unsigned char irRecv) {
  unsigned char myRawData;                          // хранит значение ик сигнала
  IrSender.begin(irSend, true);                     // запуск передачи ИК сигнала
  IrReceiver.begin(irRecv, false);                  // запуск приема ИК сигнала

  // отправление 5 ик сигналов, чтобы исключить возможности помехи
  // Serial.print("Signal wait: "); Serial.println(myRawData);
  for(unsigned char irSignal = 1; irSignal <= 5; irSignal++) {
    IrSender.sendNEC(0x102, 0x24, 1);               // отправление ИК сигнала по адресу - 0x102, команда 0x24(36), кол-во повторений 1
    // декодирование сигнала
    if(IrReceiver.decode()) {
      myRawData = IrReceiver.decodedIRData.command; // сохранение ик сигнала(команда 0x24(36))
      // Serial.print("Signal: "); Serial.println(myRawData);
      IrReceiver.resume();                          // принять след. ик сигнал
    }
    delay(500);                                     // задержка перед отправление ик сигнала(500 МС * 5 = 2500МС)
  }
  IrReceiver.stop();                                // остановить приём ИК сигнала
  // Serial.print("Rezult: "); Serial.println(myRawData);
  // true линия прервана
  // false линия не прервана
  return (myRawData == 36) ? false : true; 
}

// ожидание пересечения ик датчиков вторых линий
bool irSensorWait(unsigned char irSend, unsigned char irRecv) {
  unsigned char myRawData;                          // хранит значение ик сигнала
  IrSender.begin(irSend, true);                     // запуск передачи ИК сигнала
  IrReceiver.begin(irRecv, false);                  // запуск приема ИК сигнала

  // отправление 25 ик сигналов, чтобы исключить возможности помехи
  // Serial.print("Signal wait: "); Serial.println(myRawData);
  for(unsigned char irSignal = 1; irSignal <= 25; irSignal++) {
    IrSender.sendNEC(0x102, 0x24, 1);               // отправление ИК сигнала по адресу - 0x102, команда 0x24(36), кол-во повторений 1
    // декодирование сигнала
    if(IrReceiver.decode()) {
      myRawData = IrReceiver.decodedIRData.command; // сохранение ик сигнала(команда 0x24(36))
      if(myRawData != 36) {return true;}
      // Serial.print("Signal: "); Serial.println(myRawData);
      IrReceiver.resume();                          // принять след. ик сигнал
    }
    delay(333);                                     // задержка перед отправление ик сигнала(333 МС * 24 = 7992 МС)
  }
  IrReceiver.stop();                                // остановить приём ИК сигнала
  // Serial.print("Rezult: "); Serial.println(myRawData);
  // true линия прервана
  // false линия не прервана
  if(myRawData == 36) {return false;}
}

// получение сигнала от водителя
bool userSignal() {
  bool ButtonState = !digitalRead(PIN_BUTTON);      // состояние кнопки
  if(ButtonState && millis() - buttonTimer > 100) {
    buttonTimer = millis();
    return true;
  } 
  else {
    return false;
  }
}