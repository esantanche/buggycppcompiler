// ================================================================
// File di definizione dei thread utilizzati dall' applicazione.
// ================================================================
//
#ifndef HO_SIPAXTHR_HPP
#define HO_SIPAXTHR_HPP

// Threads standard del SIPAX
#define THD_PM           1  // Regular   PM
#define THD_VIEW         2  // Regular   VIEW
#define THD_MODEL        3  // Regular   MODEL
#define THD_BACKGROUND   4  // High      BACKGROUND (Fase di bigliettazione e contabilizzazione)
#define THD_BACKIDLE     5  // Idle      BACKGROUND (low Priority, idle time only)
#define THD_QUALITY      6  // Idle      (componenti e macchina)
#define THD_MVS          7  // Regular   Collegamento MVS
#define THD_TPF          8  // Regular   Collegamento TPF
#define THD_LOOP_CNTRL   9  // VeryHigh  Controllo threads in LOOP
#define STD_THREADS  9  // Numero di threads che le applicazione debbono allocare

// I sottosistemi utilizzano queste definizioni o direttamente o in :

// Impianto    : SIPAX.H
//    
// Contabilita': CVIEWDEF.H   (o CV_THRD.H)
//    
// Qualita'    : QM_MACH.H QM_ALL.HPP QM_ALLEX.HPP QM_NOTES.HPP QM_TIPI.H 
//               Per ora modificati solo: QM_MACH.H QM_TIPI.H

#endif
