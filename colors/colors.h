enum Color
{
    RED,
    RED_BOLD,
    GREEN,
    GREEN_BOLD,
    YELLOW,
    YELLOW_BOLD,
    BLUE,
    BLUE_BOLD,
    MAGENTA,
    MAGENTA_BOLD,
    CYAN,
    CYAN_BOLD,
    RESET
};

static const char *Colors[] = { "[0;31m", "[1;31m", "[0;32m", "[1;32m", "[0;33m", "[01;33m", "[0;34m", "[1;34m", "[0;35m", "[1;35m", "[0;36m", "[1;36m", "[0m" };

void Colors_SetColor(enum Color c);