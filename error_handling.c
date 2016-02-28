#include "mpc.h"

#include <editline/readline.h>

/* Enum of possible Error Types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

enum { LVAL_NUM, LVAL_DOUBLE, LVAL_ERR };

typedef struct {
  int type;
  long num;
  double numd;
  int err;
} lval;

lval lval_double(double x) {
  lval v;
  v.type = LVAL_DOUBLE;
  v.numd = x;
  return v;
}

lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

lval lval_err(long x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

void lval_print(lval v) {
  switch (v.type) {
    case LVAL_DOUBLE:
      printf("%f", v.numd);
      break;
    case LVAL_NUM:
      printf("%li", v.num);
      break;
    case LVAL_ERR:
      if (v.err == LERR_DIV_ZERO) {
        printf("Error: Division by zero!");
      }
      if (v.err == LERR_BAD_OP) {
        printf("Error: Invalid operation!");
      }
      if (v.err == LERR_BAD_NUM) {
        printf("Error: Invalid number!");
      }
      break;
  }
}

void lval_printlin(lval v) {
  lval_print(v);
  putchar('\n');
}

lval eval_op(lval x, char* op, lval y) {
  if (x.type == LVAL_ERR) {
    return x;
  }
  if (y.type == LVAL_ERR) {
    return y;
  }

  if (strcmp(op, "+") == 0) {
    if (x.type == y.type && x.type == LVAL_NUM) {
      return lval_num(x.num + y.num);
    } else if (x.type == LVAL_DOUBLE) {
      return lval_double(x.numd + y.num);
    } else if (y.type == LVAL_DOUBLE) {
      return lval_double(x.num + y.numd);
    } else {
      return lval_num(x.num + y.num);
    }
  }
  if (strcmp(op, "-") == 0) {
    return lval_num(x.num - y.num);
  }
  if (strcmp(op, "*") == 0) {
    return lval_num(x.num * y.num);
  }
  if (strcmp(op, "/") == 0) {
    if (y.num == 0) {
      return lval_err(LERR_DIV_ZERO);
    } else {
      return lval_num(x.num / y.num);
    }
  }
  /* if (strcmp(op, "%") == 0) { */
  /*   if (y.num == 0) { */
  /*     return lval_err(LERR_DIV_ZERO); */
  /*   } else { */
  /*     return lval_num(x.num % y.num); */
  /*   } */
  /* } */

  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    errno = 0;
    if (strstr(t->contents, ".")) {
      double d = strtod(t->contents, NULL);
      return errno != ERANGE ? lval_double(d) : lval_err(LERR_BAD_NUM);
    } else {
      long x = strtol(t->contents, NULL, 10);
      return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }
  }

  char *op = t->children[1]->contents;
  lval x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main(int argc, char** argv) {
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Double = mpc_new("double");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+\\.?[0-9]*/ ;                   \
      operator : '+' | '-' | '*' | '/' | '%' ;            \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Double, Number, Operator, Expr, Lispy);

  puts("Lispy Version 0.0.0.0.4");
  puts("Press Ctrl+c to Exit \n");

  while (1) {
    char *input = readline("lispy> ");
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      lval result = eval(r.output);
      lval_printlin(result);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(5, Double, Number, Operator, Expr, Lispy);

  return 0;
}
