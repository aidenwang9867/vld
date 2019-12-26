#ifndef SC_OPARRAY_H
#define SC_OPARRAY_H
#include "srm_oparray.h"
#include "branchinfo.h"
#include "cJSON.h"
void vld_get_op_json(int nr, zend_op *op_ptr, unsigned int base_address, cJSON **ops_op, cJSON **ops_fetch, cJSON **ops_ext, cJSON **ops_return, cJSON **ops_op1, cJSON **ops_op2, zend_op_array *opa TSRMLS_DC);
void vld_analyse_oparray_slience(zend_op_array *opa, vld_set *set, vld_branch_info *branch_info TSRMLS_DC);
void vld_analyse_branch_slience(zend_op_array *opa, unsigned int position, vld_set *set, vld_branch_info *branch_info TSRMLS_DC);
cJSON *vld_opa_dump_json(zend_op_array *opa TSRMLS_DC);
#endif //SC_OPARRAY_H