#pragma once


#ifdef _DEBUG
#define  ASSERT_ALWAYS(expr, HELP_msg, ...) \
    do { if (!(expr) && \
        (1 == _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, NULL, "{{{{ " HELP_msg " }}}}", __VA_ARGS__))) \
        __debugbreak(); } while (0,0)
#else
#define  ASSERT_ALWAYS(expr, HELP_msg, ...) \
    do { if(!expr) {MessageBox(GetActiveWindow(), HELP_msg, "Error", MB_OK); __debugbreak();}  } while (0,0)
#endif

