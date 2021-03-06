/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*!
 * \file op_common.h
 * \brief A set of utilities and common functionality
 * for relay ops.
 */
#ifndef TVM_RELAY_OP_OP_COMMON_H_
#define TVM_RELAY_OP_OP_COMMON_H_

#include <tvm/relay/expr.h>
#include <tvm/relay/op.h>
#include <tvm/relay/op_attr_types.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "type_relations.h"
#include "../pass/alter_op_layout.h"

namespace tvm {
namespace relay {

/*! Quick helper macro
 * - Expose a positional make function to construct the node.
 * - Register op to the registry.
 *
 * We make the decision to always only expose positional argument.
 * We will do rewrapping in the frontend to support language
 * sugars such as keyword arguments and default value.

 * \param OpName the name of registry.
 */
#define RELAY_REGISTER_UNARY_OP(OpName)                     \
  TVM_REGISTER_API("relay.op._make." OpName)                \
    .set_body_typed<Expr(Expr)>([](Expr data) {             \
        static const Op& op = Op::Get(OpName);              \
        return CallNode::make(op, {data}, Attrs(), {});     \
      });                                                   \
  RELAY_REGISTER_OP(OpName)                                 \
    .set_num_inputs(1)                                      \
    .add_argument("data", "Tensor", "The input tensor.")    \
    .add_type_rel("Identity", IdentityRel)                  \
    .set_attr<TOpPattern>("TOpPattern", kElemWise)          \
    .set_attr<TOpIsStateful>("TOpIsStateful", false)        \
    .set_attr<FInferCorrectLayout>("FInferCorrectLayout",   \
                                   ElemwiseArbitraryLayout) \


/*! Quick helper macro
 * - Expose a positional make function to construct the node.
 * - Register op to the registry.
 *
 * We make the decision to always only expose positional argument.
 * We will do rewrapping in the frontend to support language
 * sugars such as keyword arguments and default value.
 *
 * \param OpName the name of registry.
 */
#define RELAY_REGISTER_BINARY_OP(OpName)                          \
  TVM_REGISTER_API("relay.op._make." OpName)                      \
    .set_body_typed<Expr(Expr, Expr)>([](Expr lhs, Expr rhs) {    \
        static const Op& op = Op::Get(OpName);                    \
        return CallNode::make(op, {lhs, rhs}, Attrs(), {});       \
      });                                                         \
  RELAY_REGISTER_OP(OpName)                                       \
    .set_num_inputs(2)                                            \
    .add_argument("lhs", "Tensor", "The left hand side tensor.")  \
    .add_argument("rhs", "Tensor", "The right hand side tensor.") \
    .add_type_rel("Broadcast", BroadcastRel)                      \
    .set_attr<TOpPattern>("TOpPattern", kBroadcast)               \
    .set_attr<TOpIsStateful>("TOpIsStateful", false)              \
    .set_attr<FInferCorrectLayout>("FInferCorrectLayout",         \
                                   BinaryBroadcastLayout)

// Comparisons
#define RELAY_REGISTER_CMP_OP(OpName)                             \
  TVM_REGISTER_API("relay.op._make." OpName)                      \
  .set_body_typed<Expr(Expr, Expr)>([](Expr lhs, Expr rhs) {      \
    static const Op& op = Op::Get(OpName);                        \
    return CallNode::make(op, {lhs, rhs}, Attrs(), {});           \
  });                                                             \
  RELAY_REGISTER_OP(OpName)                                       \
    .set_num_inputs(2)                                            \
    .add_argument("lhs", "Tensor", "The left hand side tensor.")  \
    .add_argument("rhs", "Tensor", "The right hand side tensor.") \
    .add_type_rel("BroadcastComp", BroadcastCompRel)              \
    .set_attr<TOpPattern>("TOpPattern", kBroadcast)               \
    .set_attr<TOpIsStateful>("TOpIsStateful", false)              \
    .set_attr<FInferCorrectLayout>("FInferCorrectLayout",         \
                                   BinaryBroadcastLayout)


/*! \brief A helper class for matching and rewriting operators. */
template<typename R>
class OpMatch {
 public:
  using MatchFunc =
      std::function<R(const Array<Expr>& args, const Attrs& attrs, const Array<Type>& type_args)>;

  /*! \brief Match an operator with the given name.
   *  \param op_name The name of the operator to match.
   *  \param func The function to execute when it matches.
   *  \return A self-reference for builder style API.
   */
  inline OpMatch& Match(const std::string& op_name, MatchFunc func) {
    auto op = Op::Get(op_name);
    match_map_.insert({op, func});
    return *this;
  }

  /*! \brief Rewrite a call operation based on the operator and the registered
   *  match functions.
   * \param call The call to rewrite.
   * \return The result of rewriting.
   */
  inline R operator()(const Call& call) {
    auto it = match_map_.find(Downcast<Op>(call->op));
    if (it != match_map_.end()) {
      return it->second(call->args, call->attrs, call->type_args);
    } else {
      if (default_ != nullptr) {
        return default_(call->args, call->attrs, call->type_args);
      } else {
        LOG(FATAL) << "unexpected operation " << call->op;
      }
    }
  }

 private:
  /*! \brief The match function map. */
  std::unordered_map<Op, MatchFunc, NodeHash, NodeEqual> match_map_;
  /*! \brief An optional default case. */
  MatchFunc default_;
};

}  // namespace relay
}  // namespace tvm

#endif  // TVM_RELAY_OP_OP_COMMON_H_
