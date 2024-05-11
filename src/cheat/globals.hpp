#pragma once
#include <memory>
#include "include/auth/c_user.hpp"


#if enable_auth
extern user::c_user* g_user;
#endif