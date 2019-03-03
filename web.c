#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "common.h"

void trim(char *str) {
    for (int i = 0; i < strlen(str); ++i) {
        if (str[i] == '\r' || str[i] == '\n') {
            str[i]='\0';
            break;
        }
    }
}

void parse_category(char *content, char*category) {
    char *tag = "<span class=\"pos\">";
    
    char *ptr = strstr(content, tag);
    if (ptr) {
        ptr += strlen (tag);
        char *ptr2 = strstr(ptr, "</span>");
        int len = ptr2 - ptr;
        strncpy (category, ptr, len);
        category[len]='\0';
    }
}

void parse_definitions(char *content, char *word, char *definitions) {
    char tag[255];
    sprintf(tag, "Definition of %s - ", word);
    char *ptr = strstr(content, tag);
    if (ptr) {
        ptr += strlen (tag);
        char *ptr2 = strstr(ptr, "\"");
        int len = ptr2 - ptr;
        strncpy (definitions, ptr, len);
        definitions[len]='\0';
        
        char *result = str_replace(definitions,"&#39;", "'");
        if (result) {
            strcpy (definitions, result);
            free(result);
        }
    }
}

void parse_example(char *content, char *example) {
    char *tag="<div class=\"exg\">";
    char *ptr = strstr(content, tag);
    if (ptr) {
        ptr = strstr(ptr, "&lsquo;");
        ptr += 7;
        char *ptr2 = strstr (ptr, "&rsquo;");
        int len = ptr2 - ptr;
        strncpy (example, ptr, len);
        example[len]='\0';
    }
}

void parse_audio(char *content, char *audio) {
    char *tag = "<audio src='";
    char *ptr = strstr(content, tag);
    if (ptr) {
        ptr += strlen (tag);
        char *ptr2 = strstr(ptr, "\'");
        int len = ptr2 - ptr;
        strncpy (audio, ptr, len);
        audio[len]='\0';
    }
}

int call (char *word, char *category, char *definitions, char *example, char *audio) {
    char *content = malloc(1024 * 1024 * 5); //5mb
    if (!content)
        printf("Out of memory\n");
    
    category[0]=definitions[0]=example[0]=audio[0]='\0';
    
    char url[255];
    sprintf(url, "https://en.oxforddictionaries.com/definition/%s", word);
    download(url, NULL, NULL, content, false);
    parse_category(content, category);
    parse_definitions(content, word, definitions);
    parse_example(content, example);
    parse_audio(content, audio);

    free(content);
}

//gcc -o web web.c url.c -lcurl
int main(int argc, char *argv[])
{
    char filename[255];
    char output[255];
    char category[50];
    char definitions[2048];
    char example[255];
    char audio[255];
    char buff[255];
    char *version="#Ver1.1\r\n";
    bool done = false;

    strcpy (filename, "dict/Dutch.txt");
    strcpy (output, "dict/SpellingBeeDutch.txt");
    if (argc > 1) {
        if (strcmp(argv[1], "--test") == 0) {
            strcpy (buff, "dum");
            call(buff, category, definitions, example, audio);
            printf("%s:%s,%s(%s)\n", buff, category, definitions, example);
            printf("%s\n", audio);

            done = true;
        }
        else {
            strcpy (filename, "dict/");
            strcat (filename, argv[1]);
            
            strcpy (output, "dict/SpellingBee");
            strcat (output, argv[1]);
        }
    }
    if (done)
        return 0;
    
    char *str;
    FILE *fp = fopen(filename, "r");
    if (fp==NULL) {
        printf("File does not exist: %s\n", filename);
        return 1;
    }
    
    FILE *fpOutput = fopen(output, "w");
    fwrite (version, strlen(version), 1, fpOutput);

    
    str = fgets(buff, sizeof(buff), fp);
    while (str!= NULL) {
        trim(buff);
        call(buff, category, definitions, example, audio);
        printf("%s:%s,%s[%s]\n", buff, category, definitions, example);
        
        //word
        fwrite (buff, strlen(buff), 1, fpOutput);
        fwrite ("\r\n", 2, 1, fpOutput);
        //category
        fwrite (category, strlen(category), 1, fpOutput);
        fwrite ("\r\n", 2, 1, fpOutput);
        //definition
        fwrite (definitions, strlen(definitions), 1, fpOutput);
        fwrite ("\r\n", 2, 1, fpOutput);
        //example
        fwrite (example, strlen(example), 1, fpOutput);
        fwrite ("\r\n", 2, 1, fpOutput);
        //audio
        fwrite (audio, strlen(audio), 1, fpOutput);
        fwrite ("\r\n", 2, 1, fpOutput);
        //end
        fwrite ("\r\n", 2, 1, fpOutput);

        str = fgets(buff, sizeof(buff), fp);
    }
    fclose(fp);
    fclose (fpOutput);
    
    return 0;
}
