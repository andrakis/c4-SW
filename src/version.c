const char *game_version = "(not set)";
#ifdef C4SW
// c4lc has no __DATE__ or __TIME__, and c4's images are the same bytes
// each time they are built: the build is named by its source instead
const char *game_date = "c4";
const char *game_time = "games/sw";
#else
const char *game_date = __DATE__;
const char *game_time = __TIME__;
#endif
