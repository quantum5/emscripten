#include "asan_internal.h"
#include "asan_mapping.h"
#include "asan_poisoning.h"
#include "asan_thread.h"

#if SANITIZER_EMSCRIPTEN
#include <emscripten.h>
#include <cstddef>

namespace __asan {

void InitializeShadowMemory() {
  // Poison the shadow memory of the shadow area at the start of the address
  // space. This helps catching null pointer dereference.
  FastPoisonShadow(kLowShadowBeg, kLowShadowEnd - kLowShadowBeg, 0xff);
}

void AsanCheckDynamicRTPrereqs() {}
void AsanCheckIncompatibleRT() {}
void InitializePlatformInterceptors() {}
void InitializePlatformExceptionHandlers() {}
bool IsSystemHeapAddress (uptr addr) { return false; }

void *AsanDoesNotSupportStaticLinkage() {
  // On Linux, this is some magic that fails linking with -static.
  // On Emscripten, we have to do static linking, so we stub this out.
  return nullptr;
}

void InitializeAsanInterceptors() {}

// We can use a plain thread_local variable for TSD.
static thread_local void *per_thread;

void *AsanTSDGet() { return per_thread; }

void AsanTSDSet(void *tsd) { per_thread = tsd; }

// There's no initialization needed, and the passed-in destructor
// will never be called.  Instead, our own thread destruction hook
// (below) will call AsanThread::TSDDtor directly.
void AsanTSDInit(void (*destructor)(void *tsd)) {
  DCHECK(destructor == &PlatformTSDDtor);
}

void PlatformTSDDtor(void *tsd) { UNREACHABLE(__func__); }

} // namespace __asan

namespace __lsan {

#ifndef USE_THREADS
// XXX HACK: Emscripten treats thread_local variables the same as globals in
// non-threaded builds, so a hack was introduced where we skip the allocator
// cache in the common module. Now we have to define this symbol to keep that
// hack working when using LSan as part of ASan without threads.
void GetAllocatorCacheRange(uptr *begin, uptr *end) {
  *begin = *end = 0;
}
#endif

u32 GetCurrentThread() { return __asan::GetCurrentThread()->tid(); }

} // namespace __lsan

#endif // SANITIZER_EMSCRIPTEN
