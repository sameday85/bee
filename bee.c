#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "common.h"

#define WORDLEN       25
#define CLASSLEN      15
#define DEFLEN        255
#define SAMPLELEN     255
#define AUDIOLEN      255
#define LISTLEN       1000

#define DICT_VER_BASIC      0
#define GRADE_CEILING       255
#define BEECEILING          8

#define PRACTICE            0
#define QUIZ                1
#define PLACE               2

#define RC_UNKNOWN          0
#define RC_FINISHED_ALL     1
#define RC_QUIT             2


typedef struct tagWord
{
    int grade;
    int seq;
    char word[WORDLEN];
    char class[CLASSLEN];
    char def[DEFLEN];
    char sample[SAMPLELEN];
    char audio[AUDIOLEN];
} Word;

typedef struct tagStats
{
    int asked;
    int correct;
    int help;
    int progress [GRADE_CEILING];
    int suggest;
} Stats;

typedef struct tagContext {
    Word *list;
    int  *index;
    int  total;
    int  selected;
    int  selected_total;
    int  max_words;
    int  suggested_grade;
    int mode;
} Context;

int loadstats(Stats *info, char *filename) {
    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        return 0;
    }

    fread(info, sizeof(Stats), 1, fp);
    fclose(fp);
    return 0;
}

int savestats(Stats *info, char *filename) {
    FILE *fp;

    fp = fopen(filename, "w");
    if (fp == NULL) {
        return 0;
    }

    fwrite(info,sizeof(Stats), 1, fp);
    fclose(fp);
    return 0;
}

void changeclass(char src[], char dst[]) {
    if (strcmp(src,  "adj")==0) {
        strcpy(dst, "adjective");
    }
    else {
        strcpy(dst, src);
    }
}

void trim(char *str) {
    for (int i = 0; i < strlen(str); ++i) {
        if (str[i] == '\r' || str[i] == '\n') {
            str[i]='\0';
            break;
        }
    }
}

void print_word(Word *pword) {
    //if (pword->seq == 53)
    //printf ("[%d-%d]%s:%s:%s\n", pword->grade, pword->seq, pword->word, pword->class, pword->def);
}


/*
15.

ﬁsh

(noun)

a cold-blooded animal that lives in the water and that has a spine, gills and
usually ﬁns.
*/
int load_word(FILE *fp, Word *pword, int grade, int sequence) {
    char buff[1000];

    pword->grade = grade;
    pword->seq = sequence;
    //skip the blank line
    fgets(buff, sizeof(buff), fp);
    //read the word
    fgets(buff, sizeof(buff), fp);
    trim(buff);
    strcpy (pword->word, buff);
    //skip blank line
    fgets(buff, sizeof(buff), fp);
    //read the class
    fgets(buff, sizeof(buff), fp);
    trim(buff);
    strcpy (pword->class, buff);
    //skip blank line
    fgets(buff, sizeof(buff), fp);
    //read the definition
    fgets(buff, sizeof(buff), fp);
    trim(buff);
    strcpy (pword->def, buff);
    fgets(buff, sizeof(buff), fp);
    trim(buff);
    if (strlen(buff) > 0) {
        strcat (pword->def, " ");
        strcat (pword->def, buff);
    }

    //set the sample sentence
    strcpy(pword->sample, "Sorry, sample sentence is not avaiable");

    print_word(pword);
}

int loadV10 (FILE *fp, Word* list) {
    char *str, buff[1000];
    int wordcount = 0;

    int grade = 1, word_sequence = 1;
    str = fgets(buff, sizeof(buff), fp);
    while (str!= NULL) {
        trim(buff);
        char template[10];
        sprintf(template, "%d.", word_sequence);
        if (strcmp (buff, template) == 0) {
            load_word(fp, &list[wordcount], grade, word_sequence);
            ++wordcount;
            if (word_sequence++ >= 100) {
                word_sequence = 1;
                ++grade;
            }
        }
        str = fgets(buff, sizeof(buff), fp);
        trim(buff);
    }

    return wordcount;
}

int loadV11 (FILE *fp, Word* list) {
    char *str, buff[1000];
    int wordcount = 0;

    int grade = 0, word_sequence = 1;
    str = fgets(buff, sizeof(buff), fp);
    while (str!= NULL) {
        trim(buff);
        if (strlen (buff) <= 0)
            break;
        ++wordcount;
        list[wordcount].grade = 0;
        list[wordcount].seq   = wordcount;

        //word
        strcpy (list[wordcount].word, buff);
        //category
        fgets(buff, sizeof(buff), fp);
        trim(buff);
        strcpy (list[wordcount].class, buff);
        //definition
        fgets(buff, sizeof(buff), fp);
        trim(buff);
        strcpy (list[wordcount].def, buff);
        //example
        fgets(buff, sizeof(buff), fp);
        trim(buff);
        strcpy (list[wordcount].sample, buff);
        //audio
        fgets(buff, sizeof(buff), fp);
        trim(buff);
        strcpy (list[wordcount].audio, buff);
        //skip blank line
        fgets(buff, sizeof(buff), fp);

        str = fgets(buff, sizeof(buff), fp);
    }

    return wordcount;
}


//load word list from given file
//@return total number of words loaded
int load (char* filename, Word* list, int *ver) {
    char *str, buff[1000];
    int wordcount = 0;

    FILE *fp = fopen(filename, "r");
    if (fp==NULL) {
        printf("File does not exist\n");
        return wordcount;
    }
    str = fgets(buff, sizeof(buff), fp);
    if (str) {
        trim(buff);
        if (strcmp (buff, "#Ver1.0") == 0) {
            wordcount = loadV10(fp, list);
            *ver = DICT_VER_BASIC;
        }
        else if (strcmp (buff, "#Ver1.1") == 0) {
            wordcount = loadV11(fp, list);
            *ver = DICT_VER_BASIC + 1;
        }
        else {
            printf("Not supported file format\n");
        }
    }
    fclose (fp);

    return wordcount;
}

int play( char *audio_file) {
    char command[256];
    int status;

    /* create command to execute */
    sprintf( command, "omxplayer -o local %s >/dev/null", audio_file);

    /* play sound */
    status = system( command );
    return status;
}

/* speech.sh
---------------------------------------------------------------------------------------------------------------------------------
#!/bin/bash
say() { local IFS=+;/usr/bin/omxplayer >/dev/null "http://translate.google.com/translate_tts?ie=UTF-8&client=tw-ob&q=$*&tl=en"; }
say $*
---------------------------------------------------------------------------------------------------------------------------------
./speech.sh The jacket is red
pico2wave -w sentence.wav "The jacket is red" && omxplayer sentence.wav >/dev/null
*/
void read_sentence_online(char *sentence) {
    if (strlen(sentence) <= 0)
        return;

    char command[DEFLEN + SAMPLELEN];
    sprintf(command, "./speech.sh \"%s\"", sentence);
    system(command );
}

void read_sentence_offline(char *sentence) {
    if (strlen(sentence) <= 0)
        return;

    char command[DEFLEN + SAMPLELEN];
    sprintf(command, "pico2wave -w sentence.wav \"%s\" && omxplayer sentence.wav >/dev/null", sentence);
    system(command );
}
//http://ssl.gstatic.com/dictionary/static/sounds/oxford/persistent--_us_1.mp3
void read_word(char *word, char *mp3_url) {
    if (strlen(word) <= 0)
        return;
    char url[255];
    if (mp3_url)
        strcpy (url, mp3_url);
    else {
        sprintf(url, "http://ssl.gstatic.com/dictionary/static/sounds/oxford/%s--_us_1.mp3", word);
    }
    //cached voice file name (mp3)
    char voice[255];
    sprintf(voice, "voice/%s.mp3", word);

    bool mp3_available = false;
    FILE *file = fopen (voice, "r");
    if (file == NULL) {
        if (download(url, NULL, NULL, voice, true)==200) {
            mp3_available = true;
        }
        else if (download_soundoftext(word, voice) == 200) {
            mp3_available = true;
        }
    }
    else {
        fclose (file);
        mp3_available = true;
    }

    if (mp3_available) {
        play(voice);
    }
    else {
        //play mp3 online
        char command[255];
        sprintf(command, "omxplayer %s >/dev/null", url);
        system(command );
    }
}

int name_to_grade(char *filename) {
    int grade = 0, base = 10, max = GRADE_CEILING;
    for (int i = 0; i < strlen(filename); ++i) {
        grade += filename[i];
    }
    grade = base + (grade % (max - base));
    return grade;
}

int present(Context *context, Stats *currentinfo) {
    char buff[255];
    bool more = true;

    int ret = RC_FINISHED_ALL;
    while (more && context->selected < context->selected_total) {
        int wordnum = context->index[context->selected++];
        ++currentinfo->asked;
        bool failed_already = false;
        for (;;) {
            fseek(stdin,0,SEEK_END);
            read_word(context->list[wordnum].word, NULL);
            printf("[%d:%03d]Enter the word spelling: ", context->list[wordnum].grade, context->selected);
            fgets(buff, sizeof(buff), stdin);
            trim(buff);
            if (strcmp(buff, "c")==0) {
                read_sentence_online(context->list[wordnum].class);
                currentinfo->help++;
            }
            else if (strcmp(buff, "d")==0) {
                read_sentence_online(context->list[wordnum].def);
                currentinfo->help++;
            }
            else if (strcmp(buff, "r")==0) {
                if (strlen (context->list[wordnum].audio) > 0) {
                    read_word(context->list[wordnum].word, context->list[wordnum].audio);
                }
                else {
                    read_sentence_offline(context->list[wordnum].word);
                }
            }
            else if (strcmp(buff, "?")==0) {
                printf("%s\n", context->list[wordnum].word);
                failed_already=true;
                break;
            }
            else if (strcmp(buff, "q")==0) {
                more = false;
                failed_already=true;
                --currentinfo->asked;
                ret = RC_QUIT;
                break;
            }
            else if (strcmp(context->list[wordnum].word, buff)==0) {
                if (!failed_already) {
                    currentinfo->correct++;
                    play("perfect.wav");
                }
                else {
                    play("pass.mp3");
                }
                break;
            }
            else {
                failed_already=true;
                play("sorry.wav");
                if (context->mode == PLACE) {
                    break;
                }
            }
        }
    }
    return ret;
}

int do_practice (Context *context, int selected_grade, Stats *currentinfo) {
    for (int i = 0; i < context->total; ++i) {
        if ((selected_grade == 0) || context->list[i].grade == 0 || (context->list[i].grade == selected_grade)) {
            context->index[context->selected_total++]=i;
        }
    }
    if (context->selected < 0 || context->selected >= context->selected_total)
        context->selected = 0;
    return present (context, currentinfo);
}

int do_quiz(Context *context, int selected_grade, Stats *currentinfo) {
    context->selected = 0;
    for (int i = 0; i < context->total; ++i) {
        if ((selected_grade == 0) || context->list[i].grade == 0 || (context->list[i].grade == selected_grade)) {
            context->index[context->selected_total++]=i;
        }
    }
    srand(time(0));
    for (int i = 0; i < context->selected_total; ++i) {
        int sel1 = rand() % context->selected_total;
        int sel2 = rand() % context->selected_total;
        int tmp = context->index[sel1];
        context->index[sel1]=context->index[sel2];
        context->index[sel2]=tmp;
    }
    if (context->max_words > 0 && context->max_words < context->selected_total)
        context->selected_total = context->max_words;

    return present (context, currentinfo);
}

int do_placement(Context *context, Stats *currentinfo) {
    context->max_words = 10;
    context->suggested_grade = BEECEILING;
    for(int i=1; i<=BEECEILING;  i++) {
        memset(currentinfo, 0, sizeof(Stats));
        int status = do_quiz(context, i, currentinfo);
        if (status == RC_QUIT) {
            return RC_QUIT;
        }
        if (currentinfo-> correct<BEECEILING) {
            context->suggested_grade= i;
            break;
        }
        if (i<BEECEILING) {
            printf("you passed grade %d. Next we will test you grade %d words\n", i, i+1);
        }
        else{ 
            printf("you passed all grades! You should go to the next list.\n");
        }
    }
    return RC_FINISHED_ALL;
}

int main(int argc, char *argv[])
{
    Context context;
    Stats info, currentinfo;
    char dict_filename[255], stats_filename[255];
    char buff[255];
    int selected_grade;
    int dict_ver;
    int mode;

    memset (&context, 0, sizeof (context));
    //default dictionary
    strcpy(dict_filename, "dict/SpellingBee2018.txt");
    selected_grade = 3;

    mode = PRACTICE;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help")==0) {
            printf("%s [--gradeN] [--quiz/--place] dictionary_file_name\n", argv[0]);
            return 0;
        }
        else if (strncmp(argv[i], "--grade", 7) == 0) {
            selected_grade = argv[i][7]-'0';
        }
        else if (strcmp (argv[i], "--quiz") == 0) {
            mode = QUIZ;
        }
        else if (strcmp (argv[i], "--place")==0) {
            mode = PLACE;
        }
        else {
            strcpy (dict_filename, "dict/");
            strcat (dict_filename, argv[i]);
        }
    }
    if (mode == PLACE) {
        strcpy(dict_filename, "dict/SpellingBee2018.txt");
    }
    context.mode = mode;
    context.list = calloc(LISTLEN, sizeof (Word));
    context.total= load (dict_filename, context.list, &dict_ver);
    printf("Total %d words in %s\n", context.total, dict_filename);
    if (context.total <= 0) {
        free (context.list);
        return 1;
    }
    context.index= calloc(context.total, sizeof (int));
    if (dict_ver == DICT_VER_BASIC) {//SpellingBee2018.txt loaded
        if (selected_grade < 0 || selected_grade > 8)
            selected_grade = 3;
    }
    else {
        selected_grade = name_to_grade(dict_filename);
        for (int i = 0; i < context.total; ++i)
            context.list[i].grade = selected_grade;
    }
    printf("c-class,d-definition,r-read again,q-quit\n");
    printf("--------------------------------------------------------------\n");

    play("username.mp3");
    printf("Enter username: ");
    fgets(stats_filename, 20, stdin);
    trim(stats_filename);
    if (strlen(stats_filename) <= 0) {
        strcpy(stats_filename, "guest");
    }
    strcat(stats_filename, ".txt");
    memset(&info, 0, sizeof(info));
    loadstats(&info, stats_filename);
    context.selected = info.progress[selected_grade];

    memset(&currentinfo, 0, sizeof(currentinfo));
    int ret = RC_UNKNOWN;
    switch (mode) {
    case PRACTICE:
        ret = do_practice(&context, selected_grade, &currentinfo);
        break;
    case QUIZ:
        ret = do_quiz(&context, selected_grade, &currentinfo);
        break;
    case PLACE:
        ret = do_placement(&context, &currentinfo);
        break;
    }
    free (context.list);
    free (context.index);

    info.asked  += currentinfo.asked;
    info.correct+= currentinfo.correct;
    info.help   += currentinfo.help;
    if (mode == PRACTICE) {
        info.progress[selected_grade] = context.selected - 1;
    }
    savestats(&info, stats_filename);
    play("bye.wav");

    switch (mode) {
    case PRACTICE:
    case QUIZ:
        printf("your correct percentage or ratio was %d out of %d or %.3f%% today\n", currentinfo.correct, currentinfo.asked, (float)currentinfo.correct/currentinfo.asked*100);
        printf("you asked for help %d times out of %d possible times today. %f%% is your percentage for asking for help today\n", currentinfo.help, currentinfo.asked*2, (float)currentinfo.help/currentinfo.asked * 2 *100);
        printf("your correct percentage or ratio was %d out of %d or %.3f%%\n", info.correct, info.asked, (float)info.correct/info.asked*100);
        printf("you asked for help %d times out of %d possible times. %f%% is your percentage for asking for help\n", info.help, info.asked*2, (float)info.help/info.asked * 2*100);
        break;
    case PLACE:
        if (ret == RC_FINISHED_ALL && context.suggested_grade!=BEECEILING) {
            printf("your suggested grade level is %d\n", context.suggested_grade);
        }
        break;
    }

    return 0;
}

