# smartfittingroom
By using motion sensor, ultrasonic range sensor, and a RFID reader, it is possible to create a simple prototype for smart fitting room.

Hardware used:
- Grove Motion Sensor
- Grove Ultrasonic range sensor
- RFID reader (and tags)
- Mediatek LinkedIt One board
- Base shield
- LED
- Backlight LCD

Steps to connect the components together:
1. Setup the linkedIt one board with a base shield
2. Connect the motion sensor at D7, Ultrasonic sensor at D3, LED at D8, RFID reader at UART, and Backlight LCD at I2C
[Note: The grove motion sensor is very sensitive, hence, a potentiometer might be required.]
3. Open the all the codes attached above in Arduino, and push in the codes to the linkedit one board.

Setup image:
https://drive.google.com/file/d/0B_YKwVmw1nubLXc1ZzRjM1lzQnM/view?usp=sharing



