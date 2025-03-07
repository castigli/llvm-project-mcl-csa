//===- ReleaseModeModelRunner.h - Fast, precompiled model runner  ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements a model runner wrapping an AOT compiled ML model.
// Only inference is supported.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_RELEASEMODEMODELRUNNER_H
#define LLVM_ANALYSIS_RELEASEMODEMODELRUNNER_H

#include "llvm/Analysis/MLModelRunner.h"
#include "llvm/Support/ErrorHandling.h"

#include <memory>
#include <vector>

using namespace llvm;
namespace llvm {

/// ReleaseModeModelRunner - production mode implementation of the
/// MLModelRunner. It uses an AOT-compiled SavedModel for efficient execution.
template <class TGen>
class ReleaseModeModelRunner final : public MLModelRunner {
public:
  /// FeatureNames' type should be an indexed collection of std::string, like
  /// std::array or std::vector, that has a size() method.
  template <class FType>
  ReleaseModeModelRunner(LLVMContext &Ctx, const FType &FeatureNames,
                         StringRef DecisionName, StringRef FeedPrefix = "feed_",
                         StringRef FetchPrefix = "fetch_")
      : MLModelRunner(Ctx, MLModelRunner::Kind::Release),
        CompiledModel(std::make_unique<TGen>()) {
    assert(CompiledModel && "The CompiledModel should be valid");

    const size_t FeatureCount = FeatureNames.size();
    FeatureIndices.resize(FeatureCount);

    for (size_t I = 0; I < FeatureCount; ++I) {
      const int Index =
          CompiledModel->LookupArgIndex(FeedPrefix.str() + FeatureNames[I]);
      assert(Index >= 0 && "Cannot find Feature in inlining model");
      FeatureIndices[I] = Index;
    }

    ResultIndex = CompiledModel->LookupResultIndex(FetchPrefix.str() +
                                                   DecisionName.str());
    assert(ResultIndex >= 0 && "Cannot find DecisionName in inlining model");
  }

  virtual ~ReleaseModeModelRunner() = default;

  static bool classof(const MLModelRunner *R) {
    return R->getKind() == MLModelRunner::Kind::Release;
  }

private:
  void *evaluateUntyped() override {
    CompiledModel->Run();
    return CompiledModel->result_data(ResultIndex);
  }

  void *getTensorUntyped(size_t Index) override {
    return reinterpret_cast<char *>(
        CompiledModel->arg_data(FeatureIndices[Index]));
  }

  std::vector<int32_t> FeatureIndices;
  int32_t ResultIndex = -1;
  std::unique_ptr<TGen> CompiledModel;
};

/// A mock class satisfying the interface expected by ReleaseModeModelRunner for
/// its `TGen` parameter. Useful to avoid conditional compilation complexity, as
/// a compile-time replacement for a real AOT-ed model.
class NoopSavedModelImpl final {
  const char *ErrMsg = "The mock AOT-ed saved model is a compile-time stub and "
                       "should not be called.";

public:
  NoopSavedModelImpl() = default;
  int LookupArgIndex(const std::string &) { llvm_unreachable(ErrMsg); }
  int LookupResultIndex(const std::string &) { llvm_unreachable(ErrMsg); }
  void Run() { llvm_unreachable(ErrMsg); }
  void *result_data(int) { llvm_unreachable(ErrMsg); }
  void *arg_data(int) { llvm_unreachable(ErrMsg); }
};
} // namespace llvm

#endif // LLVM_ANALYSIS_RELEASEMODEMODELRUNNER_H
