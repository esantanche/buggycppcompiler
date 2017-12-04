@echo off
setlocal
if not exist PROVAMI.EXE goto nonhopgm
if %1. == . goto explain
set EXT=%2
if %2. == . set EXT=OUT
if not exist %1.IN goto nonce
echo invoco PROVAMI in modalita' batch prendendo l' input da %1.IN
echo i risultati saranno scritti su %1.%EXT%
SET TRACELEVEL=0
SET OK_BEEP=SI
@echo on
PROVAMI <%1.IN >%1.%EXT%
@echo off
goto fine
:explain
echo *******************************************
echo               TEST MOTORE
echo *******************************************
echo Utilizzo:
echo TEST TipoTest Estensione
echo Esegue i casi di prova riportati in TipoTest.IN
echo i risultati sono scritti su TipoTest.Estensione
echo se Estensione non fornito default = OUT
echo buon lavoro
goto fine
:nonce
echo *******************************************
echo                   ERRORE
echo *******************************************
echo Non esiste il file di direttive per il motore '%TMP%.IN'
echo Impossibile attivare il programma di prova
goto fine
:nonhopgm
echo *******************************************
echo                   ERRORE
echo *******************************************
echo Non esiste il file di programma PROVAMI.EXE
echo Probabilmente il file e' stato lanciato da
echo una directory errata.
echo Impossibile attivare il programma di prova
goto fine
:fine
