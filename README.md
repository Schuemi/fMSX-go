# fMSX-go
fMSX Port to ODROID-GO

to build it, please download fMSX from here: https://fms.komkon.org/fMSX/

But there is also a precompiled Firmware file here: https://github.com/Schuemi/fMSX-go/tags

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
You can have for every game a custom mapping file. Put the file in /odroid/data/msx. It schould have the name [GAME].ini. For example if your gamefiles name is GTA3.rom the keyfilename has to be GTA3.ini.

Gamesaves are saved in the same directory as the game itself. It is calles [GAME].sav and is compatible to every other fMSX port. So you can continue playing on pc, playstation etc...

