@rem Questo batch permette di compilare per Windows
@set pgm=
@set TRACELEVEL=
@if exist %1.c   set pgm=%1.c
@if exist %1.cpp set pgm=%1.cpp
@del c:\tmp\caric\%1.*
bcc32 +cwin.cfg %pgm% MOTORE.LIB BASELIB.LIB MOTORE2.LIB >c:\tmp\caric\%1.err
@copy c:\tmp\caric\%1.exe
@set pgm=
@set TRACELEVEL=3
