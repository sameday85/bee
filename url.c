#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>

#include "common.h"
//
//https://curl.haxx.se/libcurl/c/example.html
//
static size_t write_file(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

static size_t write_string(void *ptr, size_t size, size_t nmemb, void *stream) {
  *((char*)ptr + size*nmemb) = '\0';
  strcat (stream, ptr);
  
  return size * nmemb;
}

//Download a given URL into a local file or buffer
long download(char *url, char *post_fields, char *content_type, char *output, bool to_file) {
    FILE *pagefile = NULL;
    struct curl_slist *headers = NULL;    
    long http_code = 0;
    
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl_handle = curl_easy_init();
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    
    if (post_fields) {
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_fields);
    }
    if (post_fields && content_type) { 
        char buff[512];
        sprintf(buff, "Content-Type: %s", content_type);
        headers = curl_slist_append(headers, buff);
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    }

    if (to_file) {
        pagefile = fopen(output, "wb");
        if (pagefile) {
            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_file);
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
        }
    }
    else {
        output[0]='\0';
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_string);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, output);
    }
    if (!to_file || pagefile) {
        CURLcode code = curl_easy_perform(curl_handle);
        if (CURLE_OK == code) {
            curl_easy_getinfo (curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
        }
    }
    if (pagefile) {
        fclose(pagefile);
    }
    if (headers)
        curl_slist_free_all(headers);
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();    
    
    return http_code;
}

bool parse_json(char *json, char *tag, char *value) {
    *value='\0';
    char *ptr = strstr(json, tag);
    if (ptr) {
        ptr += strlen (tag);
        char *ptr1 = strchr (ptr, '\"');
        char *ptr2 = strchr (ptr1 + 1, '\"');
        int len = ptr2 - ptr1 - 1;
        strncpy (value, ptr1 + 1, len);
        value[len]='\0';
    }
    return strlen(value) > 0 ? true : false;
}

long download_soundoftext(char *word_or_sentence, char *filename) {
    long code = 0;
    
    char post_data[512], buffer[2018];
    sprintf(post_data, "{\r\n\"engine\": \"Google\",\r\n\"data\": {\r\n\"text\": \"%s\",\r\n\"voice\": \"en-US\"}\r\n}\r\n", word_or_sentence);
    download("https://api.soundoftext.com/sounds", post_data, "application/json", buffer, false);
    
    char value[255];
    if (!parse_json(buffer, "\"id\"", value))
        return code;

    char url[255];
    sprintf(url, "https://api.soundoftext.com/sounds/%s", value);
    download(url, NULL, NULL, buffer, false);
    int wait = 8; //4 seconds
    while (!parse_json(buffer, "\"location\"", value)) {
        if (--wait < 0)
            break;
        usleep(500 * 1000);
        download(url, NULL, NULL, buffer, false);
    }
    if (strlen (value) > 0)
        code = download(value, NULL, NULL, filename, true);
    return code;
}
