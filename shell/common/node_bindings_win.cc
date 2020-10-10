// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_bindings_win.h"

#include <windows.h>

#include "base/logging.h"
#include "base/system/sys_info.h"

namespace electron {

NodeBindingsWin::NodeBindingsWin(BrowserEnvironment browser_env)
    : NodeBindings(browser_env) {
  // on single-core the io comp port NumberOfConcurrentThreads needs to be 2
  // to avoid cpu pegging likely caused by a busy loop in PollEvents
  if (base::SysInfo::NumberOfProcessors() == 1) {
    // the expectation is the uv_loop_ has just been initialized
    // which makes iocp replacement safe
    CHECK_EQ(0u, uv_loop_->active_handles);
    CHECK_EQ(0u, uv_loop_->active_reqs.count);

    if (uv_loop_->iocp && uv_loop_->iocp != INVALID_HANDLE_VALUE)
      CloseHandle(uv_loop_->iocp);
    uv_loop_->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);
  }
}

NodeBindingsWin::~NodeBindingsWin() {}

void NodeBindingsWin::PollEvents() {
  auto block = false;
  auto timeout = static_cast<DWORD>(uv_backend_timeout(uv_loop_));
  auto startTime = ::GetTickCount();

  do {
    block =
        uv_loop_->idle_handles == NULL && uv_loop_->pending_reqs_tail == NULL &&
        uv_loop_->endgame_handles == NULL && uv_loop_->active_handles == 0 &&
        !uv_loop_->stop_flag && uv_loop_->active_reqs.count == 0;
    ::Sleep(100);
    if (timeout > 0 && (::GetTickCount() - startTime) > timeout)
      break;
  } while (block);
}

// static
NodeBindings* NodeBindings::Create(BrowserEnvironment browser_env) {
  return new NodeBindingsWin(browser_env);
}

}  // namespace electron
