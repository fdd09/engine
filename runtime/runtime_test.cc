// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/runtime/runtime_test.h"

#include "flutter/runtime/dart_vm.h"
#include "flutter/testing/testing.h"

namespace flutter {
namespace testing {

RuntimeTest::RuntimeTest()
    : native_resolver_(std::make_shared<TestDartNativeResolver>()),
      assets_dir_(fml::OpenDirectory(GetFixturesPath(),
                                     false,
                                     fml::FilePermission::kRead)),
      aot_symbols_(LoadELFSymbolFromFixturesIfNeccessary()) {}

void RuntimeTest::SetSnapshotsAndAssets(Settings& settings) {
  if (!assets_dir_.is_valid()) {
    return;
  }

  settings.assets_dir = assets_dir_.get();

  // In JIT execution, all snapshots are present within the binary itself and
  // don't need to be explicitly supplied by the embedder. In AOT, these
  // snapshots will be present in the application AOT dylib.
  if (DartVM::IsRunningPrecompiledCode()) {
    PrepareSettingsForAOTWithSymbols(settings, aot_symbols_);
  } else {
    settings.application_kernels = [this]() {
      std::vector<std::unique_ptr<const fml::Mapping>> kernel_mappings;
      kernel_mappings.emplace_back(
          fml::FileMapping::CreateReadOnly(assets_dir_, "kernel_blob.bin"));
      return kernel_mappings;
    };
  }
}

Settings RuntimeTest::CreateSettingsForFixture() {
  Settings settings;
  settings.leak_vm = false;
  settings.task_observer_add = [](intptr_t, fml::closure) {};
  settings.task_observer_remove = [](intptr_t) {};
  settings.isolate_create_callback = [this]() {
    native_resolver_->SetNativeResolverForIsolate();
  };
  SetSnapshotsAndAssets(settings);
  return settings;
}

void RuntimeTest::AddNativeCallback(std::string name,
                                    Dart_NativeFunction callback) {
  native_resolver_->AddNativeCallback(std::move(name), callback);
}

}  // namespace testing
}  // namespace flutter
