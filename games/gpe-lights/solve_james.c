/* xlightoff solver
** written by James Williams
**
** This program is public domain.
*/

#include <stdio.h>
#include <stdlib.h>
#include "solve.h"

typedef struct {
  int num;
  int den;
} fraction;

static fraction matrix[26][25];
#define ABS(x)  ((x)<0?-(x):(x))

/*
** Find the greatest common divisor of two integers.
**
** Inputs:
**   a: one number
**   b: another number
**
** Returns:
**   The GCD of the two numbers.
*/
int gcd(int a, int b) {
  int tmp;
  
  a = ABS(a);
  b = ABS(b);

  if (a == b) {
    return a;
  }
  
  while (a != 0) {
    if (b > a) {
      tmp = a;
      a = b;
      b = tmp;
    }

    a = (a % b);
  }

  return b;
}

/*
** Reduce a fraction to its simplest terms
**
** Inputs:
**   a: a fraction
**
** Returns:
**   The reduced fraction
*/
fraction frac_reduce(fraction a) {
  int div = gcd(a.num, a.den);

  a.num /= div;
  a.den /= div;

  if (a.den < 0) {
    a.num *= -1;
    a.den *= -1;
  }

  return a;
}

/*
** Find the recipricol of a fraction
**
** Inputs:
**   a: a fraction
**
** Returns:
**   The recipricol
*/
fraction frac_recip(fraction a) {
  int tmp;

  tmp = a.num;
  a.num = a.den;
  a.den = tmp;

  return a;
}

/*
** Multiply two fractions
**
** Inputs:
**   a: a fraction
**   b: another fraction
**
** Returns:
**   a*b
*/
fraction frac_mult(fraction a, fraction b) {
  a.num *= b.num;
  a.den *= b.den;
  return frac_reduce(a);
}

/*
** Divide one fraction by another
**
** Inputs:
**   a: a fraction
**   b: another fraction
**
** Returns:
**   a/b
*/
fraction frac_divide(fraction a, fraction b) {
  return frac_mult(a, frac_recip(b));
}

/*
** Add two fractions
**
** Inputs:
**   a: a fraction
**   b: another fraction
**
** Returns:
**   a+b
*/
fraction frac_add(fraction a, fraction b) {
  fraction c,d;

  c.num = a.num * b.den;
  c.den = a.den * b.den;
  d.num = b.num * a.den;
  c.num += d.num;

  return (frac_reduce(c));
}

/*
** Subtract two fractions
**
** Inputs:
**   a: a fraction
**   b: another fraction
**
** Returns:
**   a-b
*/
fraction frac_sub(fraction a, fraction b) {
  fraction c,d;

  c.num = a.num * b.den;
  c.den = a.den * b.den;
  d.num = b.num * a.den;
  c.num -= d.num;

  return (frac_reduce(c));
}

/*
** Create a fraction from a numerator and denomimator
**
** Inputs:
**   num: The numerator
**   den: The denominator
**
** Returns:
**   The fraction
*/
fraction frac_new(int num, int den) {
  fraction f;

  f.num = num;
  f.den = den;
  return f;
}

/*
** Create a 26x25 matrix for a given set of results
**
** Inputs:
**   board: The 25 0's and 1's that represent the final board
**
** Returns:
**   void
*/
void matrix_new(int board[25]) {
  int x, y;

  /* Fill in the heart of the matrix */
  for (y=0; y<25; y++) {
    for (x=0; x<25; x++) {
      if (x == y
	  || x == y+5
	  || x == y-5
	  || (x == y-1 && y%5 != 0)
	  || (x == y+1 && (y+1)%5 != 0)) {
	matrix[x][y] = frac_new(1,1);
      } else {
	matrix[x][y] = frac_new(0,1);
      }
    }
  }

  /* Fill in the right column of the matrix (the solutions) */
  for (y=0; y<25; y++) {
    matrix[25][y] = frac_new(board[y], 1);
  }
}
  
/*
** Perform Gauss elimination on the matrix
**
** Inputs:
**   void
**
** Returns:
**   void
*/
void gauss(void) {
  int i, j, k, pivot_row;
  double largest;
  fraction frac, multiplier;
  fraction f1, f2, f3;
  int n1, n2, lcm;

  for (i=0; i<25; i++) {
    /* Locate the pivot row */
    largest = 0;
    pivot_row = 0;

    for (j=i; j<25; j++) {
      frac = matrix[i][j];
      if (ABS((double)frac.num/(double)frac.den) > largest) {
	largest = ABS((double)frac.num/(double)frac.den);
	pivot_row = j;
      }
    }

    if (largest != 0) {
      /* Swap the current row with the pivot row */
      for (j=0; j<26; j++) {
	frac = matrix[j][i];
	matrix[j][i] = matrix[j][pivot_row];
	matrix[j][pivot_row] = frac;
      }

      /* Perform elimination on remaining rows */
      for (k=i+1; k<25; k++) {
	multiplier = frac_divide(matrix[i][k], matrix[i][i]);
	multiplier.num *= -1;
	
	for (j=0; j<26; j++) {
	  f1 = matrix[j][k];
	  f2 = frac_mult(matrix[j][i], multiplier);
	  f3 = frac_add(f1, f2);
	  matrix[j][k] = f3;
	}

	/* Revert this row back to integers */
	/* Find the least common multiple of the denominators */
	n1 = matrix[0][k].den;
	n2 = matrix[1][k].den;
	lcm = n1*n2/gcd(n1,n2);

	for (j=2; j<26; j++) {
	  lcm = lcm*matrix[j][k].den/gcd(lcm, matrix[j][k].den);
	}

	frac = frac_new(lcm, 1);

	for (j=0; j<26; j++) {
	  matrix[j][k] = frac_mult(matrix[j][k], frac);
	  matrix[j][k].num %= 2;  /* We're doing all this in modulo 2 */
	}
      }
    }
  }
}

/*
** Test a given solution
**
** Inputs:
**   board: The original board
**   sol: A solution vector
**
** Returns:
**   1 if the solution works, 0 if it doesn't
**
*/  
int test_solution(int board[25], int sol[25]) {
  int i;

  for (i=0; i<25; i++) {
    if (sol[i]) {
      board[i] = 1-board[i];
      if (i >= 5)       board[i-5] = 1-board[i-5];
      if (i <= 19)      board[i+5] = 1-board[i+5];
      if ((i+1)%5 != 0) board[i+1] = 1-board[i+1];
      if (i%5 != 0)     board[i-1] = 1-board[i-1];
    }
  }

  /* See if it worked */
  for (i=0; i<25; i++) {
    if (board[i]) {
      return 0;  /* No go */
    }
  }

  return 1; /* It worked */
}

/*
** Find the solutions after Gauss elimination
**
** Inputs:
**   void
**
** Returns:
**   void
*/
void find_solutions(int board[25], int solution[25]) {
  int a, b;
  int i, j;
  int sol_size, min_size = 26;
  int sol[25];
  int board2[25];
  fraction inter;
  fraction results[25];

  /* Zero out the solution vector */
  for (i=0; i<25; i++) {
    solution[i] = 0;
  }

  for (a=0; a<2; a++) {
    for (b=0; b<2; b++) {
      results[23] = frac_new(a,1);
      results[24] = frac_new(b,1);

      for (i=22; i>=0; i--) {
	inter = matrix[25][i];
    
	for (j=i+1; j<25; j++) {
	  inter = frac_sub(inter, frac_mult(matrix[j][i], results[j]));
	}

	results[i] = inter;
      }

      sol_size = 0;

      for (i=0; i<25; i++) {
	sol[i] = ABS(results[i].num%2);
	sol_size += sol[i];
      }

      if (sol_size < min_size) {
	/* Copy the board */
	for (i=0; i<25; i++) {
	  board2[i] = board[i];
	}

	if (test_solution(board2, sol)) {
	  /* Just found a shorter solution */
	  for (i=0; i<25; i++) {
	    solution[i] = sol[i];
	  }

	  min_size = sol_size;
	}
      }
    }
  }
}

/*
** Generate a solution for the current board and display in on stdout
**
** Inputs:
**   it: The board
**
** Returns:
**   void
*/
void solve(interface *it) {
  int solution[25];
  int board[25];
  int i,j;

  for (j=0; j<5; j++) {
    for (i=0; i<5; i++) {
      board[j*5+i] = it->tab[j][i];
    }
  }

  matrix_new(board);
  gauss();
  find_solutions(board, solution);

  /* Copy the solution */
  for (i=0; i<25; i++) {
    it->sol[i/5][i%5] = solution[i];
  }
}
