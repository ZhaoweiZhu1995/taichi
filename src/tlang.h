#pragma once

#include <taichi/common/util.h>
#include <taichi/io/io.h>
namespace taichi {
namespace math {
inline int maximum(int a) {
  return a;
}
}  // namespace math
}  // namespace taichi
#include <taichi/math.h>
#include <set>
#include <dlfcn.h>

#include "util.h"
#include "math.h"
#include "program.h"

TLANG_NAMESPACE_BEGIN

inline void layout(const std::function<void()> &body) {
  get_current_program().layout(body);
}

inline Kernel kernel(const std::function<void()> &body) {
  return get_current_program().kernel(body);
}

inline void kernel_name(std::string name) {
  get_current_program().get_current_kernel().name = name;
}

/*
inline void touch(SNode *snode, Expr target_index, Expr value) {
  auto e = Expr::create(NodeType::touch, Expr::load_if_pointer(target_index),
                        Expr::load_if_pointer(value));
  e->snode_ptr(0) = snode;
  auto &ker = get_current_program().get_current_kernel();
  ker.has_touch = true;
  return ker.ret->ch.push_back(e);
}

inline void touch(Expr &expr, Expr target_index, Expr value) {
  return taichi::Tlang::touch(expr->snode_ptr(0)->parent, target_index, value);
}

inline void reduce(Expr target, Expr value) {
  TC_ASSERT(target->type == NodeType::pointer);
  auto e = Expr::create(NodeType::reduce, target, Expr::load_if_pointer(value));
  auto &ker = get_current_program().get_current_kernel();
  return ker.ret->ch.push_back(e);
}
*/

template <typename T>
inline void declare_var(Expr &a) {
  current_ast_builder().insert(std::make_unique<FrontendAllocaStmt>(
      std::static_pointer_cast<IdExpression>(a.expr)->id, get_data_type<T>()));
}

inline void declare_unnamed_var(Expr &a, DataType dt) {
  auto id = Identifier();
  auto a_ = Expr::make<IdExpression>(id);

  current_ast_builder().insert(std::make_unique<FrontendAllocaStmt>(id, dt));

  if (a.expr) {
    a_ = a;  // assign
  }

  a.set(a_);
}

inline void declare_var(Expr &a) {
  current_ast_builder().insert(std::make_unique<FrontendAllocaStmt>(
      std::static_pointer_cast<IdExpression>(a.expr)->id, DataType::unknown));
}

inline Expr Expr::operator[](ExpressionGroup indices) {
  TC_ASSERT(is<GlobalVariableExpression>());
  return Expr::make<GlobalPtrExpression>(cast<GlobalVariableExpression>(),
                                         indices);
}

#define Declare(x) auto x = Expr(std::make_shared<IdExpression>(#x));

#define var(type, x) declare_var<type>(x);

#define Local(x)  \
  Declare(x);     \
  declare_var(x); \
  x

#define Global(x, dt)  \
  Declare(x##_global); \
  auto x = global_new(x##_global, DataType::dt);

#define AmbientGlobal(x, dt, ambient)            \
  Declare(x##_global);                           \
  auto x = global_new(x##_global, DataType::dt); \
  set_ambient(x, ambient);

inline void set_ambient(Expr expr_, float32 val) {
  auto expr = expr_.cast<GlobalVariableExpression>();
  expr->ambient_value = TypedConstant(val);
  expr->has_ambient = true;
}

inline void set_ambient(Expr expr_, int32 val) {
  auto expr = expr_.cast<GlobalVariableExpression>();
  expr->ambient_value = TypedConstant(val);
  expr->has_ambient = true;
}

inline Expr global_new(Expr id_expr, DataType dt) {
  TC_ASSERT(id_expr.is<IdExpression>());
  auto ret = Expr(std::make_shared<GlobalVariableExpression>(
      dt, id_expr.cast<IdExpression>()->id));
  return ret;
}

inline Expr global_new(DataType dt) {
  auto id_expr = std::make_shared<IdExpression>();
  return Expr::make<GlobalVariableExpression>(dt, id_expr->id);
}

template <typename T>
inline Expr Rand() {
  return Expr::make<RandExpression>(get_data_type<T>());
}

template <typename T>
inline T Eval(const T &t) {
  return t.eval();
}

inline Expr copy(const Expr &expr) {
  auto e = expr.eval();
  auto stmt = Stmt::make<ElementShuffleStmt>(
      VectorElement(e.cast<EvalExpression>()->stmt_ptr, 0));
  auto eval_expr = std::make_shared<EvalExpression>(stmt.get());
  current_ast_builder().insert(std::move(stmt));
  return Expr(eval_expr);
}

template <typename... indices>
std::vector<Index> Indices(indices... ind) {
  auto ind_vec = std::vector<int>({ind...});
  std::vector<Index> ret;
  for (auto in : ind_vec) {
    ret.push_back(Index(in));
  }
  return ret;
}

inline Expr Atomic(Expr dest) {
  // NOTE: dest must be passed by value so that the original
  // expr will not be modified into an atomic one.
  dest.atomic = true;
  return dest;
}

inline void Activate(const Expr &dest, const ExpressionGroup &expr_group) {
  current_ast_builder().insert(
      Stmt::make<FrontendSNodeOpStmt>(SNodeOpType::activate, dest, expr_group));
}

inline void Deactivate(const Expr &dest, const ExpressionGroup &expr_group) {
  current_ast_builder().insert(Stmt::make<FrontendSNodeOpStmt>(
      SNodeOpType::deactivate, dest, expr_group));
}

inline Expr Probe(const Expr &dest, const ExpressionGroup &expr_group) {
  TC_NOT_IMPLEMENTED;
  current_ast_builder().insert(
      Stmt::make<FrontendSNodeOpStmt>(SNodeOpType::probe, dest, expr_group));
}

TLANG_NAMESPACE_END

TC_NAMESPACE_BEGIN
void write_partio(std::vector<Vector3> positions, const std::string &file_name);
TC_NAMESPACE_END
