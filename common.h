#ifndef bool

#define bool        int
#define false        0 //must be zero
#define true         1

#endif


extern long download(char *url, char *post_fields, char *content_type, char *output, bool to_file);
extern long download_soundoftext(char *word_or_sentence, char *filename);
