# fMSX-go
fMSX Port to ODROID-GO

to build it, please download fMSX from here: https://fms.komkon.org/fMSX/

But there is also a precompiled Firmware file here: https://github.com/Schuemi/fMSX-go/releases

Please create these directories on your SD Card manually:
```
/roms/msx/bios
/roms/msx/games
/odroid/data/msx
```
and put the BIOS files in /roms/msx/bios. You need at least the files MSX2.ROM, MSX2EXT.ROM and DISK.ROM.

You can have subfolders in /roms/msx/games. If you have much games, subfolders are recommend. You should not have more than 150 Games in one folder.

In /odroid/data/msx you can save keymapping files if you wish. The default keymapping is:
```
[KEYMAPPING]
UP = JST_UP
RIGHT = JST_RIGHT
DOWN = JST_DOWN
LEFT = JST_LEFT
SELECT = 1
START = 2
A = JST_FIREA
B = JST_FIREB
```
The possible Key Mappings are:
JST_UP, JST_RIGHT, JST_DOWN, JST_LEFT, JST_FIREA, JST_FIREB, KBD_SPACE, KBD_F1, KBD_F2, KBD_F3, KBD_F3, KBD_F4, KBD_F5, KBD_LEFT, KBD_UP, KBD_DOWN, KBD_SHIFT, KBD_CONTROL, KBD_GRAPH, KBD_BS, KBD_TAB, KBD_CAPSLOCK, KBD_SELECT, KBD_HOME, KBD_ENTER, KBD_INSERT, KBD_COUNTRY, KBD_STOP, KBD_NUMPAD0 - KBD_NUMPAD9, KBD_ESCAPE and a single digit or a single letter.

You can use for every game a custom mapping file. Put the file in /odroid/data/msx. It schould have the name [GAME].ini. For example if your gamefiles name is GTA3.rom the keyfilename has to be GTA3.ini.

Also you can have a default setting. Go into /odroid/data/msx, if you have ever started fMSX there should be a file called "config.ini". In this file you can add your defualt keymapping the same way you would add a key mapping for a game.



Gamesaves are saved in the same directory as the game itself. It is calles [GAME].sav and is compatible to every other fMSX port. So you can continue playing on pc, playstation etc...

# Virtual Keyboard:

You can open a virtual keyboard by pressing an hold the "A" button ans then the "Menu" button. On the virtual keyboard you can use the "A" or the "B" button to press a key. If you are holding a button, this button will also be holded in the emulator. So if you go to the "shift" key, press and hold "A" and then while pressing "A" go to the "1" key and press "B"  you will write a "!". Because on the real MSX if you would press shift + 1 you will also write a "!"



# Multiplayer

Multiplayer is really fun. I tested It a hole night with some friends and a couple of beers ;)

To use multiplayer you need to have exactly the same BIOS files on both devices and the same game file in the same directory. The best way is to simply copy the SD from one device.

One is the server, the other is the client. The server starts a game with "start multiplayer server" in the menu, the other one chooses "multiplayer client". The server selects a ROM or a floppy disk. Both devices will restart and they run now the same game.

The "joystick" of the server runs in port 1, the client has port 2.

In multiplayer mode there are a few limitations:

You cannot enter the menu. To start another game or not to play in pairs, please turn off the devices.
Only the server can call the virtual keyboard.
Only the server has sound
Many games work very well, some crack the sound, some run too slowly. The problem is to run both games in exactly the same state. I've tried to get the best out of the hardware, maybe I'll find ways to optimize it, but I think it's going very well already. And it's a lot of fun.

You can't have two Multiplayer games with 4 devises at the same place yet. They're gonna bother each other, because there are no "Multiplayer rooms" yet.

When it cracks in a game: just turn off the sound.

How does this work?

- The server starts an access point with a hidden Siid
- The client searches for this access point, if he finds it he will connect
- The server tells the client what game they whant to play
- after starting the game on both devices they send UDP packts with joysick and keyboard data to each other. Both devices have to know what the other is doing in every vblank.



# Next:

The next things I'm planning are:

- Bugfix, bugfix, bugfix. No new functions the nex releases.



