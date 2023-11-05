#include <err.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ANSI_RED_BG "\x1b[41m"

#define ANSI_RESET "\x1b[0m"

#ifndef TRUE
  #define TRUE 1
#endif

#ifndef FALSE
  #define FALSE 0
#endif

enum {
  NUMBER_STACK_MAX = 128,
  OPERATOR_STACK_MAX = 64,
  TOTAL_STACK_MAX = NUMBER_STACK_MAX + OPERATOR_STACK_MAX
};

/*
 * Holds the count of all the members in number stack
 */
static size_t numstackcount = 0;

/*
 * Holds the numbers we want to calculate
 */
static double numstack[NUMBER_STACK_MAX];

/*
 * Holds the count of all the members in operator stack
 */
static size_t operstackcount = 0;

/*
 * Holds all the operators for our calculations
 */
static char operstack[OPERATOR_STACK_MAX];

double
calcadd(double n1, double n2)
{
  return (n1 + n2);
}

double
calcsub(double n1, double n2)
{
  return (n1 - n2);
}

double
calcmul(double n1, double n2)
{
  return (n1 * n2);
}

double
calcdiv(double n1, double n2)
{
  return (n1 / n2);
}

double
calcmod(double n1, double n2)
{
  return (long)n1 % (long)n2;
}

/*
 * Checks to see if a number is valid
 * A number is only allowed on decimal point,
 * one + or - symbol and only the numeric chars
 */
int
isnumeric(const char * s)
{
  size_t i;
  int decimalcount = 0;

  if (strspn(s, "+-.01234567890") != strlen(s))
    return FALSE;

  for (i = 0; s[i]; i++) {
    if (s[i] == '.' && !isdigit(s[i-1]))
      return FALSE;

    if (i != 0 && s[i] == '-')
      return FALSE;

    if (i != 0 && s[i] == '+')
      return FALSE;

     if (s[i] == '.')
      ++decimalcount;
  }

  if (decimalcount >= 2)
    return FALSE;

  return TRUE;
}

/*
 * Puts a double into the next free space in number array
 */
int
pushd(const double n)
{
  if (numstackcount < NUMBER_STACK_MAX)
    numstack[numstackcount++] = n;

  else {
    fprintf(stderr, "%sToo many numbers in stack%s\n", ANSI_RED_BG, ANSI_RESET);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/*
 * Puts a new char into the next free space in operator array
 */
int
pusho(const char c)
{
  if (operstackcount < OPERATOR_STACK_MAX)
    operstack[operstackcount++] = c;

  else {
    fprintf(stderr, "%sToo many operators in stack%s\n", ANSI_RED_BG, ANSI_RESET);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/*
 * Clears the values of number array and operator array
 * Resets the respective counts for each array
 */
static void 
clearstack(void)
{
  size_t i;

  for (i = 0; numstack[i]; i++)
    numstack[i] = 0;

  for (i = 0; operstack[i]; i++)
    operstack[i] = 0;

  numstackcount = operstackcount = 0;
}

/*
 * Attempts to calculate n1 op n2 and places value in result
 * Returns FAIL if the op is not valid
 */
static int
calculate(double n1, const char op, double n2, double * result)
{
  switch(op) {
    case '+':
      *result = calcadd(n1,n2);
      return EXIT_SUCCESS;
    case '-':
      *result = calcsub(n1,n2);
      return EXIT_SUCCESS;
    case '*':
    case 'x':
    case 'X':
      *result = calcmul(n1,n2);
      return EXIT_SUCCESS;
    case '/':
      *result = calcdiv(n1,n2);
      return EXIT_SUCCESS;
    case '^':
    case '~':
      *result = pow(n1,n2);
      return EXIT_SUCCESS;
    case '%':
      *result = calcmod(n1,n2);
      return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}

/*
 * Goes through string array following an assumption that,
 * a calculation expression looks like this '1 + 1 - 2'.
 * Or number operator number operator number
 */
static double
calculationloop(const double n[], const char * o)
{
  size_t i, k;
  double active, result;

  active = result = 0;

  for (i = 0, k = 0; n[i]; i++) {
    active = n[i];

    if (!i)
      calculate(active, o[k++], n[++i], &result);
    else
      calculate(result, o[k++], active, &result);
  }

  return result;
}

/*
 * Tries to convert each member of argv to either a number or char
 * It follows the assumption that we have a number operator number
 * It ignores the first element in order to keep compatability with,
 * the command line.
 */
static double 
parseexpr(const char ** argv)
{
  int n = 1;
  double test = 0;

  while (*++argv) {
    if (n) {
      if (!isnumeric(*argv)) {
        fprintf(stderr, "%s%s is not numeric%s\n", ANSI_RED_BG, *argv, ANSI_RESET);
        return 0;
      }

      /*
       * Convert the string to a double
       */
      if (pushd((double)strtod(*argv, (char**)NULL)))
        return 0;

      n = 0;
    }

    else {
      if (calculate(1, **argv, 1, &test) || strlen(*argv) >= 2) {
        fprintf(stderr, "%s%s is not a valid operator%s\n", ANSI_RED_BG, *argv, ANSI_RESET);
        return 0;
      }

      if (pusho(**argv))
        return 0;

      n = 1;
    }
  }

  return calculationloop(numstack, operstack); 
}

/*
 * Goes through a string splitting it up by numbers and chars,
 * It places the numbers and non numbers in a string array,
 * It then passes this array off to the expression parser,
 */
int
calcentry(const char * s, double * result)
{
  int i, k;
  char ** tokv;

  tokv = (char**)malloc(TOTAL_STACK_MAX*2); 

  if (!tokv)
    err(EXIT_FAILURE, "tokv malloc");

  memset(tokv, 0, TOTAL_STACK_MAX*2);

  tokv[0] = strndup("NONVALUE", 9);

  i = 1;

  do {
    if (!*s)
      break;

    if (!tokv[i]) {
      tokv[i] = (char*)malloc(32);

      if (!tokv[i])
        err(EXIT_FAILURE, "malloc tokv[%d]", i);

      memset(tokv[i], 0, strlen(tokv[i]));
    }

    if (isspace(*s) || isblank(*s) || iscntrl(*s))
      ; /* Ignore whitespace */

    else if (isdigit(*s))
      strncat(tokv[i], s, 1);

    /*
     * Treat decimals as part of a number
     */
    else if (*s == '.')
      strncat(tokv[i], s, 1);

    /*
     * Check for negative numbers
     */
    else if (*s == '-' && isdigit(*s+1)) {
      for (k = 0; isspace(*s-k); k++)
        ;

      /*
       * Treat the char as an operator 
       * if there is a number before it
       */
      if (isdigit((*s-k))) {
        tokv[++i] = strndup(s, 1);
        i++;
      }

      else
        strncat(tokv[i], s, 1);
    }

    /*
     * If all other checks fail,
     * Treat the char as an operator
     */
    else {
      tokv[++i] = strndup(s, 1);
      i++;
    }

  } while (*s++);

  *result = parseexpr((const char**)tokv);

  for (i = 0; tokv[i]; i++) {
    puts(tokv[i]);
    free((char*)tokv[i]);
  }

  free((char**)tokv);
  return EXIT_SUCCESS;
}

/*
 * Follows the num op num op assumption
 * Reads the file line by line,
 * Calculating each line and printing it
 * Reads in a maximum of 384 bytes per line
 */
int
readfile(FILE * fp)
{
  double result = 0;
  char buf[TOTAL_STACK_MAX*2];

  while (fgets(buf, (TOTAL_STACK_MAX*2) - 1, fp)) {
    if (calcentry(buf, &result))
      return EXIT_FAILURE;

    clearstack();
    fprintf(stdout, "%0.4f\n", result);
  }

  return EXIT_SUCCESS;
}

/*
 * Attempts to open all arguments of argv,
 * The function starts from the offset.
 */
int
openfile(const char ** argv, int offset)
{
  size_t i;
  FILE * fp;

  for (i = offset; argv[i]; i++) {
    if (!(fp = fopen(argv[i], "r")))
      err(errno, "fopen(%s)", argv[i]);

    if (readfile(fp)) {
      if (fclose(fp))
        err(errno, "fclose(%s)", argv[i]);

      return EXIT_FAILURE;
    }

    if (fclose(fp))
      err(errno, "fclose(%s)", argv[i]);
  }

  return EXIT_SUCCESS;
}

int
eventloop(void)
{
  for (;;)
    readfile(stdin);

  return EXIT_SUCCESS;
}

int
parseargs(const char ** argv)
{

  return EXIT_SUCCESS;
}

int
main(int argc, const char ** argv)
{
  return (argc < 2) ? eventloop() : openfile(argv, 1);
}
