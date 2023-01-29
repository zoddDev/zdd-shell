#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "utils.h"

struct passwd *Utils_GetCurrentUser()
{
    return getpwuid(getuid());
}