#include "sa_oparray.h"
#include "zend_alloc.h"
#include "ext/standard/url.h"
#include "set.h"
#include "php_vld.h"
extern int vld_find_jumps(zend_op_array *opa, unsigned int position, size_t *jump_count, int *jumps);
ZEND_EXTERN_MODULE_GLOBALS(vld)

#define NUM_KNOWN_OPCODES (sizeof(opcodes) / sizeof(opcodes[0]))
#define vld_cJSON_Cancel(O)                         \
    cJSON_Delete(O);                                \
    fprintf(stderr, "%s:%d\n", __FILE__, __LINE__); \
    return NULL;

/* Input zend_compile.h
 * And replace [^(...)(#define )([^ \t]+).*$]
 * BY     [/=*  \1 *=/  { "\3", ALL_USED },] REMEMBER to remove the two '=' signs
 */
static const op_usage opcodes[] = {
    /*  0 */ {"NOP", NONE_USED},
    /*  1 */ {"ADD", ALL_USED},
    /*  2 */ {"SUB", ALL_USED},
    /*  3 */ {"MUL", ALL_USED},
    /*  4 */ {"DIV", ALL_USED},
    /*  5 */ {"MOD", ALL_USED},
    /*  6 */ {"SL", ALL_USED},
    /*  7 */ {"SR", ALL_USED},
    /*  8 */ {"CONCAT", ALL_USED},
    /*  9 */ {"BW_OR", ALL_USED},
    /*  10 */ {"BW_AND", ALL_USED},
    /*  11 */ {"BW_XOR", ALL_USED},
    /*  12 */ {"BW_NOT", RES_USED | OP1_USED},
    /*  13 */ {"BOOL_NOT", RES_USED | OP1_USED},
    /*  14 */ {"BOOL_XOR", ALL_USED},
    /*  15 */ {"IS_IDENTICAL", ALL_USED},
    /*  16 */ {"IS_NOT_IDENTICAL", ALL_USED},
    /*  17 */ {"IS_EQUAL", ALL_USED},
    /*  18 */ {"IS_NOT_EQUAL", ALL_USED},
    /*  19 */ {"IS_SMALLER", ALL_USED},
    /*  20 */ {"IS_SMALLER_OR_EQUAL", ALL_USED},
    /*  21 */ {"CAST", ALL_USED | EXT_VAL},
#if PHP_VERSION_ID >= 70400
    /*  22 */ {"ASSIGN", ALL_USED},
    /*  23 */ {"ASSIGN_DIM", ALL_USED},
    /*  24 */ {"ASSIGN_OBJ", ALL_USED},
    /*  25 */ {"ASSIGN_STATIC_PROP", ALL_USED},
    /*  26 */ {"ASSIGN_OP", ALL_USED | EXT_VAL},
    /*  27 */ {"ASSIGN_DIM_OP", ALL_USED | EXT_VAL},
    /*  28 */ {"ASSIGN_OBJ_OP", ALL_USED | EXT_VAL},
    /*  29 */ {"ASSIGN_STATIC_PROP_OP", ALL_USED | EXT_VAL},
    /*  30 */ {"ASSIGN_REF", SPECIAL},
    /*  31 */ {"QM_ASSIGN", RES_USED | OP1_USED},
    /*  32 */ {"ASSIGN_OBJ_REF", ALL_USED},
    /*  33 */ {"ASSIGN_STATIC_PROP_REF", ALL_USED},
#else
    /*  22 */ {"QM_ASSIGN", RES_USED | OP1_USED},
    /*  23 */ {"ASSIGN_ADD", ALL_USED | EXT_VAL},
    /*  24 */ {"ASSIGN_SUB", ALL_USED | EXT_VAL},
    /*  25 */ {"ASSIGN_MUL", ALL_USED | EXT_VAL},
    /*  26 */ {"ASSIGN_DIV", ALL_USED | EXT_VAL},
    /*  27 */ {"ASSIGN_MOD", ALL_USED | EXT_VAL},
    /*  28 */ {"ASSIGN_SL", ALL_USED | EXT_VAL},
    /*  29 */ {"ASSIGN_SR", ALL_USED | EXT_VAL},
    /*  30 */ {"ASSIGN_CONCAT", ALL_USED | EXT_VAL},
    /*  31 */ {"ASSIGN_BW_OR", ALL_USED | EXT_VAL},
    /*  32 */ {"ASSIGN_BW_AND", ALL_USED | EXT_VAL},
    /*  33 */ {"ASSIGN_BW_XOR", ALL_USED | EXT_VAL},
#endif
    /*  34 */ {"PRE_INC", OP1_USED | RES_USED},
    /*  35 */ {"PRE_DEC", OP1_USED | RES_USED},
    /*  36 */ {"POST_INC", OP1_USED | RES_USED},
    /*  37 */ {"POST_DEC", OP1_USED | RES_USED},
#if PHP_VERSION_ID >= 70400
    /*  38 */ {"PRE_INC_STATIC_PROP", ALL_USED},
    /*  39 */ {"PRE_DEC_STATIC_PROP", ALL_USED},
    /*  40 */ {"POST_INC_STATIC_PROP", ALL_USED},
    /*  41 */ {"POST_DEC_STATIC_PROP", ALL_USED},
#else
    /*  38 */ {"ASSIGN", ALL_USED},
    /*  39 */ {"ASSIGN_REF", SPECIAL},
    /*  40 */ {"ECHO", OP1_USED},
#if PHP_VERSION_ID < 70100
    /*  41 */ {"UNKNOWN [41]", ALL_USED},
#else
    /*  41 */ {"GENERATOR_CREATE", RES_USED},
#endif
#endif
    /*  42 */ {"JMP", OP1_USED | OP1_OPLINE},
    /*  43 */ {"JMPZ", OP1_USED | OP2_USED | OP2_OPLINE},
    /*  44 */ {"JMPNZ", OP1_USED | OP2_USED | OP2_OPLINE},
    /*  45 */ {"JMPZNZ", SPECIAL},
    /*  46 */ {"JMPZ_EX", ALL_USED | OP2_OPLINE},
    /*  47 */ {"JMPNZ_EX", ALL_USED | OP2_OPLINE},
    /*  48 */ {"CASE", ALL_USED},
#if PHP_VERSION_ID < 70200
    /*  49 */ {"SWITCH_FREE", RES_USED | OP1_USED},
    /*  50 */ {"BRK", SPECIAL},
    /*  51 */ {"CONT", ALL_USED},
#else
    /*  49 */ {"CHECK_VAR", OP1_USED},
    /*  50 */ {"SEND_VAR_NO_REF_EX", ALL_USED},
    /*  51 */ {"MAKE_REF", RES_USED | OP1_USED},
#endif
    /*  52 */ {"BOOL", RES_USED | OP1_USED},
    /*  53 */ {"FAST_CONCAT", ALL_USED},
    /*  54 */ {"ROPE_INIT", ALL_USED | EXT_VAL},
    /*  55 */ {"ROPE_ADD", ALL_USED | EXT_VAL},
    /*  56 */ {"ROPE_END", ALL_USED | EXT_VAL},
    /*  57 */ {"BEGIN_SILENCE", ALL_USED},
    /*  58 */ {"END_SILENCE", ALL_USED},
    /*  59 */ {"INIT_FCALL_BY_NAME", SPECIAL},
    /*  60 */ {"DO_FCALL", SPECIAL},
    /*  61 */ {"INIT_FCALL", ALL_USED},
    /*  62 */ {"RETURN", OP1_USED},
    /*  63 */ {"RECV", RES_USED | OP1_USED},
    /*  64 */ {"RECV_INIT", ALL_USED},
    /*  65 */ {"SEND_VAL", OP1_USED},
    /*  66 */ {"SEND_VAR_EX", ALL_USED},
    /*  67 */ {"SEND_REF", ALL_USED},
    /*  68 */ {"NEW", SPECIAL},
    /*  69 */ {"INIT_NS_FCALL_BY_NAME", SPECIAL},
    /*  70 */ {"FREE", OP1_USED},
    /*  71 */ {"INIT_ARRAY", ALL_USED},
    /*  72 */ {"ADD_ARRAY_ELEMENT", ALL_USED},
    /*  73 */ {"INCLUDE_OR_EVAL", ALL_USED | OP2_INCLUDE},
    /*  74 */ {"UNSET_VAR", ALL_USED},
    /*  75 */ {"UNSET_DIM", ALL_USED},
    /*  76 */ {"UNSET_OBJ", ALL_USED},
    /*  77 */ {"FE_RESET_R", SPECIAL},
    /*  78 */ {"FE_FETCH_R", ALL_USED | EXT_VAL_JMP_REL},
    /*  79 */ {"EXIT", ALL_USED},
    /*  80 */ {"FETCH_R", RES_USED | OP1_USED | OP_FETCH},
    /*  81 */ {"FETCH_DIM_R", ALL_USED},
    /*  82 */ {"FETCH_OBJ_R", ALL_USED},
    /*  83 */ {"FETCH_W", RES_USED | OP1_USED | OP_FETCH},
    /*  84 */ {"FETCH_DIM_W", ALL_USED},
    /*  85 */ {"FETCH_OBJ_W", ALL_USED},
    /*  86 */ {"FETCH_RW", RES_USED | OP1_USED | OP_FETCH},
    /*  87 */ {"FETCH_DIM_RW", ALL_USED},
    /*  88 */ {"FETCH_OBJ_RW", ALL_USED},
    /*  89 */ {"FETCH_IS", ALL_USED},
    /*  90 */ {"FETCH_DIM_IS", ALL_USED},
    /*  91 */ {"FETCH_OBJ_IS", ALL_USED},
    /*  92 */ {"FETCH_FUNC_ARG", RES_USED | OP1_USED | OP_FETCH},
    /*  93 */ {"FETCH_DIM_FUNC_ARG", ALL_USED},
    /*  94 */ {"FETCH_OBJ_FUNC_ARG", ALL_USED},
    /*  95 */ {"FETCH_UNSET", ALL_USED},
    /*  96 */ {"FETCH_DIM_UNSET", ALL_USED},
    /*  97 */ {"FETCH_OBJ_UNSET", ALL_USED},
#if PHP_VERSION_ID >= 70300
    /*  98 */ {"FETCH_LIST_R", ALL_USED},
#else
    /*  98 */ {"FETCH_LIST", ALL_USED},
#endif
    /*  99 */ {"FETCH_CONSTANT", ALL_USED},

#if PHP_VERSION_ID >= 70300
    /*  100 */ {"CHECK_FUNC_ARG", ALL_USED},
#else
    /*  100 */ {"UNKNOWN [100]", ALL_USED},
#endif

    /*  101 */ {"EXT_STMT", ALL_USED},
    /*  102 */ {"EXT_FCALL_BEGIN", ALL_USED},
    /*  103 */ {"EXT_FCALL_END", ALL_USED},
    /*  104 */ {"EXT_NOP", ALL_USED},
    /*  105 */ {"TICKS", ALL_USED},
    /*  106 */ {"SEND_VAR_NO_REF", ALL_USED | EXT_VAL},
    /*  107 */ {"CATCH", SPECIAL},
    /*  108 */ {"THROW", ALL_USED | EXT_VAL},
    /*  109 */ {"FETCH_CLASS", SPECIAL},
    /*  110 */ {"CLONE", ALL_USED},
    /*  111 */ {"RETURN_BY_REF", OP1_USED},
    /*  112 */ {"INIT_METHOD_CALL", ALL_USED},
    /*  113 */ {"INIT_STATIC_METHOD_CALL", ALL_USED},
    /*  114 */ {"ISSET_ISEMPTY_VAR", ALL_USED | EXT_VAL},
    /*  115 */ {"ISSET_ISEMPTY_DIM_OBJ", ALL_USED | EXT_VAL},

    /*  116 */ {"SEND_VAL_EX", ALL_USED},
    /*  117 */ {"SEND_VAR", ALL_USED},

    /*  118 */ {"INIT_USER_CALL", ALL_USED | EXT_VAL},
    /*  119 */ {"SEND_ARRAY", ALL_USED},
    /*  120 */ {"SEND_USER", ALL_USED},

    /*  121 */ {"STRLEN", ALL_USED},
    /*  122 */ {"DEFINED", ALL_USED},
    /*  123 */ {"TYPE_CHECK", ALL_USED | EXT_VAL},
    /*  124 */ {"VERIFY_RETURN_TYPE", ALL_USED},
    /*  125 */ {"FE_RESET_RW", SPECIAL},
    /*  126 */ {"FE_FETCH_RW", ALL_USED | EXT_VAL_JMP_REL},
    /*  127 */ {"FE_FREE", ALL_USED},
    /*  128 */ {"INIT_DYNAMIC_CALL", ALL_USED},
    /*  129 */ {"DO_ICALL", ALL_USED},
    /*  130 */ {"DO_UCALL", ALL_USED},
    /*  131 */ {"DO_FCALL_BY_NAME", SPECIAL},

    /*  132 */ {"PRE_INC_OBJ", ALL_USED},
    /*  133 */ {"PRE_DEC_OBJ", ALL_USED},
    /*  134 */ {"POST_INC_OBJ", ALL_USED},
    /*  135 */ {"POST_DEC_OBJ", ALL_USED},
#if PHP_VERSION_ID >= 70400
    /*  136 */ {"ECHO", OP1_USED},
#else
    /*  136 */ {"ASSIGN_OBJ", ALL_USED},
#endif
    /*  137 */ {"OP_DATA", ALL_USED},
    /*  138 */ {"INSTANCEOF", ALL_USED},
    /*  139 */ {"DECLARE_CLASS", ALL_USED},
    /*  140 */ {"DECLARE_INHERITED_CLASS", ALL_USED},
    /*  141 */ {"DECLARE_FUNCTION", ALL_USED},
#if PHP_VERSION_ID >= 70400
    /*  142 */ {"DECLARE_LAMBDA_FUNCTION", OP1_USED},
#else
    /*  142 */ {"RAISE_ABSTRACT_ERROR", ALL_USED},
#endif
    /*  143 */ {"DECLARE_CONST", OP1_USED | OP2_USED},
    /*  144 */ {"ADD_INTERFACE", ALL_USED},
#if PHP_VERSION_ID >= 70400
    /*  145 */ {"DECLARE_CLASS_DELAYED", ALL_USED},
    /*  146 */ {"DECLARE_ANON_CLASS", OP2_USED | RES_USED | RES_CLASS},
    /*  147 */ {"ADD_ARRAY_UNPACK", ALL_USED},
#else
    /*  145 */ {"DECLARE_INHERITED_CLASS_DELAYED", ALL_USED},
    /*  146 */ {"VERIFY_ABSTRACT_CLASS", ALL_USED},
    /*  147 */ {"ASSIGN_DIM", ALL_USED},
#endif
    /*  148 */ {"ISSET_ISEMPTY_PROP_OBJ", ALL_USED},
    /*  149 */ {"HANDLE_EXCEPTION", NONE_USED},
    /*  150 */ {"USER_OPCODE", ALL_USED},
    /*  151 */ {"ASSERT_CHECK", ALL_USED},
    /*  152 */ {"JMP_SET", ALL_USED | OP2_OPLINE},
#if PHP_VERSION_ID >= 70400
    /*  153 */ {"UNSET_CV", ALL_USED},
    /*  154 */ {"ISSET_ISEMPTY_CV", ALL_USED},
    /*  155 */ {"FETCH_LIST_W", ALL_USED},
#else
    /*  153 */ {"DECLARE_LAMBDA_FUNCTION", OP1_USED},
    /*  154 */ {"ADD_TRAIT", ALL_USED},
    /*  155 */ {"BIND_TRAITS", OP1_USED},
#endif
    /*  156 */ {"SEPARATE", OP1_USED | RES_USED},
    /*  157 */ {"FETCH_CLASS_NAME", ALL_USED},
    /*  158 */ {"JMP_SET_VAR", OP1_USED | RES_USED},
    /*  159 */ {"DISCARD_EXCEPTION", NONE_USED},
    /*  160 */ {"YIELD", ALL_USED},
    /*  161 */ {"GENERATOR_RETURN", NONE_USED},
    /*  162 */ {"FAST_CALL", SPECIAL},
    /*  163 */ {"FAST_RET", SPECIAL},
    /*  164 */ {"RECV_VARIADIC", ALL_USED},
    /*  165 */ {"SEND_UNPACK", ALL_USED},
#if PHP_VERSION_ID >= 70400
    /*  166 */ {"YIELD_FROM", ALL_USED},
    /*  167 */ {"COPY_TMP", ALL_USED},
#else
    /*  166 */ {"POW", ALL_USED},
    /*  167 */ {"ASSIGN_POW", ALL_USED},
#endif
    /*  168 */ {"BIND_GLOBAL", ALL_USED},
    /*  169 */ {"COALESCE", ALL_USED},
    /*  170 */ {"SPACESHIP", ALL_USED},
#if PHP_VERSION_ID >= 70100
#if PHP_VERSION_ID >= 70400
    /*  171 */ {"FUNC_NUM_ARGS", ALL_USED},
    /*  172 */ {"FUNC_GET_ARGS", ALL_USED},
#else
    /*  171 */ {"DECLARE_ANON_CLASS", OP2_USED | RES_USED | RES_CLASS},
    /*  172 */ {"DECLARE_ANON_INHERITED_CLASS", OP2_USED | RES_USED | RES_CLASS},
#endif
    /*  173 */ {"FETCH_STATIC_PROP_R", RES_USED | OP1_USED | OP_FETCH},
    /*  174 */ {"FETCH_STATIC_PROP_W", RES_USED | OP1_USED | OP_FETCH},
    /*  175 */ {"FETCH_STATIC_PROP_RW", RES_USED | OP1_USED | OP_FETCH},
    /*  176 */ {"FETCH_STATIC_PROP_IS", ALL_USED},
    /*  177 */ {"FETCH_STATIC_PROP_FUNC_ARG", RES_USED | OP1_USED | OP_FETCH},
    /*  178 */ {"FETCH_STATIC_PROP_UNSET", ALL_USED},
    /*  179 */ {"UNSET_STATIC_PROP", ALL_USED},
    /*  180 */ {"ISSET_ISEMPTY_STATIC_PROP", RES_USED | OP1_USED},
    /*  181 */ {"FETCH_CLASS_CONSTANT", ALL_USED},
    /*  182 */ {"BIND_LEXICAL", ALL_USED},
    /*  183 */ {"BIND_STATIC", ALL_USED},
    /*  184 */ {"FETCH_THIS", ALL_USED},
#if PHP_VERSION_ID >= 70300
    /*  185 */ {"SEND_FUNC_ARG", ALL_USED},
#else
    /*  185 */ {"UNKNOWN [185]", ALL_USED},
#endif
    /*  186 */ {"ISSET_ISEMPTY_THIS", ALL_USED},
#endif
#if PHP_VERSION_ID >= 70200
    /*  187 */ {"SWITCH_LONG", OP1_USED | OP2_USED | OP2_JMP_ARRAY | EXT_VAL_JMP_REL},
    /*  188 */ {"SWITCH_STRING", OP1_USED | OP2_USED | OP2_JMP_ARRAY | EXT_VAL_JMP_REL},
    /*  189 */ {"IN_ARRAY", ALL_USED},
    /*  190 */ {"COUNT", ALL_USED},
    /*  191 */ {"GET_CLASS", ALL_USED},
    /*  192 */ {"GET_CALLED_CLASS", ALL_USED},
    /*  193 */ {"GET_TYPE", ALL_USED},
#if PHP_VERSION_ID >= 70400
    /*  194 */ {"ARRAY_KEY_EXISTS", ALL_USED},
#else
    /*  194 */ {"FUNC_NUM_ARGS", ALL_USED},
    /*  195 */ {"FUNC_GET_ARGS", ALL_USED},
    /*  196 */ {"UNSET_CV", ALL_USED},
    /*  197 */ {"ISSET_ISEMPTY_CV", ALL_USED},
#if PHP_VERSION_ID >= 70300
    /*  198 */ {"FETCH_LIST_W", ALL_USED},
#endif
#endif
#endif
};

static cJSON *vld_cJSON_AddStringReferencToObject(cJSON *obj, const char *key, const char *value)
{
    cJSON *tempItem = cJSON_CreateStringReference(value);
    if (!tempItem)
    {
        return NULL;
    }
    cJSON_AddItemToObject(obj, key, tempItem);
    return tempItem;
}

static cJSON *vld_cJSON_AddStringReferencToArray(cJSON *array, const char *value)
{
    cJSON *tempItem = cJSON_CreateStringReference(value);
    if (!tempItem)
    {
        return NULL;
    }
    cJSON_AddItemToArray(array, tempItem);
    return tempItem;
}

static unsigned int vld_get_special_flags(const zend_op *op, unsigned int base_address)
{
    unsigned int flags = 0;

    switch (op->opcode)
    {
    case ZEND_FE_RESET_R:
    case ZEND_FE_RESET_RW:
        flags = ALL_USED;
        flags |= OP2_OPNUM;
        break;

    case ZEND_ASSIGN_REF:
        flags = OP1_USED | OP2_USED;
        if (op->VLD_TYPE(result) != IS_UNUSED)
        {
            flags |= RES_USED;
        }
        break;

    case ZEND_DO_FCALL:
        flags = OP1_USED | RES_USED | EXT_VAL;
        /*flags = ALL_USED | EXT_VAL;
			op->op2.op_type = IS_CONST;
			op->op2.u.constant.type = IS_LONG;*/
        break;

    case ZEND_INIT_FCALL_BY_NAME:
    case ZEND_INIT_NS_FCALL_BY_NAME:
        flags = OP2_USED;
        if (op->VLD_TYPE(op1) != IS_UNUSED)
        {
            flags |= OP1_USED;
        }
        break;

    case ZEND_JMPZNZ:
        flags = OP1_USED | OP2_USED | EXT_VAL_JMP_REL | OP2_OPNUM;
        //			op->result = op->op1;
        break;

    case ZEND_FETCH_CLASS:
        flags = EXT_VAL | RES_USED | OP2_USED | RES_CLASS;
        break;

    case ZEND_NEW:
        flags = RES_USED | OP1_USED;
        flags |= OP1_CLASS;
        break;

    case ZEND_BRK:
    case ZEND_CONT:
        flags = OP2_USED | OP2_BRK_CONT;
        break;
    case ZEND_FAST_CALL:
        flags = OP1_USED | OP1_OPLINE;
        if (op->extended_value)
        {
            flags |= OP2_USED | OP2_OPNUM | EXT_VAL;
        }
        break;
    case ZEND_FAST_RET:
        if (op->extended_value)
        {
            flags = OP2_USED | OP2_OPNUM | EXT_VAL;
        }
        break;
    case ZEND_CATCH:
#if PHP_VERSION_ID >= 70300
        flags = OP1_USED;
        if (op->extended_value & ZEND_LAST_CATCH)
        {
            flags |= EXT_VAL;
        }
        else
        {
            flags |= OP2_USED | OP2_OPLINE;
        }
#else
        flags = ALL_USED;
        if (!op->result.num)
        {
#if PHP_VERSION_ID >= 70100
            flags |= EXT_VAL_JMP_REL;
#else
            flags |= EXT_VAL_JMP_ABS;
#endif
        }
#endif
        break;
    }
    return flags;
}
#if PHP_VERSION_ID >= 70400
static const char *get_assign_operation(uint32_t extended_value)
{
    switch (extended_value)
    {
    case ZEND_ADD:
        return "+=";
    case ZEND_SUB:
        return "-=";
    case ZEND_MUL:
        return "*=";
    case ZEND_DIV:
        return "/=";
    case ZEND_MOD:
        return "%=";
    case ZEND_SL:
        return "<<=";
    case ZEND_SR:
        return ">>=";
    case ZEND_CONCAT:
        return ".=";
    case ZEND_BW_OR:
        return "|=";
    case ZEND_BW_AND:
        return "&=";
    case ZEND_BW_XOR:
        return "^=";
    case ZEND_POW:
        return "**=";
    default:
        return "";
    }
}
#endif

static inline char *vld_get_zval_string(ZVAL_VALUE_TYPE value)
{
    ZVAL_VALUE_STRING_TYPE *new_str;
    int len;
    new_str = php_url_encode(ZVAL_STRING_VALUE(value), ZVAL_STRING_LEN(value) PHP_URLENCODE_NEW_LEN(new_len));
    char *out = (char *)(calloc(new_str->len + 1, sizeof(char)));
    sprintf(out, "%s", ZSTRING_VALUE(new_str));
    efree(new_str);
    return out;
}

char *vld_get_zval_token(zval val, int *isvar)
{
    char *out = NULL;
    char tmp[128] = {0};
    *isvar = 0;
    switch (val.u1.v.type)
    {
    case IS_NULL:
        break;
    case IS_LONG:
        sprintf(tmp, "%ld", val.value.lval);
        *isvar = 1;
        break;
    case IS_DOUBLE:
        sprintf(tmp, "%g", val.value.dval);
        *isvar = 1;
        break;
    case IS_STRING:
        out = vld_get_zval_string(val.value);
        break;
    case IS_ARRAY:
        out = "<array>";
        break;
    case IS_OBJECT:
        out = "<object>";
        break;
    case IS_RESOURCE:
        out = "<resource>";
        break;
#if PHP_VERSION_ID < 70300
    case IS_CONSTANT:
        out = (char *)(calloc(val.value.str->len + 1 + 10, sizeof(char)));
        if (out)
        {
            sprintf(out, "<const:'%s'>", (val.value.str->val));
        }
        break;
#endif
    case IS_CONSTANT_AST:
        out = "<const ast>";
        break;
    case IS_UNDEF:
        out = "<undef>";
        break;
    case IS_FALSE:
        out = "<false>";
        break;
    case IS_TRUE:
        out = "<true>";
        break;
    case IS_REFERENCE:
        out = "<reference>";
        break;
    case IS_CALLABLE:
        out = "<callable>";
        break;
    case IS_INDIRECT:
        out = "<indirect>";
        break;
    case IS_PTR:
        out = "<ptr>";
        break;
    default:
        out = "<unknown>";
        break;
    }
    if (!out && tmp[0] != '\0')
    {
        out = (char *)(calloc(strlen(tmp) + 1, sizeof(char)));
        memcpy(out, tmp, strlen(tmp));
    }
    return out;
}

char *vld_get_znode_token(int *isvar, unsigned int node_type, VLD_ZNODE node, unsigned int base_address, zend_op_array *op_array, int opline TSRMLS_DC)
{
    int len = 0;
    char *out = NULL;
    char tmp[128] = {0};
    switch (node_type)
    {
    case IS_UNUSED:
        *isvar = 0;
        break;
    case IS_CONST: /* 1 */
                   // VLD_PRINT1(3, " IS_CONST (%d) ", VLD_ZNODE_ELEM(node, var) / sizeof(zval));
#if PHP_VERSION_ID >= 70300
        out = vld_get_zval_token(*RT_CONSTANT((op_array->opcodes) + opline, node), isvar);

#else
        out = vld_get_zval_token(*RT_CONSTANT_EX(op_array->literals, node), isvar);
#endif
        break;
    case IS_TMP_VAR: /* 2 */
        sprintf(tmp, "~%d", VAR_NUM(VLD_ZNODE_ELEM(node, var)));
        *isvar = 1;
        break;
    case IS_VAR: /* 4 */
        sprintf(tmp, "$%d", VAR_NUM(VLD_ZNODE_ELEM(node, var)));
        *isvar = 1;
        break;
    case IS_CV: /* 16 */
        sprintf(tmp, "!%d", (VLD_ZNODE_ELEM(node, var) - sizeof(zend_execute_data)) / sizeof(zval));
        *isvar = 1;
        break;
    case VLD_IS_OPNUM:
        sprintf(tmp, "->%d", VLD_ZNODE_JMP_LINE(node, opline, base_address));
        *isvar = 1;
        break;
    case VLD_IS_OPLINE:
        sprintf(tmp, "->%d", VLD_ZNODE_JMP_LINE(node, opline, base_address));
        *isvar = 1;
        break;
    case VLD_IS_CLASS:
        sprintf(tmp, ":%d", VAR_NUM(VLD_ZNODE_ELEM(node, var)));
        *isvar = 1;
        break;
#if PHP_VERSION_ID >= 70200
    case VLD_IS_JMP_ARRAY:
    {
        zval *array_value;
        HashTable *myht;
        zend_ulong num;
        zend_string *key;
        zval *val;
#if PHP_VERSION_ID >= 70300
        array_value = RT_CONSTANT((op_array->opcodes) + opline, node);
#else
        array_value = RT_CONSTANT_EX(op_array->literals, node);
#endif
        myht = Z_ARRVAL_P(array_value);
        ZEND_HASH_FOREACH_KEY_VAL_IND(myht, num, key, val)
        {
            if (key == NULL)
            {
                len = sprintf(tmp, "%d:->%d, ", num, opline + (val->value.lval / sizeof(zend_op)));
            }
            else
            {
                len = sprintf(tmp, "'%s':->%d, ", ZSTRING_VALUE(key), opline + (val->value.lval / sizeof(zend_op)));
            }
            if (!out)
            {
                out = (char *)(calloc(len + 1, sizeof(char)));
                memcpy(out, tmp, strlen(tmp));
                *isvar = 1;
            }
            else
            {
                char *out_p = (char *)(calloc(len + 1 + strlen(out), sizeof(char)));
                memcpy(out_p, out, strlen(out));
                memcpy(out_p + strlen(out), tmp, len);
                free(out);
                out = out_p;
            }
        }
        ZEND_HASH_FOREACH_END();
    }
    break;
#endif
    default:
        *isvar = 0;
        return out;
    }
    if (!out && tmp[0] != '\0')
    {
        out = (char *)(calloc(strlen(tmp) + 1, sizeof(char)));
        memcpy(out, tmp, strlen(tmp));
    }
    return out;
}

void vld_get_op_json(int nr, zend_op *op_ptr, unsigned int base_address, cJSON **ops_op, cJSON **ops_fetch, cJSON **ops_ext, cJSON **ops_return, cJSON **ops_op1, cJSON **ops_op2, zend_op_array *opa TSRMLS_DC)
{
    static uint last_lineno = (uint)-1;
    const char *fetch_type = NULL;
    unsigned int flags, op1_type, op2_type, res_type;
    const zend_op op = op_ptr[nr];
    if (op.lineno == 0)
    {
        return;
    }
    *ops_ext = NULL;
    *ops_return = NULL;
    *ops_fetch = NULL;
    *ops_op1 = NULL;
    *ops_op2 = NULL;
    if (op.opcode >= NUM_KNOWN_OPCODES)
    {
        flags = ALL_USED;
    }
    else
    {
        flags = opcodes[op.opcode].flags;
    }
    op1_type = op.VLD_TYPE(op1);
    op2_type = op.VLD_TYPE(op2);
    res_type = op.VLD_TYPE(result);
    if (flags == SPECIAL)
    {
        flags = vld_get_special_flags(&op, base_address);
    }
    if (flags & OP1_OPLINE)
    {
        op1_type = VLD_IS_OPLINE;
    }
    if (flags & OP2_OPLINE)
    {
        op2_type = VLD_IS_OPLINE;
    }
    if (flags & OP1_OPNUM)
    {
        op1_type = VLD_IS_OPNUM;
    }
    if (flags & OP2_OPNUM)
    {
        op2_type = VLD_IS_OPNUM;
    }
    if (flags & OP1_CLASS)
    {
        op1_type = VLD_IS_CLASS;
    }
    if (flags & RES_CLASS)
    {
        res_type = VLD_IS_CLASS;
    }
    if (flags & OP2_JMP_ARRAY)
    {
        op2_type = VLD_IS_JMP_ARRAY;
    }
#if PHP_VERSION_ID >= 70000 && PHP_VERSION_ID < 70100
    switch (op.opcode)
    {
    case ZEND_FAST_RET:
        if (op.extended_value == ZEND_FAST_RET_TO_FINALLY)
        {
            fetch_type = "to_finally";
        }
        else if (op.extended_value == ZEND_FAST_RET_TO_CATCH)
        {
            fetch_type = "to_catch";
        }
        break;
    case ZEND_FAST_CALL:
        if (op.extended_value == ZEND_FAST_CALL_FROM_FINALLY)
        {
            fetch_type = "from_finally";
        }
        break;
    }
#endif

#if PHP_VERSION_ID >= 70400
    if (op.opcode == ZEND_ASSIGN_DIM_OP)
    {
        fetch_type = get_assign_operation(op.extended_value);
    }
#endif
    if (flags & OP_FETCH)
    {
        switch (op.VLD_EXTENDED_VALUE(op2))
        {
        case ZEND_FETCH_GLOBAL:
            fetch_type = "global";
            break;
        case ZEND_FETCH_LOCAL:
            fetch_type = "local";
            break;
#if PHP_VERSION_ID < 70100
        case ZEND_FETCH_STATIC:
            fetch_type = "static";
            break;
        case ZEND_FETCH_STATIC_MEMBER:
            fetch_type = "static member";
            break;
#endif
#ifdef ZEND_FETCH_GLOBAL_LOCK
        case ZEND_FETCH_GLOBAL_LOCK:
            fetch_type = "global lock";
            break;
#endif
#ifdef ZEND_FETCH_AUTO_GLOBAL
        case ZEND_FETCH_AUTO_GLOBAL:
            fetch_type = "auto global";
            break;
#endif
        default:
            fetch_type = "unknown";
            break;
        }
    }
    if (op.lineno != last_lineno)
    {
        last_lineno = op.lineno;
    }
    if (op.opcode >= NUM_KNOWN_OPCODES)
    {
        *ops_op = cJSON_CreateNull();
    }
    else
    {
        *ops_op = cJSON_CreateStringReference(opcodes[op.opcode].name);
    }
    if (fetch_type)
    {
        *ops_fetch = cJSON_CreateStringReference(fetch_type);
    }
    else
    {
        *ops_fetch = cJSON_CreateNull();
    }
    if (flags & EXT_VAL)
    {
#if PHP_VERSION_ID >= 70300
        if (op.opcode == ZEND_CATCH)
        {
            *ops_ext = cJSON_CreateStringReference("last");
        }
        else
        {
            char tmp[128] = {0};
            sprintf(tmp, "%3d", op.extended_value);
            *ops_ext = cJSON_CreateString(tmp);
        }
#else
        char tmp[128] = {0};
        sprintf(tmp, "%3d", op.extended_value);
        *ops_ext = cJSON_CreateString(tmp);
#endif
    }
#if PHP_VERSION_ID >= 70100
    if ((flags & RES_USED) && op.result_type != IS_UNUSED)
    {
#else
    if ((flags & RES_USED) && !(op.VLD_EXTENDED_VALUE(result) & EXT_TYPE_UNUSED))
    {
#endif
        int isvar = 0;
        char *ops_rt = vld_get_znode_token(&isvar, res_type, op.result, base_address, opa, nr TSRMLS_CC);
        if (isvar)
        {
            *ops_return = cJSON_CreateString(ops_rt);
            free(ops_rt);
        }
        else
        {
            *ops_return = cJSON_CreateStringReference(ops_rt);
        }
    }
    else
    {
        *ops_return = cJSON_CreateNull();
    }
    if (flags & OP1_USED)
    {
        int isvar = 0;
        char *ops_op1_str = vld_get_znode_token(&isvar, op1_type, op.op1, base_address, opa, nr TSRMLS_CC);
        if (isvar)
        {
            *ops_op1 = cJSON_CreateString(ops_op1_str);
            free(ops_op1_str);
        }
        else
        {
            *ops_op1 = cJSON_CreateStringReference(ops_op1_str);
        }
    }
    else
    {
        *ops_op1 = cJSON_CreateNull();
    }
    if (flags & OP2_USED)
    {
        if (flags & OP2_INCLUDE)
        {
            switch (op.extended_value)
            {
            case ZEND_INCLUDE_ONCE:
                *ops_op2 = cJSON_CreateStringReference("INCLUDE_ONCE");
                break;
            case ZEND_REQUIRE_ONCE:
                *ops_op2 = cJSON_CreateStringReference("REQUIRE_ONCE");
                break;
            case ZEND_INCLUDE:
                *ops_op2 = cJSON_CreateStringReference("INCLUDE");
                break;
            case ZEND_REQUIRE:
                *ops_op2 = cJSON_CreateStringReference("REQUIRE");
                break;
            case ZEND_EVAL:
                *ops_op2 = cJSON_CreateStringReference("EVAL");
                break;
            default:
                *ops_op2 = cJSON_CreateStringReference("!!ERROR!!");
                break;
            }
        }
        else
        {
            int isvar = 0;
            char *ops_op2_str = vld_get_znode_token(&isvar, op2_type, op.op2, base_address, opa, nr TSRMLS_CC);
            if (ops_op2_str && isvar)
            {
                *ops_op2 = cJSON_CreateString(ops_op2_str);
                free(ops_op2_str);
            }
            else if (ops_op2_str)
            {
                *ops_op2 = cJSON_CreateStringReference(ops_op2_str);
            }
        }
    }
    if (flags & EXT_VAL_JMP_ABS)
    {
        char tmp[64] = {0};
        sprintf(tmp, ", ->%d", op.extended_value);
        cJSON_Delete(*ops_ext);
        *ops_ext = cJSON_CreateString(tmp);
    }
    if (flags & EXT_VAL_JMP_REL)
    {
        char tmp[64] = {0};
        sprintf(tmp, ", ->%d", nr + ((int)op.extended_value / sizeof(zend_op)));
        cJSON_Delete(*ops_ext);
        *ops_ext = cJSON_CreateString(tmp);
    }
    if (!(*ops_ext))
    {
        *ops_ext = cJSON_CreateNull();
    }
    if (flags & NOP2_OPNUM)
    {
        zend_op next_op = op_ptr[nr + 1];
        int isvar = 0;
        char *ops_op2_str = vld_get_znode_token(&isvar, VLD_IS_OPNUM, next_op.op2, base_address, opa, nr TSRMLS_CC);
        cJSON_Delete(*ops_op2);
        if (!ops_op2_str && isvar)
        {
            *ops_op2 = cJSON_CreateString(ops_op2_str);
            free(ops_op2_str);
        }
        else if (ops_op2_str)
        {
            *ops_op2 = cJSON_CreateStringReference(ops_op2_str);
        }
        else
        {
            *ops_op2 = cJSON_CreateNull();
        }
    }
    if (!(*ops_op2))
    {
        *ops_op2 = cJSON_CreateNull();
    }
}

void vld_analyse_branch_slience(zend_op_array *opa, unsigned int position, vld_set *set, vld_branch_info *branch_info TSRMLS_DC)
{
    vld_set_add(branch_info->starts, position);
    branch_info->branches[position].start_lineno = opa->opcodes[position].lineno;

    /* First we see if the branch has been visited, if so we bail out. */
    if (vld_set_in(set, position))
    {
        return;
    }
    /* Loop over the opcodes until the end of the array, or until a jump point has been found */
    vld_set_add(set, position);
    while (position < opa->last)
    {
        size_t jump_count = 0;
        int jumps[VLD_BRANCH_MAX_OUTS];
        size_t i;
        /* See if we have a jump instruction */
        if (vld_find_jumps(opa, position, &jump_count, jumps))
        {
            for (i = 0; i < jump_count; i++)
            {
                if (jumps[i] == VLD_JMP_EXIT || jumps[i] >= 0)
                {
                    vld_branch_info_update(branch_info, position, opa->opcodes[position].lineno, i, jumps[i]);
                    if (jumps[i] != VLD_JMP_EXIT)
                    {
                        vld_analyse_branch_slience(opa, jumps[i], set, branch_info TSRMLS_CC);
                    }
                }
            }
            break;
        }
        /* See if we have a throw instruction */
        if (opa->opcodes[position].opcode == ZEND_THROW)
        {
            vld_set_add(branch_info->ends, position);
            branch_info->branches[position].start_lineno = opa->opcodes[position].lineno;
            break;
        }
        /* See if we have an exit instruction */
        if (opa->opcodes[position].opcode == ZEND_EXIT)
        {
            vld_set_add(branch_info->ends, position);
            branch_info->branches[position].start_lineno = opa->opcodes[position].lineno;
            break;
        }
        /* See if we have a return instruction */
        if (
            opa->opcodes[position].opcode == ZEND_RETURN || opa->opcodes[position].opcode == ZEND_RETURN_BY_REF)
        {
            vld_set_add(branch_info->ends, position);
            branch_info->branches[position].start_lineno = opa->opcodes[position].lineno;
            break;
        }
        position++;
        vld_set_add(set, position);
    }
}

void vld_analyse_oparray_slience(zend_op_array *opa, vld_set *set, vld_branch_info *branch_info TSRMLS_DC)
{
    unsigned int position = 0;
    while (position < opa->last)
    {
        if (position == 0)
        {
            vld_analyse_branch_slience(opa, position, set, branch_info TSRMLS_CC);
            vld_set_add(branch_info->entry_points, position);
        }
        else if (opa->opcodes[position].opcode == ZEND_CATCH)
        {
            vld_analyse_branch_slience(opa, position, set, branch_info TSRMLS_CC);
            vld_set_add(branch_info->entry_points, position);
        }
        position++;
    }
    vld_set_add(branch_info->ends, opa->last - 1);
    branch_info->branches[opa->last - 1].start_lineno = opa->opcodes[opa->last - 1].lineno;
}

cJSON *vld_opa_dump_json(zend_op_array *opa TSRMLS_DC)
{
    if (!opa)
    {
        return NULL; //Check params.
    }
    unsigned int i;
    int j;
    vld_set *set;
    vld_branch_info *branch_info;
    unsigned int base_address = (unsigned int)(zend_intptr_t) & (opa->opcodes[0]);
    set = vld_set_create(opa->last);
    branch_info = vld_branch_info_create(opa->last);
    vld_analyse_oparray_slience(opa, set, branch_info TSRMLS_CC);
    // Create a new json object.
    cJSON *optree = cJSON_CreateObject();
    if (!optree)
    {
        vld_cJSON_Cancel(NULL);
    }
    // Save basic block info.
    if (!vld_cJSON_AddStringReferencToObject(optree, "filename", OPARRAY_VAR_NAME(opa->filename)))
    {
        vld_cJSON_Cancel(optree);
    }
    if (opa->function_name && OPARRAY_VAR_NAME(opa->function_name))
    {
        if (!vld_cJSON_AddStringReferencToObject(optree, "function name", OPARRAY_VAR_NAME(opa->function_name)))
        {
            vld_cJSON_Cancel(optree);
        }
    }
    else
    {
        if (!cJSON_AddNullToObject(optree, "function name"))
        {
            vld_cJSON_Cancel(optree);
        }
    }
    if (!cJSON_AddNumberToObject(optree, "number of ops", opa->last))
    {
        vld_cJSON_Cancel(optree);
    }
    // Save compiled vars info.
    cJSON *cvars = cJSON_AddObjectToObject(optree, "compiled vars");
    if (!cvars)
    {
        vld_cJSON_Cancel(optree);
    }
    if (!cJSON_AddNumberToObject(cvars, "nums", opa->last_var))
    {
        vld_cJSON_Cancel(optree);
    }
    cJSON *vars = cJSON_AddArrayToObject(cvars, "vars");
    if (!vars)
    {
        vld_cJSON_Cancel(optree);
    }
    for (j = 0; j < opa->last_var; j++)
    {
        if (!vld_cJSON_AddStringReferencToArray(vars, OPARRAY_VAR_NAME(opa->vars[j])))
        {
            vld_cJSON_Cancel(optree);
        }
    }
    // Save ops info.
    cJSON *ops = cJSON_AddObjectToObject(optree, "ops");
    if (!ops)
    {
        vld_cJSON_Cancel(optree);
    }
    cJSON *ops_line = cJSON_AddArrayToObject(ops, "line");
    cJSON *ops_oline = cJSON_AddArrayToObject(ops, "#");
    cJSON *ops_op = cJSON_AddArrayToObject(ops, "op");
    cJSON *ops_fetch = cJSON_AddArrayToObject(ops, "fetch");
    cJSON *ops_ext = cJSON_AddArrayToObject(ops, "ext");
    cJSON *ops_return = cJSON_AddArrayToObject(ops, "return");
    cJSON *ops_op1 = cJSON_AddArrayToObject(ops, "op1");
    cJSON *ops_op2 = cJSON_AddArrayToObject(ops, "op2");
    if (!ops_line || !ops_oline || !ops_op || !ops_fetch || !ops_ext || !ops_return || !ops_op1 || !ops_op2)
    {
        vld_cJSON_Cancel(optree);
    }
    for (i = 0; i < opa->last; i++)
    {
        cJSON *ops_line_e = NULL;
        cJSON *ops_oline_e = NULL;
        cJSON *ops_op_e = NULL;
        cJSON *ops_fetch_e = NULL;
        cJSON *ops_ext_e = NULL;
        cJSON *ops_return_e = NULL;
        cJSON *ops_op1_e = NULL;
        cJSON *ops_op2_e = NULL;
        vld_get_op_json(i, opa->opcodes, base_address, &ops_op_e, &ops_fetch_e, &ops_ext_e, &ops_return_e, &ops_op1_e, &ops_op2_e, opa);
        ops_line_e = cJSON_CreateNumber((opa->opcodes)[i].lineno);
        if (!ops_line_e)
        {
            vld_cJSON_Cancel(optree);
        }
        cJSON_AddItemToArray(ops_line, ops_line_e);
        ops_oline_e = cJSON_CreateNumber(i);
        if (!ops_oline_e)
        {
            vld_cJSON_Cancel(optree);
        }
        cJSON_AddItemToArray(ops_oline, ops_oline_e);
        cJSON_AddItemToArray(ops_op, ops_op_e);
        cJSON_AddItemToArray(ops_fetch, ops_fetch_e);
        cJSON_AddItemToArray(ops_ext, ops_ext_e);
        cJSON_AddItemToArray(ops_return, ops_return_e);
        cJSON_AddItemToArray(ops_op1, ops_op1_e);
        cJSON_AddItemToArray(ops_op2, ops_op2_e);
    }
    // Save paths info.
    vld_branch_post_process(opa, branch_info);
    vld_branch_find_paths(branch_info);
    cJSON *paths = cJSON_AddArrayToObject(optree, "paths");
    if (!paths)
    {
        vld_cJSON_Cancel(optree);
    }
    for (i = 0; i < branch_info->paths_count; i++)
    {
        cJSON *path = cJSON_CreateArray();
        if (!path)
        {
            vld_cJSON_Cancel(optree);
        }
        cJSON_AddItemToArray(paths, path);
        for (j = 0; j < branch_info->paths[i]->elements_count; j++)
        {
            cJSON *pnode = cJSON_CreateNumber(branch_info->paths[i]->elements[j]);
            if (!pnode)
            {
                vld_cJSON_Cancel(optree);
            }
            cJSON_AddItemToArray(path, pnode);
        }
    }
    vld_set_free(set);
    vld_branch_info_free(branch_info);
    return optree;
}