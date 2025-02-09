/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSDOMGlobalObjectTask.h"

#include "ActiveDOMCallback.h"
#include "JSMainThreadExecState.h"
#include <heap/StrongInlines.h>
#include <wtf/Ref.h>

using namespace JSC;

namespace WebCore {

class JSGlobalObjectCallback FINAL : public RefCounted<JSGlobalObjectCallback>, private ActiveDOMCallback {
public:
    static PassRefPtr<JSGlobalObjectCallback> create(JSDOMGlobalObject* globalObject, GlobalObjectMethodTable::QueueTaskToEventLoopCallbackFunctionPtr functionPtr, PassRefPtr<TaskContext> taskContext)
    {
        return adoptRef(new JSGlobalObjectCallback(globalObject, functionPtr, taskContext));
    }

    void call()
    {
        if (!canInvokeCallback())
            return;

        Ref<JSGlobalObjectCallback> protect(*this);
        JSLockHolder lock(m_globalObject->vm());

        ExecState* exec = m_globalObject->globalExec();

        ScriptExecutionContext* context = m_globalObject->scriptExecutionContext();
        // We will fail to get the context if the frame has been detached.
        if (!context)
            return;

        // When on the main thread (e.g. the document's thread), we need to make sure to
        // push the current ExecState on to the JSMainThreadExecState stack.
        if (context->isDocument()) {
            JSMainThreadExecState currentState(exec);
            m_functionPtr(exec, m_taskContext.get());
        } else
            m_functionPtr(exec, m_taskContext.get());
    }

private:
    JSGlobalObjectCallback(JSDOMGlobalObject* globalObject, GlobalObjectMethodTable::QueueTaskToEventLoopCallbackFunctionPtr functionPtr, PassRefPtr<TaskContext> taskContext)
        : ActiveDOMCallback(globalObject->scriptExecutionContext())
        , m_globalObject(globalObject->vm(), globalObject)
        , m_functionPtr(functionPtr)
        , m_taskContext(taskContext)
    {
    }

    Strong<JSDOMGlobalObject> m_globalObject;
    GlobalObjectMethodTable::QueueTaskToEventLoopCallbackFunctionPtr m_functionPtr;
    RefPtr<TaskContext> m_taskContext;
};

JSGlobalObjectTask::JSGlobalObjectTask(JSDOMGlobalObject* globalObject, GlobalObjectMethodTable::QueueTaskToEventLoopCallbackFunctionPtr functionPtr, PassRefPtr<TaskContext> taskContext)
    : m_callback(JSGlobalObjectCallback::create(globalObject, functionPtr, taskContext))
{
}

JSGlobalObjectTask::~JSGlobalObjectTask()
{
}

void JSGlobalObjectTask::performTask(ScriptExecutionContext*)
{
    m_callback->call();
}

} // namespace WebCore
