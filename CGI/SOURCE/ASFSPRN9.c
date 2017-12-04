#include <stdlib.h>
#include <string.h>
#include "asfsprn9.h"

POSTI TipiPostiDisponibili(int n_servizi, int *servizi)
{
  POSTI p;
  int prenot_fac, prenot_obb, prenot_s1a;
  int i;

  p.posti_1a = 0;
  p.posti_2a = 0;
  p.cuccette_1a = 0;
  p.cuccette_2a = 0;
  p.letti_1a = 0;
  p.letti_2a = 0;
  p.ristorante = 0;
  p.fumatori = 0;

  prenot_fac = 0;
  prenot_obb = 0;
  prenot_s1a  = 0;

  for (i=0; i<n_servizi; i++) {
    if      (servizi[i] == PRENOT_FAC) prenot_fac = 1;
    else if (servizi[i] == PRENOT_OBB) prenot_obb = 1;
    else if (servizi[i] == PRENOT_S1A) prenot_s1a  = 1;
  }
  p.prenotabilita = 0;
  if      (prenot_fac == 1 && prenot_obb == 0 && prenot_s1a == 0) p.prenotabilita = 1;
  else if (                   prenot_obb == 1 && prenot_s1a == 0) p.prenotabilita = 2;
  else if (                   prenot_obb == 0 && prenot_s1a == 1) p.prenotabilita = 3;
  else if (                   prenot_obb == 1 && prenot_s1a == 1) p.prenotabilita = 4;

  for (i=0; i<n_servizi; i++) {
    if (servizi[i] == POSTI_1A) {
      if      (prenot_obb == 1)                    p.posti_1a = 3;
      else if (prenot_fac == 1 || prenot_s1a == 1) p.posti_1a = 2;
      else                                         p.posti_1a = 1;
    }
    else if (servizi[i] == POSTI_2A) {
      if      (prenot_obb == 1 && prenot_s1a == 0) p.posti_2a = 3;
      else if (prenot_fac == 1 && prenot_s1a == 0) p.posti_2a = 2;
      else                                         p.posti_2a = 1;
    }
    else if (servizi[i] == CUCCETTE_1A) {
      if      (prenot_obb == 1)                    p.cuccette_1a = 3;
      else if (prenot_fac == 1 || prenot_s1a == 1) p.cuccette_1a = 2;
      else                                         p.cuccette_1a = 1;
    }
    else if (servizi[i] == CUCCETTE_2A) {
      if      (prenot_obb == 1 && prenot_s1a == 0) p.cuccette_2a = 3;
      else if (prenot_fac == 1 && prenot_s1a == 0) p.cuccette_2a = 2;
      else                                         p.cuccette_2a = 1;
    }
    else if (servizi[i] == LETTI_1A) {
      if      (prenot_obb == 1)                    p.letti_1a = 3;
      else if (prenot_fac == 1 || prenot_s1a == 1) p.letti_1a = 2;
      else                                         p.letti_1a = 1;
    }
    else if (servizi[i] == LETTI_2A) {
      if      (prenot_obb == 1 && prenot_s1a == 0) p.letti_2a = 3;
      else if (prenot_fac == 1 && prenot_s1a == 0) p.letti_2a = 2;
      else                                         p.letti_2a = 1;
    }
    else if (servizi[i] == RISTORANTE) p.ristorante = 1;
    else if (servizi[i] == FUMATORI)   p.fumatori   = 1;
  }

  return p;
}
