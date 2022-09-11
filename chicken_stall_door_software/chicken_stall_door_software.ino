/*
   Hasenklappe
   Author: dustinbrun
           licensed under CC BY 4.0
   Version 09.2022

   Source 433Mhz Receiver: https://funduino.de/nr-03-433mhz-funkverbindung


   ------ Serial Communication, Baud: 9600 ------
   - Request State of the Door: 'S'
      - Returns: 1 - Door is open, 0 - Door is closed, 2 - Unknown Position
   - Open Door: 'U'
      - Returns: 'U'
      - If it fails or not possible (already open): 'E'
   - Open Door: 'D'
      - Returns: 'D'
      - If it fails or not possible (already closed): 'E'
   - Stop Movement: 'H'
      - Returns: 'H'

*/


#include "RCSwitch.h" // https://github.com/sui77/rc-switch

//Variables
int motor_timeout = 20000; //Motor stops after this limit (in ms) is reached, Maximum time for the Opening-/Closing- Process
int solenoid_on_time = 2000; //Time for the solenoid to be switched on (in ms) after the opening process was started. The door should exceed the solenoid position within this interval

//Remote Codes
long int remote_code_up = 5592512;  //Received Remote Codes, to which the Arduino reacts to, change to the Values of your remote. You can read those code using the "ReceiveDemo_Simple.ino"-code from the rc-switch Library
long int remote_code_down = 5592368;
long int remote_code_stop = 5592332;


// Pins
const int button_down = 12;
const int button_up = 11;
const int button_stop = 10;
const int led_error = 9;
const int led_motor = 8;
const int led_power = 7;
const int motor_a = 6;
const int motor_b = 5;
const int limit_top = A1;
const int limit_bottom = A2;
const int remote_interrupt_pin = 0; // Interrupt-Pin 0 = Arduino Pin D2
const int lock_pin = 4;


unsigned long motor_start_time = 0;
bool go_up = false;
bool go_down = false;
bool stop = false;

RCSwitch remote = RCSwitch();

void setup()
{
  Serial.begin(9600);
  remote.enableReceive(remote_interrupt_pin);

  pinMode(button_down, INPUT_PULLUP); //Pressed = LOW
  pinMode(button_up, INPUT_PULLUP);
  pinMode(button_stop, INPUT_PULLUP);
  pinMode(led_error, OUTPUT);
  pinMode(led_motor, OUTPUT);
  pinMode(led_power, OUTPUT);
  pinMode(motor_a, OUTPUT);
  pinMode(motor_b, OUTPUT);
  pinMode(lock_pin, OUTPUT);
  pinMode(limit_top, INPUT_PULLUP);   //Triggered = LOW
  pinMode(limit_bottom, INPUT_PULLUP);

  digitalWrite(motor_a, LOW);
  digitalWrite(motor_b, LOW);
  digitalWrite(lock_pin, LOW);
  digitalWrite(led_error, HIGH);
  digitalWrite(led_motor, HIGH);
  digitalWrite(led_power, HIGH);

  delay(200);
  digitalWrite(led_error, LOW);
  digitalWrite(led_motor, LOW);

  //Serial.print("A");
}

void loop()
{
  if ((digitalRead(button_down) == LOW || go_down) && digitalRead(limit_bottom) != LOW)
  {
    delay(100);
    stop = false;
    motor_start_time = millis();
    motor_down();

    while (digitalRead(limit_bottom) != LOW &&
           millis() - motor_start_time < motor_timeout &&
           stop != true)
    {
      check_messages();
      delay(10);
    }

    motor_stop();

    if ((millis() - motor_start_time >= motor_timeout) || stop == true)
    {
      digitalWrite(led_error, HIGH);
      Serial.print("E");
      delay(500);
      digitalWrite(led_error, LOW);
    }
    else
    {
      digitalWrite(led_error, LOW);
      Serial.print(get_pos());
    }

    go_down = false;
    stop = false;
    delay(200);
  }
  else if ((digitalRead(button_down) == LOW || go_down) && digitalRead(limit_bottom) == LOW)
  {
    digitalWrite(led_error, HIGH);
    Serial.print("E");
    delay(500);
    digitalWrite(led_error, LOW);
    go_down = false;

  }


  if ((digitalRead(button_up) == LOW || go_up) && digitalRead(limit_top) != LOW)
  {
    delay(100);
    stop = false;
    door_unlock();
    motor_start_time = millis();
    motor_up();

    while (digitalRead(limit_top) != LOW &&
           millis() - motor_start_time < motor_timeout &&
           stop != true)
    {
      check_messages();
      delay(10);
      if(millis() - motor_start_time > solenoid_on_time)
      {
          door_lock();
       }
    }

    motor_stop();
    door_lock();

    if ((millis() - motor_start_time >= motor_timeout) || stop == true)
    {
      digitalWrite(led_error, HIGH);
      Serial.print("E");
      delay(500);
      digitalWrite(led_error, LOW);
    }
    else
    {
      digitalWrite(led_error, LOW);
      Serial.print(get_pos());
    }

    go_up = false;
    stop = false;
    delay(200);
  }
  else if ((digitalRead(button_up) == LOW || go_up) && digitalRead(limit_top) == LOW)
  {
    digitalWrite(led_error, HIGH);
    Serial.print("E");
    delay(500);
    digitalWrite(led_error, LOW);
    go_up = false;

  }

  check_messages();
  delay(10);

}

void check_messages()
{
  if (Serial.available()) {
    char serial_message = Serial.read();
    led_power_blink();

    if (serial_message == 'S') // Get State
    {
      Serial.print(get_pos()); // 1 - Door is open, 0 - Door is closed, 2 - Unknown Position
    }
    else if (serial_message == 'U')
    {
      go_up = true;
      Serial.print("U");
    }
    else if (serial_message == 'D')
    {
      go_down = true;
      Serial.print("D");
    }
    else if (serial_message == 'H')
    {
      stop = true;
      Serial.print("H");
    }
  }

  if (remote.available())
  {
    long int remote_message = remote.getReceivedValue();
    led_power_blink();

    if (remote_message == remote_code_up)
    {
      go_up = true;
    }
    else if (remote_message == remote_code_down)
    {
      go_down = true;
    }
    else if (remote_message == remote_code_stop)
    {
      stop = true;
    }
    /*else // Unknown Code
      {
      digitalWrite(led_error, HIGH);
      Serial.print("E");
      delay(100);
      digitalWrite(led_error, LOW);
      }*/

    remote.resetAvailable();
  }

  if (digitalRead(button_stop) == LOW)
  {
    stop = true;
  }

}


int get_pos()
{
  if (digitalRead(limit_top) == LOW) return 1;
  else if (digitalRead(limit_bottom) == LOW) return 0;
  else return 2;
}


void motor_down()
{
  digitalWrite(led_motor, HIGH);
  digitalWrite(motor_a, LOW);    //Motor going down
  digitalWrite(motor_b, HIGH);
}
void motor_up()
{
  digitalWrite(led_motor, HIGH);
  digitalWrite(motor_a, HIGH);    //Motor going up
  digitalWrite(motor_b, LOW);
}
void motor_stop()
{
  digitalWrite(led_motor, LOW);
  digitalWrite(motor_a, LOW);    //Motor stop
  digitalWrite(motor_b, LOW);
}

void door_lock()  //Normally closed Solenoid Lock
{
  digitalWrite(lock_pin, LOW);
}
void door_unlock()
{
  digitalWrite(led_motor, HIGH);
  digitalWrite(lock_pin, HIGH);
  delay(200);
  digitalWrite(led_motor, LOW);
  delay(200);
}

void led_power_blink()
{
  digitalWrite(led_power, LOW);
  delay(50);
  digitalWrite(led_power, HIGH);
}
