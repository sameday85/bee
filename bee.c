#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WORDLEN       25
#define CLASSLEN      15
#define DEFLEN        255
#define SAMPLELEN     255
#define LISTLEN       1000

typedef struct tagWord
{
    int grade;
    int seq;
    char word[WORDLEN];
    char class[CLASSLEN];
    char def[DEFLEN];
    char sample[SAMPLELEN];
} Word;
typedef struct tagstats
{
    int asked, answered;
    int select, correct;
    int retry, help;
    float correctratio ;

} Stats;

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

//load word list from given file
//@return total number of words loaded
int load (char* filename, Word* list) {
    FILE *fp;
    char *str, buff[1000];
    int wordcount = 0;


    fp = fopen(filename, "r");
    if (fp==NULL) {
        printf("File does not exist\n");
        return wordcount;
    }

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
    fclose(fp);

    return wordcount;
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
void read(char *sentence) {
    if (strlen(sentence) <= 0)
        return;

    char command[DEFLEN + SAMPLELEN];
    sprintf(command, "./speech.sh \"%s\"", sentence);
    system(command );
}

void read2(char *sentence) {
    if (strlen(sentence) <= 0)
        return;

    char command[DEFLEN + SAMPLELEN];
    sprintf(command, "pico2wave -w sentence.wav \"%s\" && omxplayer sentence.wav >/dev/null", sentence);
    system(command );
}

void read_word(char *word) {
    if (strlen(word) <= 0)
        return;

    char command[255];
    sprintf(command, "omxplayer http://ssl.gstatic.com/dictionary/static/sounds/oxford/%s--_us_1.mp3 >/dev/null", word);
    system(command );
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


int main(int argc, char *argv[])
{
    Stats info;
    char more;
    char buff[255];
    int wordnum;
    int selected_grade;
    Word *list;
    int  index[LISTLEN];
    //dictionary file path
    char dict[255];
    //default dictionary
    strcpy(dict, "SpellingBee2018.txt");
    selected_grade = 3;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help")==0) {
            //print help menu
            printf("%s --gradeN dictionary_file_name\n", argv[0]);
            return 0;
        }
        else if (strncmp(argv[i], "--grade", 7) == 0) {
            selected_grade = argv[i][7]-'0';
        }
        else {
            strcpy (dict, argv[i]);
        }
    }
    if (selected_grade < 1 || selected_grade > 8)
        selected_grade = 5;
    list = calloc(LISTLEN, sizeof (Word));
    int total = load (dict, list);
    printf("Total %d words in %s\n", total, dict);
    if (total <= 0) {
        free (list);
        return 1;
    }

    int selected_total = 0;
    //initalize the random number generator
    srand(time(0));
    for (int i = 0; i < total; ++i) {
        if (selected_grade == 0 || list[i].grade == selected_grade) {
            index[selected_total++]=i;
        }
    }
    for (int i = 0; i < selected_total; ++i) {
        int sel1 = rand() % selected_total;
        int sel2 = rand() % selected_total;
        int tmp = index[sel1];
        index[sel1]=index[sel2];
        index[sel2]=tmp;
    }

    more = 'h';
    int select = 0;
    memset(&info, 0, sizeof(info));
    while (more != 'q' && select++ < selected_total) {
        wordnum = index[select];
        ++info.asked;
        int failed = 0;
        for (;;) {
            fseek(stdin,0,SEEK_END);
            read_word(list[wordnum].word);
            printf("[%d]Enter the word spelling(c-class,d-definition,r-read again,q-quit): ", list[wordnum].grade);
            scanf("%s", &buff );
            if (strcmp(buff, "c")==0) {
                read(list[wordnum].class);
                info.help++;
            }
            else if (strcmp(buff, "d")==0)
            {
                read(list[wordnum].def);
                info.help++;
            }

            else if (strcmp(buff, "r")==0) {
                read(list[wordnum].word);
                info.help++;
            }
            else if (strcmp(buff, "?")==0) {
                printf("%s\n", list[wordnum].word);
                ++failed;
                break;
            }
            else if (strcmp(buff, "q")==0) {
                more = 'q';

                break;
            }
            else if (strcmp(list[wordnum].word, buff)==0) {
                play("perfect.wav");
                if (info.retry != 1) {
                    info.correct++;
                }
                info.retry = 0;
                break;
            }
            else {
                ++failed;
                play("sorry.wav");
                info.retry = 1;
            }
        }
        info.answered++;
    }
    info.select += select;

    info.correctratio = (float)info.correct/info.answered;
    play("bye.wav");
    free (list);
    printf("your correct percentage or ratio was %d out of %d or %.3f%\n", info.correct, info.answered, info.correctratio*100   );
    return 0;
}

