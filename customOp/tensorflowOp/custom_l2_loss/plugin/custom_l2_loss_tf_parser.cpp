﻿/* Copyright (C) 2019. Huawei Technologies Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Apache License Version 2.0.You may not use this file except in compliance with the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Apache License for more details at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <Python.h>
#include "custom/custom_op.h"
#include "framework/omg/register.h"
#include "framework/omg/omg_types.h"
#include "operator.h"
#include "attr_value.h"
#include <memory>
#include <string>
#include <vector>

using namespace ge;
namespace domi
{

// #### Obtains the processing function of the output tensor description. 
Status TFcustom_l2_lossInferShapeAndType(const ge::Operator& op, vector<ge::TensorDesc>& v_output_desc)
{
    auto tensorDesc      = op.GetInputDesc(0);
    auto shape = tensorDesc.GetShape();

    tensorDesc.SetShape(shape);
    v_output_desc.push_back(tensorDesc);

    return SUCCESS;

}


// build Te Binary file
Status TFcustom_l2_lossBuildTeBin(const ge::Operator& op, TEBinInfo& te_bin_info)
{
    std::string FilePath   = "";
	std::string RealFilePath = "";
    std::string FuncName   = "";
    std::string KernelName = "";
	
    
	// ### Parse input tensor description 
	TensorDesc input_desc     = op.GetInputDesc(0); 

    FilePath   = "../operator/custom_l2_loss";
    FuncName   = "custom_l2_loss";
    KernelName = "custom_l2_loss" + std::to_string(input_desc.GetShape().GetDim(0)) + std::to_string(input_desc.GetShape().GetDim(1));

    //get real path of Py module
	char *cwd = getcwd(NULL, 0);
	if (cwd == NULL) {
		printf("Get current directory path failed!\n");
		return FAILED;
	}
	std::string cwd_s(cwd);
	char *real_path = realpath((cwd_s + "/" + FilePath + ".py").c_str(), NULL);
	if (real_path == NULL) {
		printf("Get real path of Py module failed!\n");
		return FAILED;
	}
	std::string RealFilePath_ = std::string(real_path);
	RealFilePath = RealFilePath_.substr(0, RealFilePath_.rfind("."));

	std::map<ge::DataType, std::string> operation_map = {
		{ ge::DT_UNDEFINED, "undefined" },
		{ ge::DT_FLOAT, "float32" },
		{ ge::DT_FLOAT16, "float16" },
		{ ge::DT_INT8, "int8" },
		{ ge::DT_INT16, "int16" },
		{ ge::DT_INT32, "int32" },
		{ ge::DT_INT64, "int64" },
		{ ge::DT_UINT8, "uint8" },
		{ ge::DT_UINT16, "uint16" },
		{ ge::DT_UINT32, "uint32" },
		{ ge::DT_UINT64, "uint64" },
		{ ge::DT_BOOL, "bool" },
		{ ge::DT_DOUBLE, "double" },
		{ ge::DT_DUAL, "dual" },
		{ ge::DT_DUAL_SUB_INT8, "dual_sub_int8" },
		{ ge::DT_DUAL_SUB_UINT8, "dual_sub_uint8" }
	};
	std::string dtype = operation_map[op.GetInputDesc(0).GetDataType()];
	
    // i => int; s => string; f => dobule; O => bool, and bool value is Py_True or Py_False
    te::BuildTeCustomOp(te_bin_info.ddk_version, op.GetName(), RealFilePath, FuncName,
 		"(i,i,i,i),s, s,O", 
		input_desc.GetShape().GetDim(0), input_desc.GetShape().GetDim(1), input_desc.GetShape().GetDim(2), input_desc.GetShape().GetDim(3),
		dtype.c_str(),
		KernelName.c_str(),
        Py_True);

    // set te op json to te_bin_info 
    te_bin_info.bin_file_path  = "./kernel_meta/" + KernelName + ".o";
    te_bin_info.json_file_path = "./kernel_meta/" + KernelName + ".json";
 
    return SUCCESS;
}

REGISTER_CUSTOM_OP("custom_l2_loss") //custom_l2_loss is the type name of the operator in the OM model. It can be specified randomly and cannot be the same as an existing type name. 
    .FrameworkType(TENSORFLOW)  // Enumerated type. The options are as follows: CAFFE, TENSORFLOW
    .OriginOpType("L2Loss")  // // L2Loss indicates the type name of the operator in the caffe framework.
    .ParseParamsFn(AutoMappingFn)  // AutoMappingFn indicates automatic mapping the parameters of op.
    .InferShapeAndTypeFn(TFcustom_l2_lossInferShapeAndType)       // Set output description and datatype function
    .TEBinBuildFn(TFcustom_l2_lossBuildTeBin) // Build Te op binary function
    .ImplyType(ImplyType::TVM) // Implementation type. Enumerated type, The options are as follows: TVM, AI_CPU.
    .Formats({DOMI_TENSOR_ND},{DOMI_TENSOR_ND});   //  #### Format of the input and output

}  // namespace domi
