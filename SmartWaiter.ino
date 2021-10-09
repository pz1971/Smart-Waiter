#include <Servo.h>
#include <LiquidCrystal.h>
#include <Keypad.h>

const int one_sec = 1000 ;

char keys[4][4] = {
  {'7', '8', '9', 'A'},
  {'4', '5', '6', 'B'},
  {'1', '2', '3', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[4] = {35, 36, 37, 38}; //connect to the row pinouts of the keypad
byte colPins[4] = {34, 33, 32, 31}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, 4, 4);

Servo wheel_rotator1, wheel_rotator2, body_rotator, container ;
int wheel_1_fwd = 51, wheel_1_rev = 50, wheel_2_fwd = 49, wheel_2_rev = 48 ;
int buzzer = 45 ;

//LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
LiquidCrystal lcd(44, 43, 42, 41, 40, 39); // 20x4

int IRSensor = 30 ;
int lm35 = A0;

int position[][2] = {
  // {up, right} // unit per second
  {0,0},    // kitchen
  {5, 2},   // table1
  {5, -2},  // table2
  {10, 2},  // table3
  {10, -2}, // table4
  {15, 2},  // table5
  {15, -2}  // table6
};

int pos[] = {0, 0};
String pin ;
int table ;
const int kitchen = 0 ;
int desired_temp ;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  // servo motors
  wheel_rotator1.attach(53);
  wheel_rotator1.write(0);
  wheel_rotator2.attach(52);
  wheel_rotator2.write(0);
  body_rotator.attach(47);
  body_rotator.write(0);
  container.attach(46);
  container.write(0);
  
  // DC motors
  pinMode(wheel_1_fwd, OUTPUT);
  pinMode(wheel_1_rev, OUTPUT);
  pinMode(wheel_2_fwd, OUTPUT);
  pinMode(wheel_2_rev, OUTPUT);
  // buzzer
  pinMode(buzzer, OUTPUT);
  // LCD
  lcd.begin(20, 4);
  // Sensors
  pinMode(IRSensor, INPUT);
}

void loop() {
  start();
  in_front_of_kitchen();
  go(kitchen, table) ;
  in_front_of_table();
  go(table, kitchen);
}

void start()
{
  pos[0] = pos[1] = 0 ;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Press ON to start");

  while(keypad.getKey() != '*') ;
  delay(one_sec / 2);
}

void in_front_of_kitchen()
{
  open_container();
  while(true)
  {
    get_temp_input();
    float cur_temp = get_temp_cel();
    if(cur_temp >= desired_temp - 2.0 && cur_temp <= desired_temp + 2.0)
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Perfect temp!!");
      break ;
    }
    else
    {
      notify(1000);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Inappropriate temp!!");
    }
    delay(one_sec / 2);
  }
  close_container();
  pin_input(true);
  table = table_no_input();
}

void open_container()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Press A to open the");
  lcd.setCursor(0,1);
  lcd.print("container.");

  while(keypad.getKey()!= 'A')
    continue;

  container.write(120);
  delay(one_sec);
}

void close_container()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("container will close");
  lcd.setCursor(0,1);
  lcd.print("in - ");
  lcd.setCursor(0,2);
  lcd.print("Press C to reset");

  char ch ;
  for(int i = 10 ; i > 0 ; i--)
  {
    lcd.setCursor(5,1);
    lcd.print(i);
    lcd.print(" ");
    
    ch = keypad.getKey();
    if(ch && ch == 'C')
      i = 11;
    delay(one_sec / 5);
  }
  container.write(0);
  delay(one_sec);
}

void pin_input(bool in_kitchen)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("input 4 digit pin:");
  lcd.setCursor(0,1);
  lcd.print("<Press C to clear>");
  lcd.setCursor(0,2);
  lcd.print("<Press B to set>");
  
  char ch ;
  pin = "----";
  for(int i = 0 ;  ; i = min(4, i + 1))
  {
    while(true)
    {
      ch = keypad.getKey();
      if(ch == 'A' || ch == 'D' || ch == '*' || ch == '#')
        continue ;
      if(ch)
        break ;
    }
    if(ch == 'C')
    {
      i = -1 ;
      pin = "----";
      lcd.setCursor(0,3);
      lcd.print(pin);
      continue;
    }
    if(ch == 'B')
    {
      if(i == 4)
        break;
      else
      {
        i-- ;
        continue;
      }
    }
    if(i <= 3)
    {
      pin[i] = ch ;
      lcd.setCursor(0,3);
      lcd.print(pin);
    }
  }
  if(in_kitchen)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Password Set");
    lcd.setCursor(0,1);
    lcd.print("Successfully.");
    lcd.setCursor(0,2);
    lcd.print(pin);
  }
  delay(one_sec);
}

int table_no_input()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Input Table No.");
  lcd.setCursor(0,1);
  lcd.print("<1, 6>");
  lcd.setCursor(0,2);

  char ch = 0 ;
  while(true)
  {
    ch = keypad.getKey();
    if(ch && ch >= '1' && ch <= '6')
      break ;
  }
  lcd.print(ch);
  delay(one_sec);
  return ch - '0' ;
}

void go(int from, int to)
{
  if(from == kitchen)
  {
    body_rotator.write(180);
    wheel_rotator1.write(180);
    wheel_rotator2.write(180);
    delay(one_sec);
    move(position[to][0] - position[from][0], 0, 1);
    
    if(to % 2 == 0)
    {
      body_rotator.write(270);
      wheel_rotator1.write(270);
      wheel_rotator2.write(270);
      delay(one_sec);
      move(position[to][1] - position[from][1], 1, -1);
    }
    else
    {
      body_rotator.write(90);
      wheel_rotator1.write(90);
      wheel_rotator2.write(90);
      delay(one_sec);
      move(position[to][1] - position[from][1], 1, 1);
    }
  }
  else // to == kitchen
  {
    if(from % 2 == 0)
    {
      body_rotator.write(90);
      wheel_rotator1.write(90);
      wheel_rotator2.write(90);
      delay(one_sec);
      move(position[to][1] - position[from][1], 1, 1);
    }
    else
    {
      body_rotator.write(270);
      wheel_rotator1.write(270);
      wheel_rotator2.write(270);
      delay(one_sec);
      move(position[to][1] - position[from][1], 1, -1);
    }
    
    body_rotator.write(0);
    wheel_rotator1.write(0);
    wheel_rotator2.write(0);
    delay(one_sec);
    move(position[to][0] - position[from][0], 0, -1);
  }
}

void move(int len, const int ii, const int jj)
{
  len = abs(len);
  digitalWrite(wheel_1_rev, LOW);
  digitalWrite(wheel_2_rev, LOW);
  digitalWrite(wheel_1_fwd, LOW);
  digitalWrite(wheel_2_fwd, LOW);

  print_pos();
      
  for(int i = 0 ; i < len ; i++)
  {
    if(digitalRead(IRSensor) == 1) // obstacle found
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Hello There!");
      lcd.setCursor(0,1);
      lcd.print("You Are on my Way.");
      notify(500);
      
      print_pos();
      i-- ;
      delay(one_sec);
      continue ;
    }
    else // no obstacle
    {
      lcd.setCursor(0,0);
      lcd.print("                    ");
      lcd.setCursor(0,1);
      lcd.print("                    ");
      
      digitalWrite(wheel_1_fwd, HIGH);
      digitalWrite(wheel_2_fwd, HIGH);
      delay(one_sec);
      digitalWrite(wheel_1_fwd, LOW);
      digitalWrite(wheel_2_fwd, LOW);
      pos[ii] += jj ;
      print_pos();
    }
  }
  print_pos();
}

void print_pos()
{
      lcd.setCursor(0,3);
      lcd.print("                    ");
      lcd.setCursor(0,3);
      lcd.print("Pos: (");
      lcd.print(pos[0]);
      lcd.print(",");
      lcd.print(pos[1]);
      lcd.print(")");
}

void notify(const int freq)
{
  tone(buzzer, freq);
  delay(one_sec);
  noTone(buzzer);
  delay(one_sec / 2);
}

void in_front_of_table()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Your food arrived");
  notify(one_sec);
  delay(one_sec);
  
  String pin2 = pin ;
  while(true)
  {
    pin_input(false);
    if(pin == pin2)
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Proceed...");
      open_container();
      break;
    }
    else
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("incorrect password");
      lcd.setCursor(0,1);
      lcd.print("Please try again.");
      delay(one_sec);
    }
  }
  delay(one_sec);
  close_container();
}

float get_temp_cel()
{
  int value=analogRead(lm35); //Reading the value from sensor
  float tempc=(value*500.0)/1023;
  return tempc ;
}

void get_temp_input()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Input desired Temp:");
  lcd.setCursor(0,1);
  lcd.print("+-2,C->Clear,B->Done");
  lcd.setCursor(0,2);
  desired_temp = 0 ;
  while(true)
  {
    lcd.setCursor(0,3);
    lcd.print("Cur temp:");
    lcd.print(get_temp_cel());
    
    char ch = keypad.getKey();
    if(!ch)
      continue ;
    if(ch >= '0' && ch <= '9')
      desired_temp = desired_temp * 10 + (ch - '0') ;
    else if(ch == 'C')
      desired_temp = 0 ;
    else if(ch == 'B')
      break ;
    lcd.setCursor(0,2);
    lcd.print(desired_temp);
    lcd.print("                    ");
  }
  
  delay(one_sec);
}
