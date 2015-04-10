#include "cg_local.h"
#include <pthread.h>

#ifdef __APPLE__
# include <curl/curl.h>
#else
# include "../../libs/curl/include/curl/curl.h"
#endif

#define WEB_URL "http://assets.timeruns.net"
#define USER_AGENT "ETrun-client"

int web_query(char *result, char *query) {
    CURL      *curl = NULL;
    CURLcode  res;
    char      fullQuery[512] = { 0 };
    char error[1024];

    if (!query) {
        return 1;
    }

    curl = curl_easy_init();
    if (curl) {
        // Prepare query
        strcpy(fullQuery, WEB_URL);

        // Append query
        strcat(fullQuery, "/");
        strcat(fullQuery, query);

        // Setup curl
        curl_easy_setopt(curl, CURLOPT_URL, fullQuery);
        // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        // curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5); // 5 secs timeout
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1); // Fail on error
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // Prevents curl to raise signals
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL); // SSL
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);// User-agent
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);// Don't verify the certificate
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);// Don't verify the host
        // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);// Don't verify the host
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != 0) {
            // Check timemout (code 28)
            if (res == CURLE_OPERATION_TIMEDOUT) {
                strcpy(result, "timeout");
            } else if (res == CURLE_HTTP_RETURNED_ERROR) {
                strcpy(result, "net error");
            } else {
                //strcpy(result, "unknown error");
                sprintf(result, "curl error: %s", error);
            }

            return qfalse;
        }
        return res;
    } else {
        return qfalse;
    }
}

static void *web_get_levelshot(void *data) {
    int            code;
    char result[512];
    char query[512];

    code = web_query(result, query);

    if (code == 0) {
        CG_Printf("%s: %s\n", GAME_VERSION, result);
    } else {
        CG_Printf("%s: failed to check API (code: %d, result: %s)\n", GAME_VERSION, code, result);
    }

    return NULL;
}

#define LEVELSHOT_HANDLER 0

qboolean web_async_call(int queryType) {
    pthread_t      thread;
    pthread_attr_t attr;
    void           *(*handler)(void *) = NULL;
    int            returnCode = 0;

    // Create threads as detached as they will never be joined
    if (pthread_attr_init(&attr)) {
        return qfalse;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
        return qfalse;
    }

    switch (queryType) {
        case LEVELSHOT_HANDLER:
          handler = web_get_levelshot;
          break;

        default:
          return qfalse;
    }

    returnCode = pthread_create(&thread, &attr, handler, NULL);

    if (returnCode) {
        return qfalse;
    }

    if (pthread_attr_destroy(&attr)) {
        return qfalse;
    }

    return qtrue;
}
