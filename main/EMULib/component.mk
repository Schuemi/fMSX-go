COMPONENT_PRIV_INCLUDEDIRS := ../odroidGo
CPPFLAGS := -DSND_CHANNELS=16 -DBPS16 -DLSB_FIRST -DESP32 -Dopendir=_opendir -Dreaddir=_readdir -Dstat=_stat -Dfopen=_fopen  -Drewinddir=_rewinddir  -Dfclose=_fclose
CFLAGS :=  -Ofast -mlongcalls -Wno-error

