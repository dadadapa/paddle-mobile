/* Copyright (c) 2016 Baidu, Inc. All Rights Reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
==============================================================================*/

#pragma once

#include "paddle_mobile_object.h"
#include "scope.h"
#include "tensor.h"
#include "op_kernel_type.h"
#include "../common/variant.h"
#include "block_desc.h"
#include "variable.h"
#include <map>

namespace paddle_mobile {
namespace framework {
    /// If a variable is a empty variable, that name will be used.
    constexpr char kEmptyVarName[] = "@EMPTY@";

    using VariableNameMap = std::map<std::string, std::vector<std::string> >;
    using AttributeMap = std::string;
    class InferShapeContext;
    class ExecutionContext;

    class OperatorBase: PaddleMobileObject{
    public:
        OperatorBase(const std::string& type, const VariableNameMap& inputs,
                     const VariableNameMap& outputs, const AttributeMap & attrs);

        virtual ~OperatorBase() {}

        /// Net will call this interface function to Run an op.
        //  The implementation should be written at RunImpl
        void Run(const Scope& scope);

        template <typename T>
        inline const T& Attr(const std::string& name) const;

        const VariableNameMap& Inputs() const { return inputs_; }
        const VariableNameMap& Outputs() const { return outputs_; }
        //! Get a input with argument's name described in `op_proto`
        std::string Input(const std::string& name) const;
        //! Get a input which has multiple variables.
        const std::vector<std::string>& Inputs(const std::string& name) const;
        std::vector<std::string> InputVars() const;

        //! Get a output with argument's name described in `op_proto`
        std::string Output(const std::string& name) const;
        //! Get an output which has multiple variables.
        //! TODO add a vector_view to prevent memory copy.
        const std::vector<std::string>& Outputs(const std::string& name) const;
        virtual std::vector<std::string> OutputVars(bool has_intermediate) const;

        const std::string& Type() const { return type_; }
        void SetType(const std::string& type) { type_ = type; }
        const AttributeMap& Attrs() const { return attrs_; }

    protected:
        std::string type_;
        // NOTE: in case of OpGrad, inputs_ contains:
        // I (Inputs)
        // O (Outputs)
        // OG (Output Gradients)
        VariableNameMap inputs_;

        // NOTE: in case of OpGrad, outputs_ contains
        // IG (Inputs Gradients)
        VariableNameMap outputs_;
        AttributeMap attrs_;
    private:
        void CheckAllInputOutputSet() const;
        virtual void RunImpl(const Scope& scope) const = 0;
    };


    class OperatorWithKernel : public OperatorBase{
    public:
        OperatorWithKernel(const std::string& type, const VariableNameMap& inputs,
                           const VariableNameMap& outputs, const AttributeMap& attrs)
                : OperatorBase(type, inputs, outputs, attrs) {}

        virtual void InferShape(InferShapeContext* ctx) const {};

    protected:
        virtual OpKernelType GetExpectedKernelType(const ExecutionContext& ctx) const;
        virtual OpKernelType GetKernelTypeForVar(
                const std::string& var_name, const Tensor4f& tensor,
                const OpKernelType& expected_kernel_type) const;
    private:
        void RunImpl(const Scope& scope) const final;
    };



    class OpKernelBase: PaddleMobileObject{
    public:
        /**
         * ExecutionContext is the only parameter of Kernel Run function.
         * Run will get input/output variables, state such as momentum and
         * device resource such as CUDA stream, cublas handle, etc. from
         * ExecutionContext. User should construct it before run the Operator.
         */
        virtual void Compute(const ExecutionContext& context) const = 0;

        virtual ~OpKernelBase() = default;
    };


    template <typename T>
    class OpKernel : public OpKernelBase{
    public:
        using ELEMENT_TYPE = T;
    };


    class ExecutionContext {
    public:
        ExecutionContext(const OperatorBase& op, const Scope& scope)
                : op_(op), scope_(scope) {}

        const OperatorBase& op() const { return op_; }

        const Scope& scope() const { return scope_; }

        template <typename T>
        inline const T& Attr(const std::string& name) const {
            return op_.Attr<T>(name);
        }

        size_t InputSize(const std::string& name) const {
            return op_.Inputs(name).size();
        }

        size_t OutputSize(const std::string& name) const {
            return op_.Outputs(name).size();
        }

        const Variable* InputVar(const std::string& name) const {}

        Variable* OutputVar(const std::string& name) const {}

        const std::vector<const Variable*> MultiInputVar(const std::string& name) const {}

        std::vector<Variable*> MultiOutputVar(const std::string& name) const {}

        template <typename T>
        const T* Input(const std::string& name) const {}

        template <typename T>
        T* Output(const std::string& name) const {}

        template <typename T>
        const std::vector<const T*> MultiInput(const std::string& name) const {}

        template <typename T>
        std::vector<T*> MultiOutput(const std::string& name) const {}

        //! Get actual name vector for this input.
        const std::vector<std::string>& Inputs(const std::string& name) const {
            return op_.Inputs(name);
        }

        //! Get actual name vector for this output.
        const std::vector<std::string>& Outputs(const std::string& name) const {
            return op_.Outputs(name);
        }

    private:
        const OperatorBase& op_;
        const Scope& scope_;
    };

} // operators
} // paddle_mobile
