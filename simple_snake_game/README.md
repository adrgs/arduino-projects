## Simple Snake Game	

Outputs using **Adafruit-ST7735-Library** on a 1.44 128x128 TFT_LCD Screen.

The snake is controlled using an IR remote, file needs **Arduino-IRremote-dev**.

Connections:

I'll prefix Arduino with A-, Screen with S- and Infrared Receiver with R-

```
A-13   ->  S->SCL
A-11   ->  S->SDA
A-10   ->  S->CS
A-9    ->  S->DC
A-8    ->  S->RES
A-7    ->  R->Y
A->5V  ->  S->VCC, R->R
A->GND ->  S->GND, R->GND
```

Photo of the setup:

![setup_photo](setup_photo.png)