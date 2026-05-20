#pragma once

#ifdef USE_LLENSE
#include <llense/libcurl/curl/curl.h>
#include <llense/libjansson/jansson.h>
#else
#include <curl/curl.h>
#include <jansson.h>
#endif
