#ifndef HTTPSERVER_GLOBAL_H
#define HTTPSERVER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(HTTPSERVER_LIBRARY)
#  define HTTPSERVER_EXPORT Q_DECL_EXPORT
#else
#  define HTTPSERVER_EXPORT Q_DECL_IMPORT
#endif

#endif // HTTPSERVER_GLOBAL_H
