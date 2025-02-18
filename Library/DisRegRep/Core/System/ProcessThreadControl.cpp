#include <DisRegRep/Core/System/ProcessThreadControl.hpp>
#include <DisRegRep/Core/System/Platform.hpp>

#include <DisRegRep/Core/Exception.hpp>

#ifdef DRR_CORE_SYSTEM_PLATFORM_OS_WINDOWS
#include <DisRegRep/Core/System/ProcessThreadControlWindows.hpp>
#elifdef DRR_CORE_SYSTEM_PLATFORM_OS_POSIX
#include <DisRegRep/Core/System/ProcessThreadControlPosix.hpp>
#endif

#include <thread>

namespace ProcThrCtrl = DisRegRep::Core::System::ProcessThreadControl;
namespace Internal_ =
#ifdef DRR_CORE_SYSTEM_PLATFORM_OS_WINDOWS
ProcThrCtrl::Windows
#elifdef DRR_CORE_SYSTEM_PLATFORM_OS_POSIX
ProcThrCtrl::Posix
#endif
;

using std::jthread;

namespace {

[[nodiscard]] Internal_::ThreadType getNativeThread(jthread* const thread) noexcept {
	return thread ? thread->native_handle() : Internal_::getCurrentThread();
}

[[nodiscard]] Internal_::ProcessType getNativeProcess() noexcept {
	return Internal_::getCurrentProcess();
}

}

ProcThrCtrl::Priority ProcThrCtrl::getPriority(jthread* const thread) {
	return Internal_::getNativeThreadPriority(getNativeThread(thread));
}

void ProcThrCtrl::setPriority(const Priority priority, jthread* const thread) {
	DRR_ASSERT(priority >= PriorityPreset::Min && priority <= PriorityPreset::Max);
	Internal_::setNativeThreadPriority(getNativeThread(thread), priority);
}

ProcThrCtrl::AffinityMask ProcThrCtrl::getAffinityMask(jthread* const thread) {
	return Internal_::getNativeThreadAffinityMask(getNativeProcess(), getNativeThread(thread));
}

void ProcThrCtrl::setAffinityMask(const AffinityMask affinity_mask, jthread* const thread) {
	DRR_ASSERT(affinity_mask.any());
	DRR_ASSERT(jthread::hardware_concurrency() <= affinity_mask.size());
	Internal_::setNativeThreadAffinityMask(getNativeProcess(), getNativeThread(thread), affinity_mask);
}