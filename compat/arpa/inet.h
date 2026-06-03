#ifndef YAKISOBA_COMPAT_ARPA_INET_H
#define YAKISOBA_COMPAT_ARPA_INET_H

#if defined(_WIN32)
#  include <winsock2.h>
#else
#  include_next <arpa/inet.h>
#endif

#endif
