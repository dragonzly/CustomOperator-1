"""
Copyright (C) 2016. Huawei Technologies Co., Ltd. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the Apache License Version 2.0.You may not use this file except in compliance with the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
Apache License for more details at
http://www.apache.org/licenses/LICENSE-2.0

tf mod
"""

from te import tvm
from topi.cce import util
from te.platform.cce_build import build_config

SHAPE_SIZE_LIMIT = 150000000  # shape limit for tf_mod

def _flatten_indice(data, indice):
    """
    flatten indice of data
    Parameters
    ----------
    data : tvm.tensor
        input data

    indice : list
        list of indice

    Returns
    -------
    ret : tensor
        example when indice = [1,2,3]:
        return tensor : data[1][2][3].
    """
    tmp = data
    for i in indice:
        tmp = tmp[i]
    return tmp

@util.check_input_type((list, tuple), (list, tuple), str, str, bool, bool)
def custom_mod(shape1, shape2, dtype, kernel_name="cce_tf_mod", need_build=False, need_print=False):
    """
    do element-wise mod operation between two input tensors

    Parameters:
    ----------
    shape1 : shape of input data1

    shape2 : shape of input data2

    dtype : source data type, support float16,float32,int32

    kernel_name : cce kernel name, default value is "cce_tf_mod"

    need_buid : if need to build CCEC kernel, default value is False

    need_print : if need to print the ir, default value is False

    Returns
    -------
    None
    """

    max_dim = 8
    shape1_len = len(shape1)
    shape2_len = len(shape2)
    if shape1_len > max_dim or shape2_len > max_dim:
        raise RuntimeError(
            "mod_cce only support up to %d dimensions while the shape's dimensions is %d, %d" %
            (max_dim, shape1_len, shape2_len))

    util.check_kernel_name(kernel_name)
    util.check_shape_rule(shape1)
    util.check_shape_rule(shape2)
    util.check_shape_size(shape1, SHAPE_SIZE_LIMIT)
    util.check_shape_size(shape2, SHAPE_SIZE_LIMIT)

    check_list = ["float16", "float32", "int32"]
    device_api_map = {"float16": "cc_device_floormod_float16",
                      "float32": "cc_device_floormod_float",
                      "int32": "cc_device_floormod_int32"
                      }

    dtype = dtype.lower()
    if not (dtype in check_list):
        raise RuntimeError(
            "tf_mod_cce only support %s while dtype is %s" % (",".join(check_list), dtype))

    shape1, shape2, shape_out = util.produce_shapes(shape1, shape2)
    util.check_shape_size(shape_out, SHAPE_SIZE_LIMIT)

    inp_dtype = dtype.lower()

    device_api = device_api_map[inp_dtype]

    ## block
    block_num = "block_num"
    block_idx = "block_idx"
    ## x param
    v_xndim_cnt = tvm.const(len(shape1), "int32")
    p_xshape = util.create_param_ptr(shape1, "int32", "p_xshape")
    xpadC0 = tvm.const(0, "int32")
    data_input_x = tvm.placeholder(shape1, name="data_input_x", dtype=inp_dtype)
    ## y param
    v_yndim_cnt = tvm.const(len(shape2), "int32")
    p_yshape = util.create_param_ptr(shape2, "int32", "p_yshape")
    ypadC0 = tvm.const(0, "int32")
    data_input_y = tvm.placeholder(shape2, name="data_input_y", dtype=inp_dtype)
    ## output
    v_out_ndim_cnt = tvm.const(len(shape_out), "int32")
    p_out_shape = util.create_param_ptr(shape_out, "int32", "p_yshape")
    out_padC0 = tvm.const(0, "int32")

    output = tvm.extern(shape_out, [p_xshape, data_input_x, p_yshape, data_input_y, p_out_shape],
                        lambda ins, outs:
                        tvm.call_extern("int32_t", device_api,
                                        block_num,
                                        block_idx,
                                        v_xndim_cnt,
                                        ins[0].access_ptr("r"),  # shape x
                                        xpadC0,
                                        ins[1].access_ptr("r"),  # input x
                                        v_yndim_cnt,
                                        ins[2].access_ptr("r"),  # shape y
                                        ypadC0,
                                        ins[3].access_ptr("r"),  # input y
                                        v_out_ndim_cnt,
                                        ins[4].access_ptr("r"),  # shape out
                                        out_padC0,
                                        outs[0].access_ptr("w")),
                        name="output", dtype=inp_dtype)

    s = tvm.create_schedule(output.op)

    if need_print:
        with build_config:
            print(tvm.lower(s, [data_input_x, data_input_y, output], simple_mode=True))
    if need_build:
        with build_config:
            tvm.build(s, [data_input_x, data_input_y, output], "cce", name=kernel_name)
