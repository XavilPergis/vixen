#pragma once

#ifdef TRACY_ENABLE

#include <Tracy.hpp>

#define VIXEN_PROF_SCOPE ZoneScoped;
#define VIXEN_PROF_SCOPE_NAMED(name) ZoneScopedN(name);
#define VIXEN_PROF_SCOPE_COLORED(color) ZoneScopedC(color);
#define VIXEN_PROF_SCOPE_NAMED_COLORED(name, color) ZoneScopedNC(name, color);

#define VIXEN_PROF_MESSAGE(view) TracyMessage((view).begin(), (view).len());
#define VIXEN_PROF_SET_SCOPE_NAME(view) ZoneName((view).begin(), (view).len());

#define VIXEN_PROF_ALLOC_EVENT_NAMED(ptr, size, name) TracyAllocN(ptr, size, name);
#define VIXEN_PROF_DEALLOC_EVENT_NAMED(ptr, name) TracyFreeN(ptr, name);

#else

#define VIXEN_PROF_SCOPE
#define VIXEN_PROF_SCOPE_NAMED(name)
#define VIXEN_PROF_SCOPE_COLORED(color)
#define VIXEN_PROF_SCOPE_NAMED_COLORED(name, color)

#define VIXEN_PROF_MESSAGE(view)
#define VIXEN_PROF_SET_SCOPE_NAME(view)

#define VIXEN_PROF_ALLOC_EVENT_NAMED(ptr, size, name)
#define VIXEN_PROF_DEALLOC_EVENT_NAMED(ptr, name)

#endif

#define VIXEN_PROF_COLOR_WAITING 0x444444
