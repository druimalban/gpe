/*
 * Switch the light off !
 * A little brain-oriented game.
 *-----
 * To My angel.
 * January 2002.
 * Sed
 *-----
 * This file is in the public domain.
 *-----
 * the solve file
 */

#include "solve.h"

#include <string.h>

#include "interface.h"

/* booo, a static global ! */
static int m[25][26];

/* we init the matrix according to the rules of the game
 * The matrix looks like this :
 * x x x ... x x y
 * x x x ... x x y
 * .
 * .
 * .
 * x x x ... x x y
 *
 * where the x come from the game rules and the y from the initial lighting.
 *
 * How are the x calculated ?
 * each line is the dependance flow of a cell.
 * For example, let's take the first line.
 * The first line is the dependance flow of the (0, 0) cell, the upper left
 * one.
 * This cell depends on itself, and on its two neighbors, the right one and
 * the lower one. If you click one of those three cells, it will change the
 * value of the (0, 0) cell. So you will find a 1 for those three cells
 * in the first line of the matrix (and maybe one more for the y if this
 * cell was initially on).
 * This construction may look strange but it is correct.
 * Seen differently, we have :
 * let's Yx (x from 0 to 24) and Y'x represent the value of the cell x (cells
 * are numeroted like this :
 *  1  2  3  4  5
 *  6  7  8  9 10
 * 11 12 13 14 15
 * 16 17 18 19 20
 * 21 22 23 24 25)
 * Yx for current value and Y'x for next value,
 * and Xx (x from 0 to 24) represent a click on the cell x.
 * So we have :
 * Y'1 = (Y1 + X1 + X2 + X6) mod 2
 * Imagine you click nothing, thus X1 = X2 = X6 = 0, so Y'1 = Y1 which is ok.
 * Now, imagine you click all, so : Y'1 = (Y1 + 3) mod 2 = (Y1 + 1) mod 2,
 * which means you change the value of Y1, which is correct if you look at
 * the game rules. You can check the other cases.
 * You repeat it for each cell, and there you are with the matrix.
 */
static void init_matrix(void)
{
  int i, j, l;

  memset(m, 0, 26*25*sizeof(int));

  /* where the magic remains... */
  for (l=0; l<25; l++) {
    i = l % 5;
    j = l / 5;
    m[l][i+j*5] = 1;
    if (i>0)
      m[l][(i-1)+j*5] = 1;
    if (i<4)
      m[l][(i+1)+j*5] = 1;
    if (j>0)
      m[l][i+(j-1)*5] = 1;
    if (j<4)
      m[l][i+(j+1)*5] = 1;
  }
}

/* get (from the game) the initial lighting of the game */
static void get_init_pos(interface *it)
{
  int *t = &it->tab[0][0];
  int i;

  for (i=0; i<25; i++, t++)
    m[i][25] = *t;
}

/* exchange line a and b of the matrix */
static void exchange(int a, int b)
{
  int t[26];

  if (a==b)
    return;

  memcpy(t, m[a], 26*sizeof(int));
  memcpy(m[a], m[b], 26*sizeof(int));
  memcpy(m[b], t, 26*sizeof(int));
}

/* substract line b from the matrix to line a */
/* because we are in arithmetic modulo 2, substraction is xor */
static void substract(int a, int b)
{
  int i;

  for (i=0; i<26; i++)
    m[a][i] ^= m[b][i];
}

static void transform_matrix(void)
{
  int i, l;

  /* We want a matrix like this :
   *
   *   x x x x ... x x x y
   *   0 x x x ... x x x y
   *   0 0 x x ... x x x y
   *   0 0 0 x ... x x x y
   *   .
   *   .
   *   .
   *   0 0 0 0 ... 0 x x y
   *   0 0 0 0 ... 0 0 x y
   *
   * (x are coming from the board, y are the 25 initial light's values)
   *
   *  In fact, we'll get "0 ... 0 y" for the two last lines, this means
   * two vars can be chosen with any value we want, leading to 4 solutions
   * if both the corresponding y are 0. If one of the y is 1, there is no
   * solution to the system.
   */
  for (i=0; i<24; i++) {
    /* on each line, first find a line with a leading 1 at pos i */
    for (l=i; l<25; l++)
      if (m[l][i]) {
	exchange(l, i);
	break;
      }
    if (l == 25)
      continue;

    /* found one, substract line i to all following lines with a 1 at pos i */
    for (l=i+1; l<25; l++)
      if (m[l][i])
	substract(l, i);
  }
}

/* the matrix has been transformed, v[23] and v[24] have been chosen,
 * let's set the other v[x] to their value.
 */
static void get_solution(int v[25])
{
  int i;

  /* v[23] and v[24] are set, let's set the other, according to the matrix */
  /* first, we set v[i] to the value of m[i][25] (which is the last column,
   * which is a constant in the system), then substract the value of v[j]
   * if their is a one in m[i][j]. The equation for the line i is :
   * Ai Xi + A(i+1) X(i+1) + ... + An Xn + B = 0
   * (we know Ai = 1)
   * We are in modulo 2 arithmetic, so we set Xi (ie v[i]) to B, then
   * substract Xj if Aj = 1
   *  Due to the matrix transformation, we know Aj = 0 for all j < i,
   * so we run from X22 to X0.
   * Arithmetic modulo 2 means substraction is in fact a xor.
   */
  for (i=22; i>=0; i--) {
    int l;

    v[i] = m[i][25];

    for (l=i+1; l<25; l++)
      if (m[i][l])
	v[i] ^= v[l];
  }
}

/* returns the number of 1 in v */
static int get_size(int v[25])
{
  int i, l=0;

  for (i=0; i<25; i++)
    l += v[i];

  return l;
}

static int do_solve(interface *it)
{
  int cur, cur_size, size;
  int i;

  int sol[4][25];

  /* first, transform the matrix */
  transform_matrix();

  /* then check that the last too lines are ok */
  if (m[23][25] || m[24][25])
    return -1;

  /* now, get the four solutions (four, because vars 23 and 24 are free
   * and can take 0 or 1 as value, leading to four possibilities)
   */
  sol[0][23] = 0;
  sol[0][24] = 0;
  get_solution(sol[0]);

  sol[1][23] = 0;
  sol[1][24] = 1;
  get_solution(sol[1]);

  sol[2][23] = 1;
  sol[2][24] = 0;
  get_solution(sol[2]);

  sol[3][23] = 1;
  sol[3][24] = 1;
  get_solution(sol[3]);

  /* now get the shortest solution */
  cur = 0;
  cur_size = get_size(sol[0]);

  for (i=1; i<4; i++)
    if ((size = get_size(sol[i])) < cur_size)
      cur_size = size, cur = i;

  memcpy(it->sol, sol[cur], 25*sizeof(int));

  return 0;
}

void solve(interface *it)
{
  init_matrix();
  get_init_pos(it);
  do_solve(it);
}
