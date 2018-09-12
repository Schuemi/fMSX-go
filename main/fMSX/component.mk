COMPONENT_PRIV_INCLUDEDIRS := ../Z80 ../EMULib ../odroidGo
CPPFLAGS := -DLSB_FIRST -DESP32 -Dopendir=_opendir -Dreaddir=_readdir -Dstat=_stat -Dfopen=_fopen -Drewinddir=_rewinddir  -Dfclose=_fclose
CFLAGS := -Ofast -mlongcalls -Wno-error
