#include "tac.h"
#include "optimization.h"

AST *last_cmd_list;
AST *last_loop_cmd_list;

char *tac_type_str[] = {
    "",
    "TAC_SYMBOL",
    "TAC_ADD",
    "TAC_SUB",
    "TAC_MUL",
    "TAC_DIV",
    "TAC_NEG",
    "TAC_NOT",
    "TAC_AND",
    "TAC_OR",
    "TAC_LE",
    "TAC_GE",
    "TAC_EQ",
    "TAC_DIF",
    "TAC_GT",
    "TAC_LT",
    "TAC_COPY",
    "TAC_JFALSE",
    "TAC_LABEL",
    "TAC_JTRUE",
    "TAC_JUMP",
    "TAC_RET",
    "TAC_BEGINFUN",
    "TAC_ENDFUN",
    "TAC_CALL",
    "TAC_ARG",
    "TAC_VEC_ACCESS",
    "TAC_PRINT",
    "TAC_READ",
    "TAC_PRINT_ARG",
    "TAC_VARDEC",
    "TAC_VECDEC"};

// TAC methods
TAC *tac_create(int type, HASH *res, HASH *op1, HASH *op2)
{
    TAC *tac = NULL;
    tac = (TAC *)calloc(1, sizeof(TAC));

    tac->type = type;
    tac->res = res;
    tac->op1 = op1;
    tac->op2 = op2;
    tac->prev = NULL;
    tac->next = NULL;

    return tac;
}

void tac_print(TAC *tac)
{
    if (tac == NULL || (tac && tac->type == TAC_SYMBOL))
        return;
    fprintf(
        stderr, "TAC(%s, %s, %s, %s)\n",
        tac->type < TAC_TYPE_COUNT ? tac_type_str[tac->type] : "TAC_UNKNOWN",
        tac->res && tac->res->text ? tac->res->text : "0",
        tac->op1 && tac->op1->text ? tac->op1->text : "0",
        tac->op2 && tac->op2->text ? tac->op2->text : "0");
    tac_print(tac->next);
}

void tac_print_backwards(TAC *tac)
{
    if (tac == NULL)
        return;

    tac_print_backwards(tac->prev);
    tac_print(tac);
}

TAC *tac_join(TAC *l1, TAC *l2)
{
    TAC *tac = NULL;
    if (l1 == NULL)
        return l2;
    if (l2 == NULL)
        return l1;

    for (tac = l2; tac->prev != NULL; tac = tac->prev)
        ;
    tac->prev = l1;
    return l2;
}

// Code Generation

TAC *make_binary_operation(AST *node, int type, TAC *code0, TAC *code1)
{
    int datatype = code0 ? code0->res->datatype : 0;
    if (datatype == 0)
    {
        datatype = node->result_datatype;
    }

    return tac_join(tac_join(code0, code1), tac_create(type, make_temp(datatype), code0 ? code0->res : NULL, code1 ? code1->res : NULL));
}

TAC *make_if(TAC *code0, TAC *code1)
{
    TAC *jumptac = NULL;
    TAC *labeltac = NULL;
    HASH *if_label = NULL;

    if_label = make_label(CONDITIONAL_IF);

    jumptac = tac_create(TAC_JFALSE, if_label, code0 ? code0->res : NULL, NULL);
    jumptac->prev = code0;
    labeltac = tac_create(TAC_LABEL, if_label, NULL, NULL);
    labeltac->prev = code1;

    return tac_join(jumptac, labeltac);
}

TAC *make_if_else(TAC *code0, TAC *code1, TAC *code2)
{
    TAC *jumptac = NULL;
    TAC *labeltac = NULL;
    TAC *endtac = NULL;
    TAC *unconditional_jump_tac = NULL;
    HASH *else_label = NULL;
    HASH *if_label = NULL;
    HASH *end_label = NULL;

    else_label = make_label(CONDITIONAL_ELSE);
    if_label = make_label(CONDITIONAL_IF);

    end_label = make_label(CONDITIONAL_ENDIF);

    jumptac = tac_create(TAC_JFALSE, else_label, code0 ? code0->res : NULL, NULL);
    jumptac->prev = code0;
    labeltac = tac_create(TAC_LABEL, else_label, NULL, NULL);
    unconditional_jump_tac = tac_create(TAC_JUMP, end_label, NULL, NULL);
    labeltac->prev = unconditional_jump_tac;

    unconditional_jump_tac->prev = code1;

    endtac = tac_create(TAC_LABEL, end_label, NULL, NULL);
    endtac->prev = code2;

    return tac_join(tac_join(jumptac, labeltac), endtac);
}

TAC *make_loop(TAC *code0, TAC *code1)
{
    TAC *loop_start_tac = NULL;
    TAC *loop_jump_tac = NULL;
    TAC *loop_end_tac = NULL;
    TAC *unconditional_jump_tac = NULL;

    HASH *loop_start_label = NULL;
    HASH *loop_end_label = NULL;

    loop_start_label = make_label(LOOP_START);
    loop_start_tac = tac_create(TAC_LABEL, loop_start_label, NULL, NULL);

    loop_end_label = make_label(LOOP_END);
    loop_jump_tac = tac_create(TAC_JFALSE, loop_end_label, code0 ? code0->res : NULL, NULL);
    loop_jump_tac->prev = code0;

    unconditional_jump_tac = tac_create(TAC_JUMP, loop_start_label, NULL, NULL);
    loop_end_tac = tac_create(TAC_LABEL, loop_end_label, NULL, NULL);

    return tac_join(tac_join(tac_join(loop_start_tac, loop_jump_tac), tac_join(code1, unconditional_jump_tac)), loop_end_tac);
}

TAC *make_unary_operation(int type, TAC *code0)
{
    int datatype = code0 ? code0->res->datatype : 0;
    return tac_join(code0, tac_create(type, make_temp(datatype), code0 ? code0->res : NULL, NULL));
}

TAC *make_function(AST *node, TAC *code0, TAC *code1)
{
    TAC *jumptac = NULL;
    TAC *labeltac = NULL;

    jumptac = tac_create(TAC_BEGINFUN, node->symbol, NULL, NULL);
    jumptac->prev = code0;
    labeltac = tac_create(TAC_ENDFUN, node->symbol, NULL, NULL);
    labeltac->prev = code1;

    return tac_join(jumptac, labeltac);
}

TAC *make_call(AST *node, TAC *code0, TAC *code1)
{
    TAC *call_tac = NULL;
    int datatype = code0 ? code0->res->datatype : 0;
    call_tac = tac_create(TAC_CALL, make_temp(datatype), node->symbol, NULL);

    return tac_join(tac_join(code0, code1), call_tac);
}

TAC *make_arg(AST *node, TAC *code0, TAC *code1)
{
    TAC *arg_tac = NULL;

    // how can I improve that?
    HASH *temp = (HASH *)calloc(1, sizeof(HASH));
    temp->text = (char *)calloc(1, sizeof(char));
    temp->text = node->func_param;

    arg_tac = tac_create(TAC_ARG, temp, code0 ? code0->res : NULL, NULL);
    arg_tac->prev = code0;

    return tac_join(arg_tac, code1);
}

TAC *make_print_arg(TAC *code0, TAC *code1, HASH *str)
{
    TAC *print_tac = NULL;

    if (str == NULL)
    {
        print_tac = tac_create(TAC_PRINT_ARG, code0 ? code0->res : NULL, NULL, NULL);
    }
    else
    {
        print_tac = tac_create(TAC_PRINT_ARG, str, NULL, NULL);
    }

    print_tac->prev = code0;

    return tac_join(print_tac, code1);
}

TAC *generate_code(AST *node)
{
    if (node == NULL)
        return NULL;
    int i;
    TAC *result = NULL;
    TAC *code[MAX_SONS];

    for (i = 0; i < MAX_SONS; i++)
    {
        code[i] = generate_code(node->son[i]);
    }

    switch (node->type)
    {
    case AST_IDENTIFIER:
    case AST_LIT_INT:
    case AST_LIT_REAL:
    case AST_LIT_CHAR:
    {
        result = tac_create(TAC_SYMBOL, node->symbol, NULL, NULL);
        break;
    }
    case AST_ADD:
    case AST_SUB:
    case AST_MUL:
    case AST_DIV:
    case AST_AND:
    case AST_OR:
    case AST_LE:
    case AST_GE:
    case AST_EQ:
    case AST_DIF:
    case AST_GT:
    case AST_LT:
    {
        int tac_type = get_tac_type_from_ast(node->type);
        result = make_binary_operation(node, tac_type, code[0], code[1]);
        break;
    }
    case AST_NEG:
    case AST_NOT:
    {
        int tac_type = get_tac_type_from_ast(node->type);
        result = make_unary_operation(tac_type, code[0]);
        break;
    }
    case AST_VAR_ATTRIB:
    {
        result = tac_join(code[0], tac_create(TAC_COPY, node->symbol, code[0] ? code[0]->res : NULL, NULL));
        break;
    }
    case AST_VEC_ATTRIB:
    {
        result = tac_join(code[0], tac_join(code[1], tac_create(TAC_COPY, node->symbol, code[0] ? code[0]->res : NULL, code[1] ? code[1]->res : NULL)));
        break;
    }
    case AST_VEC_ACCESS:
    {
        int datatype = code[0] ? code[0]->res->datatype : 0;
        result = tac_join(code[0], tac_create(TAC_COPY, make_temp(datatype), node->symbol, code[0] ? code[0]->res : NULL));
        break;
    }
    case AST_IF:
    {
        result = make_if(code[0], code[1]);
        break;
    }
    case AST_IF_ELSE:
    {
        result = make_if_else(code[0], code[1], code[2]);
        break;
    }
    case AST_LOOP:
    {
        result = make_loop(code[0], code[1]);
        break;
    }
    case AST_RETURN_CMD:
    {
        result = tac_join(code[0], tac_create(TAC_RET, code[0] ? code[0]->res : NULL, NULL, NULL));
        break;
    }
    case AST_FUNC_DECL_INT:
    case AST_FUNC_DECL_REAL:
    case AST_FUNC_DECL_CHAR:
    case AST_FUNC_DECL_BOOL:
    {
        result = make_function(node, code[0], code[1]);

        TAC *beginfun_tac = NULL;
        for (beginfun_tac = result; beginfun_tac->type != TAC_BEGINFUN; beginfun_tac = beginfun_tac->prev)
            ;
        break;
    }
    case AST_FUNC_CALL:
    {
        result = make_call(node, code[0], code[1]);
        break;
    }
    case AST_EXPR_LIST:
    {

        result = make_arg(node, code[0], code[1]);
        break;
    }
    case AST_OUTPUT_CMD:
    {
        result = tac_join(code[0], tac_create(TAC_PRINT, NULL, NULL, NULL));
        break;
    }
    case AST_OUTPUT_PARAM_LIST:
    {
        if (node->son[0] && node->son[0]->type == AST_LIT_STRING)
        {
            result = make_print_arg(code[0], code[1], node->son[0]->symbol);
        }
        else
        {
            result = make_print_arg(code[0], code[1], NULL);
        }
        break;
    }
    case AST_INPUT_EXPR_INT:
    {
        result = tac_create(TAC_READ, make_temp(DATATYPE_INT), NULL, NULL);
        break;
    }
    case AST_INPUT_EXPR_REAL:
    {
        result = tac_create(TAC_READ, make_temp(DATATYPE_REAL), NULL, NULL);
        break;
    }
    case AST_INPUT_EXPR_CHAR:
    {
        result = tac_create(TAC_READ, make_temp(DATATYPE_CHAR), NULL, NULL);
        break;
    }

    case AST_VAR_DECL_INT:
    case AST_VAR_DECL_REAL:
    case AST_VAR_DECL_CHAR:
    case AST_VAR_DECL_BOOL:
    {
        result = tac_create(TAC_VARDEC, node->symbol, node->son[0]->symbol, NULL);
        break;
    }
    case AST_VEC_DECL_INT:
    case AST_VEC_DECL_REAL:
    case AST_VEC_DECL_CHAR:
    case AST_VEC_DECL_BOOL:
    {
        result = tac_join(tac_join(code[0], tac_create(TAC_VECDEC, node->symbol, code[0] ? code[0]->res : NULL, NULL)), code[1]);
        break;
    }
    default:
    {
        result = tac_join(code[0], tac_join(code[1], tac_join(code[2], code[3])));
        break;
    }
    }

    return result;
}

int get_tac_type_from_ast(int type)
{
    switch (type)
    {
    case AST_ADD:
        return TAC_ADD;
    case AST_SUB:
        return TAC_SUB;
    case AST_MUL:
        return TAC_MUL;
    case AST_DIV:
        return TAC_DIV;
    case AST_AND:
        return TAC_AND;
    case AST_OR:
        return TAC_OR;
    case AST_LE:
        return TAC_LE;
    case AST_GE:
        return TAC_GE;
    case AST_EQ:
        return TAC_EQ;
    case AST_DIF:
        return TAC_DIF;
    case AST_GT:
        return TAC_GT;
    case AST_LT:
        return TAC_LT;
    case AST_NEG:
        return TAC_NEG;
    case AST_NOT:
        return TAC_NOT;
    default:
        return -1;
    }
}

TAC *tac_reverse(TAC *tac)
{
    if (!tac)
        return NULL;
    TAC *aux = tac;
    for (aux = tac; aux->prev; aux = aux->prev)
    {
        aux->prev->next = aux;
    }

    return aux;
}

