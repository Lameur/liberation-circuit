/*
 * Liberation Circuit - Compiler Warning Fixes and Compatibility
 * 
 * This header provides macros and utilities to fix common compiler warnings
 * and improve cross-compiler compatibility.
 */

#ifndef COMPILER_FIXES_H
#define COMPILER_FIXES_H

/* ================================================================
 * COMPILER DETECTION
 * ================================================================ */

#ifdef __clang__
    #define COMPILER_CLANG 1
    #define COMPILER_GCC 0
    #define COMPILER_MSVC 0
#elif defined(__GNUC__) && !defined(__clang__)
    #define COMPILER_CLANG 0
    #define COMPILER_GCC 1
    #define COMPILER_MSVC 0
#elif defined(_MSC_VER)
    #define COMPILER_CLANG 0
    #define COMPILER_GCC 0
    #define COMPILER_MSVC 1
#else
    #define COMPILER_CLANG 0
    #define COMPILER_GCC 0
    #define COMPILER_MSVC 0
#endif

/* ================================================================
 * WARNING SUPPRESSION MACROS
 * ================================================================ */

#if COMPILER_GCC
    #define SUPPRESS_WARNING_PUSH _Pragma("GCC diagnostic push")
    #define SUPPRESS_WARNING_POP  _Pragma("GCC diagnostic pop")
    #define SUPPRESS_MISLEADING_INDENTATION _Pragma("GCC diagnostic ignored \"-Wmisleading-indentation\"")
    #define SUPPRESS_UNUSED_VARIABLE _Pragma("GCC diagnostic ignored \"-Wunused-variable\"")
    #define SUPPRESS_UNUSED_PARAMETER _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"")
    #define SUPPRESS_UNUSED_FUNCTION _Pragma("GCC diagnostic ignored \"-Wunused-function\"")
    #define SUPPRESS_IMPLICIT_FUNCTION_DECL _Pragma("GCC diagnostic ignored \"-Wimplicit-function-declaration\"")
    #define SUPPRESS_SIGN_COMPARE _Pragma("GCC diagnostic ignored \"-Wsign-compare\"")
    #define SUPPRESS_FORMAT_TRUNCATION _Pragma("GCC diagnostic ignored \"-Wformat-truncation\"")
#elif COMPILER_CLANG
    #define SUPPRESS_WARNING_PUSH _Pragma("clang diagnostic push")
    #define SUPPRESS_WARNING_POP  _Pragma("clang diagnostic pop")
    #define SUPPRESS_MISLEADING_INDENTATION _Pragma("clang diagnostic ignored \"-Wmisleading-indentation\"")
    #define SUPPRESS_UNUSED_VARIABLE _Pragma("clang diagnostic ignored \"-Wunused-variable\"")
    #define SUPPRESS_UNUSED_PARAMETER _Pragma("clang diagnostic ignored \"-Wunused-parameter\"")
    #define SUPPRESS_UNUSED_FUNCTION _Pragma("clang diagnostic ignored \"-Wunused-function\"")
    #define SUPPRESS_IMPLICIT_FUNCTION_DECL _Pragma("clang diagnostic ignored \"-Wimplicit-function-declaration\"")
    #define SUPPRESS_SIGN_COMPARE _Pragma("clang diagnostic ignored \"-Wsign-compare\"")
    #define SUPPRESS_FORMAT_TRUNCATION /* Not applicable to Clang */
#elif COMPILER_MSVC
    #define SUPPRESS_WARNING_PUSH __pragma(warning(push))
    #define SUPPRESS_WARNING_POP  __pragma(warning(pop))
    #define SUPPRESS_MISLEADING_INDENTATION /* Not applicable to MSVC */
    #define SUPPRESS_UNUSED_VARIABLE __pragma(warning(disable: 4101))
    #define SUPPRESS_UNUSED_PARAMETER __pragma(warning(disable: 4100))
    #define SUPPRESS_UNUSED_FUNCTION __pragma(warning(disable: 4505))
    #define SUPPRESS_IMPLICIT_FUNCTION_DECL __pragma(warning(disable: 4013))
    #define SUPPRESS_SIGN_COMPARE __pragma(warning(disable: 4018))
    #define SUPPRESS_FORMAT_TRUNCATION /* Not applicable to MSVC */
#else
    #define SUPPRESS_WARNING_PUSH
    #define SUPPRESS_WARNING_POP
    #define SUPPRESS_MISLEADING_INDENTATION
    #define SUPPRESS_UNUSED_VARIABLE
    #define SUPPRESS_UNUSED_PARAMETER
    #define SUPPRESS_UNUSED_FUNCTION
    #define SUPPRESS_IMPLICIT_FUNCTION_DECL
    #define SUPPRESS_SIGN_COMPARE
    #define SUPPRESS_FORMAT_TRUNCATION
#endif

/* ================================================================
 * CONVENIENCE MACROS FOR COMMON WARNING PATTERNS
 * ================================================================ */

/* Use this around code blocks with intentional misleading indentation */
#define BEGIN_MISLEADING_INDENTATION_OK \
    SUPPRESS_WARNING_PUSH \
    SUPPRESS_MISLEADING_INDENTATION

#define END_MISLEADING_INDENTATION_OK \
    SUPPRESS_WARNING_POP

/* Use this for intentionally unused variables */
#define UNUSED_VARIABLE(var) \
    SUPPRESS_WARNING_PUSH \
    SUPPRESS_UNUSED_VARIABLE \
    (void)(var); \
    SUPPRESS_WARNING_POP

/* Use this for parameters that are intentionally unused */
#define UNUSED_PARAMETER(param) ((void)(param))

/* Mark functions as potentially unused (for static functions) */
#if COMPILER_GCC || COMPILER_CLANG
    #define MAYBE_UNUSED __attribute__((unused))
#else
    #define MAYBE_UNUSED
#endif

/* ================================================================
 * PLATFORM-SPECIFIC FIXES
 * ================================================================ */

/* Windows-specific fixes */
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    
    /* Suppress common Windows warnings */
    #if COMPILER_MSVC
        #pragma warning(disable: 4996) /* 'function': was declared deprecated */
        #pragma warning(disable: 4244) /* conversion from 'type1' to 'type2', possible loss of data */
        #pragma warning(disable: 4267) /* conversion from 'size_t' to 'type', possible loss of data */
        #pragma warning(disable: 4305) /* truncation from 'type1' to 'type2' */
    #endif
    
    /* Define missing POSIX functions on Windows */
    #define strcasecmp _stricmp
    #define strncasecmp _strnicmp
    #define snprintf _snprintf
    #define vsnprintf _vsnprintf
#endif

/* ================================================================
 * ALLEGRO-SPECIFIC FIXES
 * ================================================================ */

/* Include Allegro headers with warning suppression */
#define INCLUDE_ALLEGRO_SAFELY() \
    SUPPRESS_WARNING_PUSH \
    SUPPRESS_UNUSED_PARAMETER \
    SUPPRESS_SIGN_COMPARE \
    _Pragma("GCC diagnostic ignored \"-Wredundant-decls\"") \
    _Pragma("GCC diagnostic ignored \"-Wpedantic\"")

#define END_ALLEGRO_INCLUDE() \
    SUPPRESS_WARNING_POP

/* ================================================================
 * UTILITY MACROS FOR COMMON CODE PATTERNS
 * ================================================================ */

/* Safe string copy with bounds checking */
#define SAFE_STRCPY(dest, src, size) do { \
    strncpy((dest), (src), (size) - 1); \
    (dest)[(size) - 1] = '\0'; \
} while(0)

/* Safe string concatenation with bounds checking */
#define SAFE_STRCAT(dest, src, size) do { \
    size_t dest_len = strlen(dest); \
    if (dest_len < (size) - 1) { \
        strncpy((dest) + dest_len, (src), (size) - dest_len - 1); \
        (dest)[(size) - 1] = '\0'; \
    } \
} while(0)

/* Array size calculation */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Min/Max macros that avoid double evaluation */
#define SAFE_MIN(a, b) ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b; \
})

#define SAFE_MAX(a, b) ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b; \
})

/* Fallback for compilers that don't support __typeof__ */
#if !COMPILER_GCC && !COMPILER_CLANG
    #undef SAFE_MIN
    #undef SAFE_MAX
    #define SAFE_MIN(a, b) ((a) < (b) ? (a) : (b))
    #define SAFE_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* ================================================================
 * DEBUGGING AND ASSERTIONS
 * ================================================================ */

#ifdef DEBUG_MODE
    #include <assert.h>
    #define ASSERT(condition) assert(condition)
    #define DEBUG_PRINT(fmt, ...) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
    #define ASSERT(condition) ((void)0)
    #define DEBUG_PRINT(fmt, ...) ((void)0)
#endif

/* Static assertion for compile-time checks */
#if COMPILER_GCC || COMPILER_CLANG
    #define STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#elif COMPILER_MSVC
    #define STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
    #define STATIC_ASSERT(condition, message) typedef char static_assertion_##__LINE__[(condition) ? 1 : -1]
#endif

/* ================================================================
 * NETWORK-SPECIFIC FIXES
 * ================================================================ */

#ifdef NETWORK_ENABLED
    /* Windows socket compatibility */
    #ifdef _WIN32
        #include <winsock2.h>
        #include <ws2tcpip.h>
        typedef SOCKET socket_t;
        typedef int socklen_t;
        #define CLOSE_SOCKET(s) closesocket(s)
        #define SOCKET_ERROR_LAST WSAGetLastError()
        #define SOCKET_WOULD_BLOCK WSAEWOULDBLOCK
        #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #else
        #include <sys/socket.h>
        #include <netinet/in.h>
        #include <arpa/inet.h>
        #include <unistd.h>
        #include <errno.h>
        typedef int socket_t;
        #define CLOSE_SOCKET(s) close(s)
        #define SOCKET_ERROR_LAST errno
        #define SOCKET_WOULD_BLOCK EWOULDBLOCK
        #define INVALID_SOCKET_VALUE -1
    #endif
#endif

/* ================================================================
 * COMPILER-SPECIFIC OPTIMIZATIONS
 * ================================================================ */

/* Branch prediction hints */
#if COMPILER_GCC || COMPILER_CLANG
    #define LIKELY(x)   __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define LIKELY(x)   (x)
    #define UNLIKELY(x) (x)
#endif

/* Function attributes */
#if COMPILER_GCC || COMPILER_CLANG
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
    #define NEVER_INLINE __attribute__((noinline))
    #define PURE_FUNCTION __attribute__((pure))
    #define CONST_FUNCTION __attribute__((const))
#elif COMPILER_MSVC
    #define ALWAYS_INLINE __forceinline
    #define NEVER_INLINE __declspec(noinline)
    #define PURE_FUNCTION
    #define CONST_FUNCTION
#else
    #define ALWAYS_INLINE inline
    #define NEVER_INLINE
    #define PURE_FUNCTION
    #define CONST_FUNCTION
#endif

/* ================================================================
 * COMPATIBILITY FIXES FOR OLDER COMPILERS
 * ================================================================ */

/* C99 compatibility */
#ifndef __STDC_VERSION__
    #define inline
    #define restrict
#elif __STDC_VERSION__ < 199901L
    #define inline
    #define restrict
#endif

/* Boolean type for C89 compatibility */
#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L)
    typedef enum { false = 0, true = 1 } bool;
#endif

#endif /* COMPILER_FIXES_H */