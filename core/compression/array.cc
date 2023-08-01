
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

// extendible array for pairs

#include "array.h"

#include <stdlib.h>

int insertArray(Tarray *A, int pair)

{
  if (A->size == A->maxsize) {
    if (A->maxsize == 0) {
      A->maxsize = A->minsize;
      A->pairs = (int *)malloc(A->maxsize * sizeof(int));
    } else {
      A->maxsize /= A->factor;
      A->pairs = (int *)realloc(A->pairs, A->maxsize * sizeof(int));
    }
  }
  A->pairs[A->size] = pair;
  return A->size++;
}

void deleteArray(Tarray *A)

{
  A->size--;
  if (A->size == 0) {
    A->maxsize = 0;
    free(A->pairs);
    A->pairs = NULL;
  } else if ((A->size < A->maxsize * A->factor * A->factor) &&
             (A->maxsize * A->factor >= A->minsize)) {
    A->maxsize *= A->factor;
    A->pairs = (int *)realloc(A->pairs, A->maxsize * sizeof(int));
  }
}

Tarray createArray(float factor, int minsize)

{
  Tarray A;
  A.pairs = NULL;
  A.maxsize = 0;
  A.size = 0;
  A.factor = factor;
  A.minsize = minsize;
  return A;
}

void destroyArray(Tarray *A)

{
  if (A->maxsize == 0) return;
  free(A->pairs);
  A->pairs = NULL;
  A->maxsize = 0;
  A->size = 0;
}
