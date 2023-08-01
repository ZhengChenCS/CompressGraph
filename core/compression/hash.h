
/*

Repair -- an implementation of Larsson and Moffat's compression and
decompression algorithms.
Copyright (C) 2010-current_year Gonzalo Navarro

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Author's contact: Gonzalo Navarro, Dept. of Computer Science, University of
Chile. Blanco Encalada 2120, Santiago, Chile. gnavarro@dcc.uchile.cl

*/

	// linear probing hash table for pairs

#ifndef HASHINCLUDED
#define HASHINCLUDED

#include "basics.h"
#include "records.h"

typedef struct
  { int *table;
    int maxpos; // of the form (1<<smth)-1
    int used;
    Trarray *Rec; // records
  } Thash;

Thash createHash (int maxpos, Trarray *Rec); 
					// creates new empty hash table

void destroyHash (Thash *H); // destroys hash table, not heap nor list

void insertHash (Thash *H, int id); // inserts H->Rec[id].pair in hash 
				   // assumes it is not already there
				   // sets ptr from Rec to hash as well

void deleteHash (Thash *H, int id); // deletes H->Rec[id].pair from hash

int searchHash (Thash H, Tpair p); // returns id, -1 if not existing

void hashRepos (Thash *H, int id); // repositions pair

#endif
