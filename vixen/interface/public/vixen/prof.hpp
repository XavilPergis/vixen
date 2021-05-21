#pragma once

#include <Tracy.hpp>

#define VIXEN_PROF_SCOPE ZoneScoped;
#define VIXEN_PROF_SCOPE_NAMED(name) ZoneScopedN(name);
#define VIXEN_PROF_SCOPE_COLORED(color) ZoneScopedC(color);

#define VIXEN_PROF_MESSAGE(view) TracyMessage((view).begin(), (view).len());
#define VIXEN_PROF_SET_SCOPE_NAME(view) ZoneName((view).begin(), (view).len());

#define VIXEN_PROF_COLOR_WAITING 0x444444
