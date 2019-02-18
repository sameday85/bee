#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define WORDLEN       25
#define CLASSLEN      15
#define DEFLEN        255
#define SAMPLELEN     255
#define AUDIOLEN      255
#define LISTLEN       1000

#define DICT_VER_BASIC      0

#define bool        int
#define true        1
#define false       0


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
    float possiblehelp;
    int progress [255];
} Stats;

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

//https://stackoverflow.com/questions/24321295/how-can-i-download-a-file-using-c-socket-programming
//only http protocol supported
bool download_url (char *url, char *filename) {
    char *prefix="http://";
    if (strncmp(url, prefix, strlen(prefix)) != 0)
        return false;

    struct addrinfo hints, *servinfo;
    char domain[255];

    char *request = strchr(url + strlen(prefix) + 1, '/');
    int len = request - url - strlen(prefix);
    strncpy (domain, url + strlen(prefix), len);
    domain[len]='\0';

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(domain, "http", &hints, &servinfo) != 0) {
        return false;
    }

    int total_len = 0;
    //Create socket
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc != -1) {
        //Connect to remote server
        if (connect(socket_desc, (struct sockaddr *)servinfo->ai_addr, servinfo->ai_addrlen) == 0) {
            //Send request
            char message[255];
            sprintf(message, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", request, domain);
            if (send(socket_desc, message, strlen(message), 0) > 0) {
                FILE *file = fopen(filename, "w");
                if (file) {
                    bool error = true;
                    int   max_len = 1024 * 1024;  //1mb
                    char *reply=malloc(max_len);
                    //http response header
                    int len = recv(socket_desc, reply, max_len, 0);
                    if (len > 0) {
                        if (strstr(reply, "HTTP/1.1 200 OK") == reply) {
                            char *ptr = strstr (reply, "\r\n\r\n");
                            if (ptr) {
                                ptr += 4;
                                len -= ptr - reply;
                                fwrite(ptr, len, 1, file);
                                total_len += len;

                                error = false;
                            }
                        }
                    }
                    if (error == false) {
                        len = recv(socket_desc, reply, max_len, 0);
                        while (len > 0) {
                            fwrite(reply, len, 1, file);
                            total_len += len;
                            len = recv(socket_desc, reply, max_len, 0);
                        }
                    }
                    free(reply);
                    fclose(file);
                    if (error)
                        remove(filename);
                }
            }
        }
        shutdown (socket_desc, SHUT_RD);
    }
    freeaddrinfo(servinfo);

    return total_len > 0 ? true : false;
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
        if (download_url(url, voice)) {
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
    int grade = 0, base = 100, max = 255;
    for (int i = 0; i < strlen(filename); ++i) {
        grade += filename[i];
    }
    grade = base + (grade % (max - base));
    
    return grade;
}

int main(int argc, char *argv[])
{

    Stats info;
    char more;
    char buff[255];
    char stats_filename[255];
    char filename[255];
    int wordnum;
    int selected_grade;
    int dict_ver;
    Word *list;
    int  index[LISTLEN];
    //dictionary file path
    char dict[255];
    bool quiz_mode;


    quiz_mode=false;
    //default dictionary
    strcpy(dict, "dict/SpellingBee2018.txt");
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
        else if (strcmp (argv[i], "--quiz") == 0) {
            quiz_mode = true;
        }
        else {
            strcpy (dict, "dict/");
            strcat (dict, argv[i]);
        }
    }
    list = calloc(LISTLEN, sizeof (Word));
    int total = load (dict, list, &dict_ver);
    printf("Total %d words in %s\n", total, dict);
    if (total <= 0) {
        free (list);
        return 1;
    }
    if (dict_ver == DICT_VER_BASIC) {//SpellingBee2018.txt loaded
        if (selected_grade < 0 || selected_grade > 8)
            selected_grade = 3;
    }
    else {
        selected_grade = name_to_grade(dict);
    }
    printf("c-class,d-definition,r-read again,q-quit, clear-clear progress\n");
    printf("--------------------------------------------------------------\n");

    int selected_total = 0;
    //initalize the random number generator
    srand(time(0));
    for (int i = 0; i < total; ++i) {
        if (selected_grade == 0 || list[i].grade == 0 || list[i].grade == selected_grade) {
            index[selected_total++]=i;
        }
    }

    if (quiz_mode == true) {
        for (int i = 0; i < selected_total; ++i) {
            int sel1 = rand() % selected_total;
            int sel2 = rand() % selected_total;
            int tmp = index[sel1];
            index[sel1]=index[sel2];
            index[sel2]=tmp;
        }
    }

    more = 'h';
    read_sentence_online("enter username");
    printf("Enter username: ");

    fgets(stats_filename, sizeof(stats_filename) - 4, stdin);
    trim(stats_filename);
    if (strlen(stats_filename) <= 0) {
        strcpy(stats_filename, "anonymous.txt");
    }
    strcat(stats_filename, ".txt");
    memset(&info, 0, sizeof(info));
    loadstats(&info, stats_filename);


    int select = info.progress[selected_grade];
    if (select < 0 || select >= selected_total)
        select = 0;

    while (more != 'q' && select++ < selected_total) {
        wordnum = index[select];
        ++info.asked;
        bool donotcount = false;
        for (;;) {
            fseek(stdin,0,SEEK_END);
            read_word(list[wordnum].word, NULL);
            int word_grade = dict_ver == DICT_VER_BASIC ? list[wordnum].grade : selected_grade;
            printf("[%d:%03d]Enter the word spelling: ", word_grade, select);
            fgets(buff, sizeof(buff), stdin);
            trim(buff);
            if (strcmp(buff, "c")==0) {
                read_sentence_online(list[wordnum].class);
                info.help++;
            }
            else if (strcmp(buff, "d")==0) {
                read_sentence_online(list[wordnum].def);
                info.help++;
            }

            else if (strcmp(buff, "r")==0) {
                if (strlen (list[wordnum].audio) > 0) {
                    read_word(list[wordnum].word, list[wordnum].audio);
                }
                else {
                    read_sentence_offline(list[wordnum].word);
                }
            }
            else if (strcmp(buff, "?")==0) {
                printf("%s\n", list[wordnum].word);
                donotcount=true;
                break;
            }
            else if (strcmp(buff, "q")==0) {
                more = 'q';
                donotcount=true;
                --info.asked;
                break;
            }
            else if (strcmp(buff, "clear")==0) {
                remove(stats_filename);
                memset(&info, 0, sizeof(info));
                info.asked++;
            }
            else if (strcmp(list[wordnum].word, buff)==0) {
                if (!donotcount) {
                    info.correct++;
                    play("perfect.wav");
                }
                else {
                    read_sentence_online("you finally passed the question! Yay!");
                }
                break;
            }
            else {
                donotcount=true;
                play("sorry.wav");
            }
        }
    }
    
    info.progress[selected_grade]=select;
    savestats(&info, stats_filename);
    play("bye.wav");
    free (list);
    info.possiblehelp=(float)info.asked*2;
    printf("your correct percentage or ratio was %d out of %d or %.3f%%\n", info.correct, info.asked, (float)info.correct/info.asked*100);
    printf("you asked for help %d times out of %d possible times. %f%% is your percentage for asking for help\n", info.help, info.asked*2, (float)info.help/info.possiblehelp*100);

    return 0;
}
