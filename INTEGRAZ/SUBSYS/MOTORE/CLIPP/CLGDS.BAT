@echo off
echo **********************************************************
echo compilazione per le procedure CLIPPER di caricamento dati
echo "Precaricati MOTORE"
echo **********************************************************
echo il parametro "RUN" esegue automaticamente le procedure
SET ETABETA=E:\ETABETA\
SET M300=E:\M300\
SET CVB_UFF=E:\CVB_UFF\
SET TMP=E:\TMP\
SET INTERMED=E:\MOTORE1\INTERMED\
SET OUT=E:\DATIMOT\
SET CLIPDIR=E:\CLIPPER5\BIN\
SET LIB=E:\CLIPPER5\LIB\
SET CLIPPER=//F:30
SET ITG_DRIVE=E:
SET TMP=E:\TMP\
set DBG1=
set DBG2=

@echo on
@if %2. == RUNONLY. goto RUN
@if %2. == runonly. goto RUN
@if %3. == B. set DBG1=/B
@if %3. == B. set DBG2=CLD
@if %3. == b. set DBG1=/B
@if %3. == b. set DBG2=CLD
%CLIPDIR%CLIPPER %ITG_DRIVE%\integraz\subsys\motore\prepdati\%1 /o%TMP%%1.obc %DBG1%
@IF ERRORLEVEL 1 GOTO FINE
%CLIPDIR%rtlink FILE %TMP%%1.obc OUTPUT %TMP%%1.exe LIB CLIPPER LIBRARY extend
@IF ERRORLEVEL 1 GOTO FINE
del %TMP%%1.obc
@if %2. == RUN. goto run
@if %2. == run. goto run
@goto fine
:run
%DBG2% %TMP%%1 %3 %4 %5 %6 %7 %8
del %TMP%%1.exe
:fine
