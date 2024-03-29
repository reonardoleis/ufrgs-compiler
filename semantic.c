#pragma once
#include "semantic.h"
#include "ast.h"
#include "ast_types.h"
#include "hash.h"
#include "symbols.h"
#include "debug.h"
#include "semantic_utils.h"

int SemanticErrors = 0;

void check_and_set_declarations(AST *node)
{
    int i;
    int required_vec_type = 0;
    if (!node)
        return;

    switch (node->type)
    {
    case AST_VAR_DECL_INT:
    case AST_VAR_DECL_CHAR:
    case AST_VAR_DECL_REAL:
    case AST_VAR_DECL_BOOL:
    {
        if (node->symbol)
        {
            if (node->symbol->type != SYMBOL_IDENTIFIER)
            {
                fprintf(stderr, "Semantic error: identifier %s already declared at line %d\n", node->symbol->text, node->line_number);
                ++SemanticErrors;
            }

            node->symbol->type = SYMBOL_VARIABLE;
            node->symbol->datatype = ast_type_to_datatype(node->type);
        }

        break;
    }
    case AST_VEC_DECL_INT:
    case AST_VEC_DECL_CHAR:
    case AST_VEC_DECL_REAL:
    case AST_VEC_DECL_BOOL:
    {
        int required_vec_type = 0;
        if (node->type == AST_VEC_DECL_INT)
            required_vec_type = AST_LIT_INT;
        else if (node->type == AST_VEC_DECL_CHAR)
            required_vec_type = AST_LIT_CHAR;
        else if (node->type == AST_VEC_DECL_REAL)
            required_vec_type = AST_LIT_REAL;

        if (node->symbol)
        {
            if (node->symbol->type != SYMBOL_IDENTIFIER)
            {
                fprintf(stderr, "Semantic error: identifier %s already declared at line %d\n", node->symbol->text, node->line_number);
                ++SemanticErrors;
            }

            node->symbol->type = SYMBOL_VECTOR;
            node->symbol->datatype = ast_type_to_datatype(node->type);
            node->symbol->is_vector = 1;
            int vec_size = atoi(node->son[0]->symbol->text);
            int initialization_count = 0;

            if (node->son[1] != NULL)
            {
                AST *initialization_item = node->son[1];
                while (initialization_item != NULL)
                {
                    if (!(verify_literal_compatibility(initialization_item->son[0]->type, required_vec_type)))
                    {
                        fprintf(stderr, "Semantic error: vector %s has initialization item with wrong type (expected type %s got %s) at line %d\n",
                                node->symbol->text, ast_type_str(required_vec_type),
                                ast_type_str(initialization_item->son[0]->type), node->line_number);
                        ++SemanticErrors;
                    }
                    initialization_item = initialization_item->son[1];
                    ++initialization_count;
                }

                if (initialization_count != vec_size)
                {
                    fprintf(stderr, "Semantic error: vector %s has %d initialization items, but its size is %d at line %d\n",
                            node->symbol->text, initialization_count, vec_size, node->line_number);
                    ++SemanticErrors;
                }
            }
        }

        break;
    }
    case AST_FUNC_DECL_INT:
    case AST_FUNC_DECL_CHAR:
    case AST_FUNC_DECL_REAL:
    case AST_FUNC_DECL_BOOL:
    {
        if (node->symbol)
        {
            if (node->symbol->type != SYMBOL_IDENTIFIER)
            {
                fprintf(stderr, "Semantic error: identifier %s already declared at line %d\n", node->symbol->text, node->son[0]->line_number);
                ++SemanticErrors;
            }

            node->symbol->type = SYMBOL_FUNCTION;
            node->symbol->datatype = ast_type_to_datatype(node->type);
            node->symbol->is_function = 1;
            set_function_id(node->symbol);

            AST *param = node->son[0];
            if (param->type == AST_EMPTY_PARAM_LIST)
            {
                break;
            }
            int count = 0;
            while (param)
            {

                node->symbol->params[count] = ast_type_to_datatype(param->son[0]->type);
                param = param->son[1];
                ++count;
            }

            node->symbol->param_count = count;
        }

        break;
    }

    case AST_PARAM_INT:
    case AST_PARAM_CHAR:
    case AST_PARAM_REAL:
    case AST_PARAM_BOOL:
    {
        if (node->symbol)
        {
            if (node->symbol->type != SYMBOL_IDENTIFIER)
            {
                fprintf(stderr, "Semantic error: identifier %s already declared at line %d\n", node->symbol->text, node->line_number);
                ++SemanticErrors;
            }

            node->symbol->type = SYMBOL_PARAMETER;
            node->symbol->datatype = ast_type_to_datatype(node->type);
        }

        break;
    }
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        check_and_set_declarations(node->son[i]);
    }
}

void check_undeclared(void)
{
    SemanticErrors += hash_check_undeclared();
}

void check_operands(AST *node)
{
    int i;

    if (!node)
        return;

    if (is_input_cmd(node))
    {
        int datatype = get_input_cmd_type(node);
        node->result_datatype = datatype;
    }

    if (node->type == AST_LIT_INT || node->type == AST_LIT_CHAR || node->type == AST_LIT_REAL)
    {
        node->result_datatype = ast_type_to_datatype(node->type);
    }

    if (node->type == AST_IDENTIFIER)
    {
        if (node->symbol) {
            if (node->symbol->is_function) {
                fprintf(stderr, "Semantic error: function %s used as value at line %d (should be called instead)\n", node->symbol->text, node->line_number);
                ++SemanticErrors;
            }

            if (node->symbol->is_vector && node->son[0] == NULL) {
                fprintf(stderr, "Semantic error: vector %s used as value at line %d (should be indexed instead)\n", node->symbol->text, node->line_number);
                ++SemanticErrors;
            }
        }
        node->result_datatype = expression_typecheck(node);
    }

    switch (node->type)
    {
    case AST_FUNC_CALL:
    {
        if (node->symbol && !node->symbol->is_function)
        {
            fprintf(stderr, "Semantic error: tried to call %s which is not a function at line %d\n", node->symbol->text, node->line_number);
            ++SemanticErrors;
        }
        break;
    }
    case AST_VEC_ACCESS:
    {
        if (node->symbol && !node->symbol->is_vector)
        {
            fprintf(stderr, "Semantic error: tried to index %s which is not a vector at line %d\n", node->symbol->text, node->line_number);
            ++SemanticErrors;
        }

        if (node->son[0]->type == AST_IDENTIFIER && node->son[0]->symbol) {
            if (node->son[0]->symbol->is_function) {
                fprintf(stderr, "Semantic error: cannot use function %s as vector index at line %d\n", node->son[0]->symbol->text, node->line_number);
                ++SemanticErrors;
            }
            if (node->son[0]->symbol->is_vector) {
                fprintf(stderr, "Semantic error: cannot use vector %s as vector index at line %d\n", node->son[0]->symbol->text, node->line_number);
                ++SemanticErrors;
            }
        }

        if (node->son[0]->symbol && node->son[0]->symbol->datatype == 0) {
            node->son[0]->symbol->datatype = expression_typecheck(node->son[0]);
        } else if (node->son[0]->result_datatype == 0) {
            node->son[0]->result_datatype = expression_typecheck(node->son[0]);
        }

        if ((node->son[0]->symbol && node->son[0]->symbol->datatype != DATATYPE_INT) || node->son[0]->result_datatype != DATATYPE_INT) {
            fprintf(stderr, "Semantic error: tried to index %s with non-integer expression at line %d\n", node->symbol->text, node->line_number);
            ++SemanticErrors;
        }

        break;
    }
    case AST_NESTED_EXPR:
    {
        int datatype = expression_typecheck(node);
        node->result_datatype = datatype;
        break;
    }

    case AST_MUL:
    case AST_DIV:
    case AST_ADD:
    case AST_SUB:
    case AST_LE:
    case AST_GE:
    case AST_EQ:
    case AST_DIF:
    case AST_GT:
    case AST_LT:
    {
        debug_printf("node->type: %s\n", ast_type_str(node->type));
        AST *left_operand = node->son[0];
        AST *right_operand = node->son[1];

        if (left_operand->symbol != NULL && left_operand->symbol->is_vector && left_operand->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid left operand (vector should be indexed) at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (right_operand->symbol != NULL && right_operand->symbol->is_vector && right_operand->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid right operand (vector should be indexed) at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (left_operand->symbol != NULL && left_operand->symbol->is_function && left_operand->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid left operand (function should be called) at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (right_operand->symbol != NULL && right_operand->symbol->is_function && right_operand->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid right operand (function should be called) at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        int errored = 0;

        if (!is_bool(left_operand) && !(left_operand->type == AST_NESTED_EXPR) && !(left_operand->type == AST_NEG) && !is_numeric(left_operand) && !is_arithmetic(left_operand) && !is_input_cmd(left_operand))
        {

            errored = 1;
            fprintf(stderr, "Semantic error: invalid left operand at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (!(is_bool(right_operand)) && !(right_operand->type == AST_NESTED_EXPR) && !(right_operand->type == AST_NEG) && !is_numeric(right_operand) && !is_arithmetic(right_operand) && !is_input_cmd(right_operand))
        {
            errored = 1;
            fprintf(stderr, "Semantic error: invalid right operand at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (!errored)
        {
            int left_datatype = 0;
            int right_datatype = 0;

            if (left_operand->symbol)
            {
                left_datatype = left_operand->symbol->datatype;
            }

            if (right_operand->symbol)
            {
                right_datatype = right_operand->symbol->datatype;
            }

            if (left_operand->type == AST_NESTED_EXPR || is_input_cmd(left_operand) || left_operand->type == AST_NEG ||
                is_arithmetic(left_operand))
            {
                left_datatype = expression_typecheck(left_operand);
            }

            if (right_operand->type == AST_NESTED_EXPR || is_input_cmd(right_operand) || right_operand->type == AST_NEG ||
                is_arithmetic(right_operand))
            {
                right_datatype = expression_typecheck(right_operand);
            }

            if ((left_operand->symbol || is_input_cmd(left_operand) || left_operand->type == AST_NEG) &&
                (right_operand->symbol || is_input_cmd(right_operand) || left_operand->type == AST_NEG) && left_datatype != right_datatype &&
                !compare_datatypes(left_datatype, right_datatype))
            {

                fprintf(stderr, "Semantic error: operands should have same type at line %d\n", node->line_number);
                ++SemanticErrors;
            }
            else
            {
                if (left_datatype != right_datatype && !compare_datatypes(left_datatype, right_datatype))
                {
                    fprintf(stderr, "Semantic error: operands should have same type at line %d\n", node->line_number);
                    ++SemanticErrors;
                }
            }

            if (is_arithmetic(node) && !is_arithmetic(left_operand) && !is_arithmetic(right_operand) && !is_numeric(left_operand) && !is_numeric(right_operand))
            {
                fprintf(stderr, "Node ID: %d\n", node->id);
                fprintf(stderr, "Semantic error: operands should be arithmetic at line %d\n", node->line_number);
                ++SemanticErrors;
            }

            errored = 1;
        }

        if (!errored && !expression_typecheck(node))
        {
            fprintf(stderr, "Semantic error: invalid resulting expression type for %s at line %d\n", ast_type_str(node->type), node->line_number);
            ++SemanticErrors;
        }
        else
        {
            node->result_datatype = expression_typecheck(node);
        }

        break;
    }

    case AST_NEG:
    {

        AST *operand = node->son[0];

        if (operand->symbol && operand->symbol->is_vector && operand->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid unary operand (vector should be indexed) at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (operand->symbol && operand->symbol->is_function && operand->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid unary operand (function should be called) at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (operand->type != AST_NESTED_EXPR && !is_numeric(operand) && !is_arithmetic(operand) && !is_input_cmd(operand))
        {
            fprintf(stderr, "Semantic error: invalid unary arithmetic/numeric operand at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (!expression_typecheck(node))
        {
            fprintf(stderr, "Semantic error: invalid resulting expression type for %s at line %d\n", ast_type_str(node->type), node->line_number);
            ++SemanticErrors;
        }
        else
        {
            node->result_datatype = expression_typecheck(node);
            if (node->result_datatype == DATATYPE_BOOL)
            {
                fprintf(stderr, "Semantic error: invalid resulting expression type for %s (got bool, expected numeric-compatible type) at line %d\n", ast_type_str(node->type), node->line_number);
                ++SemanticErrors;
            }
        }
        break;
    }

    case AST_AND:
    case AST_OR:
    {
        AST *left_operand = node->son[0];
        AST *right_operand = node->son[1];

        if (left_operand->symbol && left_operand->symbol->is_vector && left_operand->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid left operand (vector should be indexed) at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (right_operand->symbol && right_operand->symbol->is_vector && right_operand->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid right operand (vector should be indexed) at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (left_operand->symbol && left_operand->symbol->is_function && left_operand->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid left operand (function should be called) at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (right_operand->symbol && right_operand->symbol->is_function && right_operand->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid right operand (function should be called) at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        int errored = 0;

        if (!is_bool(left_operand) && !(left_operand->type == AST_NESTED_EXPR) && !is_logic(left_operand) && !is_input_cmd(left_operand))
        {
            fprintf(stderr, "Semantic error: invalid left operand for %s at line %d\n", ast_type_str(node->type), node->line_number);
            ++SemanticErrors;
        }

        if (!is_bool(right_operand) && !(right_operand->type == AST_NESTED_EXPR) && !is_logic(right_operand) && !is_input_cmd(right_operand))
        {
            fprintf(stderr, "Semantic error: invalid right operand for %s at line %d\n", ast_type_str(node->type), node->line_number);
            ++SemanticErrors;
        }

        if (!errored)
        {
            int left_datatype = 0;
            int right_datatype = 0;

            if (left_operand->symbol)
            {
                left_datatype = left_operand->symbol->datatype;
            }

            if (right_operand->symbol)
            {
                right_datatype = right_operand->symbol->datatype;
            }

            if (left_operand->type == AST_NESTED_EXPR || is_input_cmd(left_operand))
            {
                left_datatype = expression_typecheck(left_operand);
            }

            if (right_operand->type == AST_NESTED_EXPR || is_input_cmd(right_operand))
            {
                right_datatype = expression_typecheck(right_operand);
            }

            if ((left_operand->symbol || is_input_cmd(left_operand)) && (right_operand->symbol || is_input_cmd(right_operand)) && left_datatype != right_datatype)
            {
                fprintf(stderr, "Semantic error: operands should have same type at line %d\n", node->line_number);
                ++SemanticErrors;
            }
        }

        if (!expression_typecheck(node))
        {
            fprintf(stderr, "Semantic error: invalid resulting expression type for %s at line %d\n", ast_type_str(node->type), node->line_number);
            ++SemanticErrors;
        }
        else
        {
            node->result_datatype = expression_typecheck(node);
        }

        break;
    }

    case AST_NOT:
    {
        AST *operand = node->son[0];

        if (operand->symbol && operand->symbol->is_vector && operand->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid unary logical operand (vector should be indexed) at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (operand->symbol && operand->symbol->is_function && operand->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid unary logical operand (function should be called) at line %d\n", node->line_number);
            ++SemanticErrors;
        }

        if (operand->type != AST_NESTED_EXPR && !is_logic(operand) && !is_bool(operand) && !is_input_cmd(operand))
        {
            if (operand->symbol)
            {
                fprintf(stderr, "Semantic error: invalid unary logical operand (%s) at line %d\n", datatype_str[operand->symbol->datatype], node->line_number);
                ++SemanticErrors;
            }
            else
            {
                fprintf(stderr, "Semantic error: invalid unary logical operand (%s) at line %d\n", datatype_str[get_input_cmd_type(operand)], node->line_number);
                ++SemanticErrors;
            }
        }

        if (!expression_typecheck(node))
        {
            fprintf(stderr, "Semantic error: invalid resulting expression type for %s at line %d\n", ast_type_str(node->type), node->line_number);
            ++SemanticErrors;
        }
        else
        {
            node->result_datatype = expression_typecheck(node);
        }

        break;
    }
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        check_operands(node->son[i]);
    }
}

int expression_typecheck(AST *node)
{
    int i;

    if (!node)
        return 1;

    if (node->type == AST_NESTED_EXPR)
    {
        return expression_typecheck(node->son[0]);
    }

    if (node->type == AST_IDENTIFIER || node->type == AST_VEC_ACCESS || node->type == AST_FUNC_CALL ||
        node->type == AST_LIT_INT || node->type == AST_LIT_REAL || node->type == AST_LIT_CHAR)
    {
        if (node->symbol)
        {
            node->typechecked = 1;

            if (node->symbol->datatype == DATATYPE_CHAR)
            {
                return DATATYPE_INT;
            }

            if (node->type == AST_LIT_INT)
            {
                return DATATYPE_INT;
            }
            if (node->type == AST_LIT_REAL)
            {
                return DATATYPE_REAL;
            }
            if (node->type == AST_LIT_CHAR)
            {
                return DATATYPE_CHAR;
            }

            return node->symbol->datatype;
        }
    }

    if (is_logic(node))
    {
        if (is_binary(node))
        {
            node->typechecked = 1;

            int expr1_ty = expression_typecheck(node->son[0]);
            int expr2_ty = expression_typecheck(node->son[1]);

            return (((node->son[0]->type == AST_NESTED_EXPR || is_logic(node->son[0])) && expression_typecheck(node->son[1])) ||
                    ((node->son[1]->type == AST_NESTED_EXPR || is_logic(node->son[1])) && expression_typecheck(node->son[0])) ||
                    (expr1_ty == expr2_ty))
                       ? DATATYPE_BOOL
                       : 0;
        }

        if (is_unary(node))
        {
            node->typechecked = 1;
            return (is_logic(node->son[0]) && expression_typecheck(node->son[0])) ||
                           (expression_typecheck(node->son[0]))
                       ? DATATYPE_BOOL
                       : 0;
        }
    }

    if (is_binary(node))
    {
        int expr1_ty = expression_typecheck(node->son[0]);
        int expr2_ty = expression_typecheck(node->son[1]);

        node->typechecked = 1;
        return (expr1_ty == expr2_ty) ? expr1_ty : 0;
    }

    if (is_unary(node))
    {
        node->typechecked = 1;
        int expr_ty = expression_typecheck(node->son[0]);
        return expression_typecheck(node->son[0]) ? expr_ty : 0;
    }

    if (is_input_cmd(node))
    {
        node->typechecked = 1;
        return get_input_cmd_type(node);
    }

    return 0;
}

int find_first_datatype(AST *node)
{
    int i;

    if (!node)
        return 0;

    if (is_logic(node) && node->son[0] && node->son[0]->type != AST_NESTED_EXPR)
    {
        return DATATYPE_BOOL;
    }

    if (node->type == AST_NESTED_EXPR)
    {
        return expression_typecheck(node->son[0]);
    }

    if (node->type == AST_IDENTIFIER || node->type == AST_VEC_ACCESS || node->type == AST_FUNC_CALL ||
        node->type == AST_LIT_INT || node->type == AST_LIT_REAL || node->type == AST_LIT_CHAR)
    {
        if (node->symbol)
        {
            return node->symbol->datatype;
        }
    }

    if (is_binary(node))
    {
        return expression_typecheck(node->son[0]);
    }

    if (is_unary(node))
    {
        return expression_typecheck(node->son[0]);
    }

    if (is_input_cmd(node))
    {
        return get_input_cmd_type(node);
    }

    return 0;
}

int check_assignments(AST *node)
{
    int i;

    if (!node)
        return 0;

    if (node->type == AST_VAR_ATTRIB)
    {
        if (node->symbol && node->symbol->is_function)
        {
            fprintf(stderr, "Semantic error: invalid assignment to function %s at line %d\n", node->symbol->text, node->line_number);
            ++SemanticErrors;
        }

        int expected_datatype = node->symbol->datatype;
        int resulting_datatype = expression_typecheck(node->son[0]);

        if (node->son[0]->type == AST_IDENTIFIER)
        {
            if (node->symbol && node->symbol->is_vector && node->son[0]->symbol && !node->son[0]->symbol->is_vector)
            {
                fprintf(stderr, "Semantic error: invalid assignment of scalar/function to vector at line %d\n", node->line_number);
                ++SemanticErrors;
            }
            else
            {
                if (node->son[0]->symbol->is_vector && !node->symbol->is_vector && !node->symbol->is_function)
                {
                    fprintf(stderr, "Semantic error: invalid assignment of vector to scalar at line %d\n", node->line_number);
                    ++SemanticErrors;
                }
                else if (node->son[0]->symbol->is_function && !node->symbol->is_vector && !node->symbol->is_function)
                {
                    fprintf(stderr, "Semantic error: invalid assignment of function %s to scalar %s at line %d\n",
                            node->symbol->text, node->son[0]->symbol->text, node->line_number);
                    ++SemanticErrors;
                }
                else if (node->son[0]->symbol->is_function && node->symbol->is_vector)
                {
                    fprintf(stderr, "Semantic error: invalid assignment of function %s to vector %s at line %d\n",
                            node->symbol->text, node->son[0]->symbol->text, node->line_number);
                    ++SemanticErrors;
                }
            }
        }
        else if (node->symbol && node->symbol->is_vector)
        {
            fprintf(stderr, "Semantic error: invalid assignment of expression to vector %s at line %d\n", node->symbol->text, node->line_number);
            ++SemanticErrors;
        }

        if (expected_datatype != resulting_datatype && resulting_datatype != 0 && !compare_datatypes(expected_datatype, resulting_datatype))
        {
            fprintf(stderr, "Semantic error: invalid assignment of %s to %s at line %d\n", datatype_str[resulting_datatype], datatype_str[expected_datatype], node->line_number);
            ++SemanticErrors;
        }
        else if (resulting_datatype == 0)
        {
            if (node->symbol && node->son[0]->symbol &&
                !compare_datatypes(node->symbol->datatype, node->son[0]->symbol->datatype))
            {
                fprintf(stderr, "Semantic error: invalid assignment of %s to %s at line %d\n", datatype_str[node->son[0]->symbol->datatype], datatype_str[node->symbol->datatype], node->line_number);
                ++SemanticErrors;
            }
        }
        else
        {
        }
    }

    if (node->type == AST_VEC_ATTRIB)
    {
        AST *vec_indexer = node->son[0];

        if (vec_indexer->type != AST_LIT_INT && vec_indexer->type != AST_LIT_CHAR)
        {
            if (vec_indexer->type == AST_FUNC_CALL)
            {
                int func_datatype = vec_indexer->symbol->datatype;
                if (func_datatype != DATATYPE_INT && func_datatype != DATATYPE_CHAR)
                {
                    fprintf(stderr, "Semantic error: invalid vector indexer type (expected int or char, got %s -> %s) at line %d\n",
                            ast_type_str(vec_indexer->type), datatype_str[func_datatype], node->line_number);
                    ++SemanticErrors;
                }
            }
            else
            {
                int resulting_type = expression_typecheck(vec_indexer);
                vec_indexer->result_datatype = resulting_type;
                int vec_indexer_result_type = vec_indexer->result_datatype;
                if (vec_indexer_result_type != DATATYPE_INT && vec_indexer_result_type != DATATYPE_CHAR)
                {
                    fprintf(stderr, "Semantic error: invalid vector indexer type (expected int or char, got %s) at line %d\n", datatype_str[vec_indexer_result_type], node->line_number);
                    ++SemanticErrors;
                }
            }
        }

        int expected_datatype = node->symbol->datatype;
        int resulting_datatype = node->son[1]->result_datatype;

        if (node->son[1]->type == AST_IDENTIFIER)
        {
            if (node->son[1]->symbol->is_vector && node->son[1]->type != AST_VEC_ACCESS)
            {
                fprintf(stderr, "Semantic error: invalid assignment of vector to vector index at line %d\n", node->line_number);
                ++SemanticErrors;
            }

            if (node->son[1]->symbol->is_function && node->son[1]->type != AST_FUNC_CALL)
            {
                fprintf(stderr, "Semantic error: invalid assignment of function to vector index at line %d\n", node->line_number);
                ++SemanticErrors;
            }
        }

        if (expected_datatype != resulting_datatype && resulting_datatype != 0)
        {
            fprintf(stderr, "Semantic error: invalid assignment of %s to %s[] at line %d\n", datatype_str[resulting_datatype], datatype_str[expected_datatype], node->line_number);
            ++SemanticErrors;
        }
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        check_assignments(node->son[i]);
    }
}

void check_return(AST *node)
{
    int i;
    int required_vec_type = 0;
    if (!node)
        return;

    switch (node->type)
    {
    case AST_FUNC_DECL_INT:
    case AST_FUNC_DECL_CHAR:
    case AST_FUNC_DECL_REAL:
    case AST_FUNC_DECL_BOOL:
    {
        if (node->symbol)
        {

            if (!check_return_aux(node, node->symbol->datatype))
            {
                fprintf(stderr, "Semantic error: function %s is missing return statement at line %d\n", node->symbol->text, node->line_number);
                ++SemanticErrors;
            }

            AST *param = node->son[0];
            if (param->type == AST_EMPTY_PARAM_LIST)
            {
                break;
            }

            int count = 0;
            while (param)
            {

                node->symbol->params[count] = ast_type_to_datatype(param->son[0]->type);
                param = param->son[1];
                ++count;
            }

            node->symbol->param_count = count;
        }

        break;
    }
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        check_return(node->son[i]);
    }
}

int check_return_aux(AST *node, int required_datatype)
{
    int i;

    if (!node)
        return 0;

    int found = 0;

    if (node->type == AST_RETURN_CMD)
    {
        int return_datatype = node->son[0]->result_datatype;

        if (node->son[0]->symbol && node->son[0]->symbol->is_vector && node->son[0]->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid return type (expected %s, got vector) at line %d\n", datatype_str[required_datatype], node->line_number);
            ++SemanticErrors;
        }

        if (node->son[0]->symbol && node->son[0]->symbol->is_function && node->son[0]->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid return type (expected %s, got function) at line %d\n", datatype_str[required_datatype], node->line_number);
            ++SemanticErrors;
        }

        if (return_datatype != required_datatype && !validate_return_type(required_datatype, node->son[0]))
        {
            if (return_datatype != 0)
            {
                fprintf(stderr, "Semantic error: invalid return type (expected %s, got %s) at line %d\n", datatype_str[required_datatype], datatype_str[return_datatype], node->line_number);
            }
            else
            {
                fprintf(stderr, "Semantic error: invalid return type (expected %s, got incompatible type) at line %d\n", datatype_str[required_datatype], node->line_number);
            }

            ++SemanticErrors;
        }

        if (node->son[0]->type == AST_NESTED_EXPR)
        {
            AST *first_nested_expr_item = node->son[0]->son[0];
            if (first_nested_expr_item->type == AST_IDENTIFIER &&
                (first_nested_expr_item->symbol->is_vector || first_nested_expr_item->symbol->is_function))
            {
                fprintf(stderr, "Semantic error: invalid return type (expected %s, got %s %s) at line %d\n", 
                datatype_str[required_datatype], 
                datatype_str[return_datatype], 
                first_nested_expr_item->symbol->is_vector ? "vector" : "function",
                node->line_number);
                ++SemanticErrors;
            }
        }

        found = 1;
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        found = found | check_return_aux(node->son[i], required_datatype);
    }

    return found;
}

void check_function_call(AST *node)
{
    int i;

    if (!node)
        return;

    if (node->type == AST_FUNC_CALL)
    {
        if (node->symbol->type == SYMBOL_IDENTIFIER)
        {
            // fprintf(stderr, "Semantic error: call to undeclared function %s at line %d\n", node->symbol->text, node->line_number);
            //++SemanticErrors;
            return;
        }
       
        int parameter_count = 0;
        int found = 0;
        int error_in_number_of_params = 0;
        AST *expr_list = node->son[0];
        AST *expr_list_copy = expr_list;
        if (expr_list)
        {
            AST *expr = expr_list->son[0];
            int index = 0;
            while (expr)
            {
                int expected_datatype = node->symbol->params[index];
                int actual_datatype;

                if (expr->result_datatype != 0)
                {
                    actual_datatype = expr->result_datatype;
                }
                else
                {
                    if (expr->symbol)
                    {
                        actual_datatype = expr->symbol->datatype;
                    }
                    else
                    {
                        // it is an input(type)
                        switch (expr->type)
                        {
                        case AST_INPUT_EXPR_INT:
                            actual_datatype = DATATYPE_INT;
                            break;
                        case AST_INPUT_EXPR_CHAR:
                            actual_datatype = DATATYPE_CHAR;
                            break;
                        case AST_INPUT_EXPR_REAL:
                            actual_datatype = DATATYPE_REAL;
                            break;
                        case AST_INPUT_EXPR_BOOL:
                            actual_datatype = DATATYPE_BOOL;
                            break;
                        }
                    }
                }

                if (!compare_datatypes(expected_datatype, actual_datatype) && expected_datatype != 0)
                {
                    fprintf(stderr, "Semantic error: invalid parameter type (expected %s, got %s) at line %d\n",
                            datatype_str[expected_datatype], datatype_str[actual_datatype], node->line_number);
                    ++SemanticErrors;
                }

                index++;
                if (expr_list->son[1] != NULL)
                {
                    expr_list = expr_list->son[1];
                    if (expr_list)
                        expr = expr_list->son[0];
                }
                else
                {
                    expr = NULL;
                }
            }

            expr = expr_list_copy->son[0];

            while (expr)
            {
                parameter_count++;
                if (expr_list_copy->son[1] != NULL)
                {
                    expr_list_copy = expr_list_copy->son[1];
                    if (expr_list_copy)
                        expr = expr_list_copy->son[0];
                }
                else
                {
                    expr = NULL;
                }
            }
        }

        

        if (parameter_count != node->symbol->param_count)
        {
            if (parameter_count == 0)
            {
                fprintf(stderr, "Semantic error: invalid number of parameters (expected %d, got none) at line %d\n", node->symbol->param_count, node->line_number);
            }
            else
            {
                fprintf(stderr, "Semantic error: invalid number of parameters (expected %d, got %d) at line %d\n", node->symbol->param_count, parameter_count, node->line_number);
            }

            ++SemanticErrors;
        }
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        check_function_call(node->son[i]);
    }
}

void check_conditional_stmts(AST *node)
{
    int i;

    if (!node)
        return;

    if (node->type == AST_IF || node->type == AST_IF_ELSE || node->type == AST_LOOP)
    {
        if (node->son[0]->result_datatype != DATATYPE_BOOL && (node->son[0]->symbol && node->son[0]->symbol->datatype != DATATYPE_BOOL))
        {
            fprintf(stderr, "Semantic error: invalid conditional statement (expected bool, got %s) at line %d\n", datatype_str[node->son[0]->result_datatype], node->line_number);
            ++SemanticErrors;
        }
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        check_conditional_stmts(node->son[i]);
    }
}