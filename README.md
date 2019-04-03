# Auto Sprinkler
Auto Sprinkler Project, made by Arduino

## Last update: 2018.6.16

# Abstract
Developing a clock system and tuned by 5 buttons - Mode, Switch, Up, Down, and Forcing respectively. When current time goes to setup time, it will trigger relay action for sprinkling water. Sprinkler runs twice at one day. Their actuation start time and sprinkling duration can be set. 16x2 LCD shows the clock and temperature information.

# Mode

- 1: Normal

- 2: Set Current Time

- 3: Set T1 time - T1 means 1st sprinkling time

- 4: Set T2 time - T2 means 2nd sprinkling time

- 5: Sprinkle Time - set sprinkling duration

# Software Environment

- Compiler: Arduino IDE v1.8.5

- Coding IDE: Visual Studio Code v.1.24.0


# Used Software Packages:

- DS3231 : [Library : DS3231](http://www.rinkydinkelectronics.com/library.php?id=73)

- 16x2 LCM : [Arduino 1602 LCD shield with keypad](https://www.hobbyist.co.nz/?q=16x2-arduino-lcd-shield)

# Hardware Pin Define

- LCM : A0(Button), D4, D5, D6, D7, D8, D9, D10

- DS3231 : A5(SCL), A4(SDA)

- Relay : D3(pin 3)

------
# Hardware Snap:

![Arduino UNO R3](https://1.bp.blogspot.com/-MdXBNJGPiUQ/WsG77v79mVI/AAAAAAAAMGM/lg7zbrOWqpg9XzH0eufiBjoX9Io1bYpzwCLcBGAs/s400/21443540638033_923.jpg)

![5V Relay](https://a.rimg.com.tw/s2/4/13/90/21544933817232_452_m.jpg)

![16x2 LCM](https://www.hobbyist.co.nz/sites/default/files/LCDShield.jpg)

![DS3231 RTC IC (I2C)](https://a.rimg.com.tw/s2/d/29/0c/21446709651724_974_m.jpg)


# Revision History

- 20190403 : Based on new relay module, its trigger signal is high enable(the original is low enable). So I modified the function digitalWrite(RELAY_PIN, X) and changed its HIGH/LOW.

