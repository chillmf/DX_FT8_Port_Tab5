# DX_FT8_Port_Tab5

<img width="1205" height="792" alt="image" src="https://github.com/user-attachments/assets/845a5e70-2de3-434a-b0d6-a40ec0e9d4c3" />


## This is a port of DX_FT8 From STM32F Board to Tab5 Board.
The original work was done several
years ago with a great team of collaborators.
Here is a link to the previous work:
https://github.com/chillmf/DX-FT8-Transceiver-Source-Code_V2

Recently a really nice board has become popular, The Stack5 TAB5.
This board uses an ESP32-P4 processor and it has a great display.
Further, it includes a pluggable battery so that the only connection
you need to make is the antenna. This is a true handheld FT8 Rig.

## This will make carrying along on a airline flight easier.

As you can see from the photo of the back side, this rig uses a 
DX_Uno designed by Barb, WB2CBA for a quick and easy integration 
of a FT8 transceiver with the TAB5 Board. 

<img width="584" height="456" alt="image" src="https://github.com/user-attachments/assets/7e12ab5a-351e-4c9f-8d9a-291c5406805e" />

Measurement shows that the power output using Barb's DX_Un0 Board is right at 400 mWatt
and the Tab5 Built In Battery supports continuous operation for 4 Hours.

## Hopefully, this software project will spawn a new generation of FT8 Transceiver.

The rig was assembled by grafting a Teensy Audio board onto a DX_Un0 board. Please see the connection drawing in the Connection
Folder for the required connection between a Teensy Audio Adapter Board Rev. D. here is a link to the Teensy Board Details:
https://www.pjrc.com/store/teensy3_audio.html

## Getting Started
To get started you will need to create and store your station data in a file located on an SD Card. Here is an example of the contents
of the file.

<img width="290" height="178" alt="image" src="https://github.com/user-attachments/assets/d8d04b5e-d5ac-4944-97c1-180e84b4e731" />

You will need to enter your station details in order to make FT8 contacts. Create the file using a basic text editor and save it with
the file name of "StationData.ini.

The Section Marked [Wifi] is required for setting the Tab5 Board Real Time Clock, RTC via an Internet Connection to a Network Time Protocol Server.
You will certainly will want to set the RTC when you first commission your Tab5 with this application. However, once you set the RTC 
you may remove this section until you want to reset the RTC.

If you remove the RTC Section the application will run without bothering to reset the RTC on opening the application. Or, if you leave
the RTC Section in the StationData.ini file, the RTC will be updated during opening of the application if the referenced Wifi connection
is available. It's your choice.




