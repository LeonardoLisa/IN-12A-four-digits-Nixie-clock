
<div align="center"><img src="\Hardware\Pictures\IN-12A_Clock.jpg" alt="IN-12A four digits nixie clock" width="600"/></div>

<h4 align="center">Be sure to :star: my repo so you can keep up to date on any progress!</h4>

## About
Everyone loves Nixie clocks, but most of them are pretty ugly. This project aims to build the most elegant open source IN-12A nixie clock on the web. With an Art Deco aesthetic and a steampunk flare, it is certainly a memorable and eye-pleasing clock. Our second goal is to keep it affordable, so we modelled 3d printed parts for the case. In this repository you will find all of the production files and instructions to costruct your own clock.
The design is broken into three units:

- Front and back panel
- 3D printed case
- Main circuit board

At the moment the main circuit board does not include: a high voltage DC-DC converter, an arduino uno, ds3231 module and the 3 push switch. You will need to wire those components by hand and position them inside of the case. In further revisions all of these features will be integrated in the main board.

## Feature overview
- Gold plated front panel with a reversible design
- Real time clock (RTC) Time keeper
- Temperature and humidity sensor
- "Slot machine" digit cycling every hour to prevent cathode poisoning
- Programmable display shutdown during night time
- Programmable RGB led, solid color and rainbow effects are available in the clock menu

## License
This repository uses a complex multilicense system 
- All shared materials in the Hardware directory are shared under the [Creative Commons BY-NC-SA 4](https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en) license.  
- All software in the Software directory is shared under [GPLv3](https://www.gnu.org/licenses/quick-guide-gplv3.html) the license.

## Installation
### Hardware
1. First of all you need to purchase or make all of the following components:
    - 3d print all of the files present under the `\Hardware\Case` directory and buy the components listed in the BOM-mechanical-assembly.xlsx 
    - Contact your favorite pcb manifacturer and order the front and back panels under the `\Hardware\Front-Back_panel` directory, as well as the main board circuit under the `\Hardware\Main_board` directory. 
    - Buy the main board components listed in the BOM-main-board.xlsx as well the BOM-miscellaneous-components.xlsx
    - Find a set of four old stock IN-12A (ИН-12A) nixie tubes, and two INS-1 (ИНC-1) nixie dots.

2. Assemble the main board following the schematic. For soldering the smd components you may need an hot air gun but, excluding the hs3001, all smd components can be soldered using only a soldering iron. 
    <div align="center"><img src="\Hardware\Pictures\Assembled_main_board.jpg" alt="Main board populated with components" width="600"/></div>
When soldering the INS-1 tubes fold the lead as shown in the picture.
    <div align="center"><img src="\Hardware\Pictures\INS-1_soldering.jpg" alt="INS-1 tube assembly detail" width="600"/></div>

3. Wire up all the main board to the Arduino UNO following the diagram
    | Arduino UNO pin | Main board pin |
    | --- | --- |
    | GND | GND |
    | 4 | CLK |
    | 5 | DIN |
    | 8 | LE |
    | 13 | LED_DIN |
    | SLC | SLC (optional) |
    | SDA | SDA (optional) |

    Note: connect SDA and SLC only if you intend to use the HS3001 sensor.

4. Wire the remaining components.
    - Connect the switch buttons: plus menu push switch to Arduino pin A0, the minus menu push switch to pin A2 and menu push switch to pin A1.
    - Connect DS3231 rtc module: connect 5V, GND, SLC and SDA DS3231 pins to the corresponding one of the Arduino.
    - Connect the high voltage output to the main board and the GND to the common ground.

5. Power supply reccomandations.
    - High voltage DC-DC converter requirements: 150-165V 7mA.
    - Low voltge power supply requirements: 5V 300mA.

### Firmware
The firmware can be directly flashed on the Arduino using the Arduino IDE or with an external programmer. A demo version for testing the hardware or familiarizing with the clock display functionalities is also available.