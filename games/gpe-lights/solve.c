/* XLightOff solver
 * by Chupcko :
 *   chupcko@galeb.etf.bg.ac.yu
 *   http://alas.matf.bg.ac.yu/~chupcko/
 */

#include "solve.h"

#include <string.h>

#include "interface.h"

static int copytab[5][5];
static int copymoves[5][5];

/* this function will choose between the current solution
 * and the one we just calculated and take the shortest
 * (in number of steps)
 */
void helpit(interface *it)
{
  int s=0, t=0;
  int i;
  int *p = &it->sol[0][0];

  for (i=0; i<25; i++)
    s += *p++;

  p = &copymoves[0][0];

  for (i=0; i<25; i++)
    t += *p++;

  if (t < s)
    memcpy(it->sol, copymoves, 25*sizeof(int));
}

static void solve_action(interface *it, int x, int y)
{
  copytab[x][y] ^= 1;
  copymoves[x][y] ^= 1;

  if (y>0)
    copytab[x][y-1] ^= 1;

  if (x>0)
    copytab[x-1][y] ^= 1;

  if (y<N-1)
    copytab[x][y+1] ^= 1;

  if (x<N-1)
    copytab[x+1][y] ^= 1;
}

void solve(interface *it)
{
/*

0 00 0 00 0 ~ 00000 01110 10101 11011
1 00 1 11 7 ~ 00010 01100 10111 11001
2 01 0 10 2 ~ 00111 01001 10010 11100
3 01 1 01 5 ~ 00101 01011 10000 11110
4 10 0 01 1 ~ 00011 01101 10110 11000
5 10 1 10 6 ~ 00001 01111 10100 11010
6 11 0 11 3 ~ 00100 01010 10001 11111
7 11 1 00 4 ~ 00110 01000 10011 11101

*/

  int x, y, r, t;
  static int s[8][4][N]= {
    {{0,0,0,0,0},
     {0,1,1,1,0},
     {1,0,1,0,1},
     {1,1,0,1,1}},

    {{0,0,0,1,0},
     {0,1,1,0,0},
     {1,0,1,1,1},
     {1,1,0,0,1}},

    {{0,0,1,1,1},
     {0,1,0,0,1},
     {1,0,0,1,0},
     {1,1,1,0,0}},

    {{0,0,1,0,1},
     {0,1,0,1,1},
     {1,0,0,0,0},
     {1,1,1,1,0}},

    {{0,0,0,1,1},
     {0,1,1,0,1},
     {1,0,1,1,0},
     {1,1,0,0,0}},

    {{0,0,0,0,1},
     {0,1,1,1,1},
     {1,0,1,0,0},
     {1,1,0,1,0}},

    {{0,0,1,0,0},
     {0,1,0,1,0},
     {1,0,0,0,1},
     {1,1,1,1,1}},

    {{0,0,1,1,0},
     {0,1,0,0,0},
     {1,0,0,1,1},
     {1,1,1,0,1}}
  };

  /* let's init the solution to all 1 (it will be shorter) */
  {
    int t[25] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    memcpy(it->sol, t, 25*sizeof(int));
  }

  memcpy(copytab, it->tab, N*N*sizeof(int));
  for (x=0; x<N-1; x++)
    for (y=0; y<N; y++)
      if (copytab[x][y])
        solve_action(it,x+1,y);
  r=copytab[N-1][0]<<2|copytab[N-1][1]<<1|copytab[N-1][2];
  for (t=0; t<4; t++) {
    memcpy(copytab, it->tab, N*N*sizeof(int));
    bzero(copymoves, N*N*sizeof(int));
    for (y=0; y<N; y++)
      if (s[r][t][y])
        solve_action(it,0,y);
    for (x=0; x<N-1; x++)
      for (y=0; y<N; y++)
        if (copytab[x][y])
          solve_action(it,x+1,y);
    helpit(it);
  }
}
