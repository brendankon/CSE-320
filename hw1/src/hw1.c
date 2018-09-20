#include <stdlib.h>

#include "debug.h"
#include "hw1.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the content of three frames of audio data and
 * two annotation fields have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */
unsigned int hexVal = 0;

int str_compare(char* char1, char* char2){

    while(*(char1) != '\0' && *(char2) != '\0'){
        if(*(char1) != *(char2))
            return 0;
        char1++;
        char2++;
    }

    if(*(char1) != *(char2))
        return 0;
    return 1;
}

int strIntConvert(char* word){
    int i = 0;

    for(char *p = word; *p; p++){
        i = i * 10 + (*p - '0');
    }

    return i;
}

int hexCheck(char *input){
    int count = 0;
    while(*input){
        char c = *(input);
        if('A' <= c && c <= 'F'){
            c = c - 'A' + 10;
            count++;
        }
        else if('a' <= c && c <= 'f'){
            c = c - 'a' + 10;
            count++;
        }
        else if('0' <= c && c <= '9'){
            c = c - '0';
            count++;
        }
        else{
            hexVal = 0;
            return 0;
        }
        hexVal = (hexVal << 4) | (c & 0xF);
        input++;
    }

    if(count < 9)
        return 1;
    return 0;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 1 if validation succeeds and 0 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variables "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 1 if validation succeeds and 0 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv)
{
    int aCount = 2;
    argv++;
    if(str_compare(*(argv),"-h")){
        global_options = global_options ^ 0x8000000000000000;
        return 1;
    }

    if(argc > 5 || argc < 2)
        return 0;

    if(str_compare(*(argv),"-u") || str_compare(*(argv),"-d") ){
        if(str_compare(*(argv), "-u"))
            global_options = global_options ^  0x4000000000000000;
        else
            global_options = global_options ^  0x2000000000000000;
        argv++;
        aCount++;
        if(aCount > argc || *(argv) == NULL){
            return 1;
        }
        if(str_compare(*(argv),"-f")){
            argv++;
            aCount++;
            if(aCount > argc || *(argv) == NULL)
                return 0;
            if(strIntConvert(*(argv)) < 1 || strIntConvert(*(argv)) > 1024){
                global_options = 0;
                return 0;
            }
            int f = strIntConvert(*(argv));
            f--;
            unsigned long factor = (unsigned long)f << 48;
            global_options = global_options ^ factor;
            argv++;
            aCount++;
            if(aCount > argc || *(argv) == NULL)
                return 1;
            else if(str_compare(*(argv), "-p")){
                global_options = global_options ^ 0x800000000000000;
                return 1;
            }
            global_options = 0;
            return 0;
        }

        if(str_compare(*(argv), "-p")){
            global_options = global_options ^ 0x800000000000000;
            argv++;
            aCount++;
            if(aCount > argc || *(argv) == NULL)
                return 1;
            if(str_compare(*(argv), "-f")){
                argv++;
                aCount++;
                if(strIntConvert(*(argv)) < 1 || strIntConvert(*(argv)) > 1024){
                    global_options = 0;
                    return 0;
                }
                unsigned long factor = (unsigned long)strIntConvert(*(argv));
                factor--;
                factor = factor << 48;
                global_options = global_options ^ factor;
                return 1;
            }
            global_options = 0;
            return 0;
        }
        global_options = 0;
        return 0;
    }

    if(str_compare(*(argv), "-c")){
        global_options = global_options ^ 0x1000000000000000;
        argv++;
        aCount++;
        if(aCount > argc || *(argv) == NULL){
            global_options = 0;
            return 0;
        }

        if(str_compare(*(argv), "-k")){
            argv++;
            aCount++;
            if(hexCheck(*(argv))){
                global_options = global_options ^ hexVal;
                argv++;
                aCount++;
                if(aCount > argc || *(argv) == NULL){
                    return 1;
                }
                else if(str_compare(*(argv), "-p")){
                    global_options = global_options ^ 0x0800000000000000;
                    return 1;
                }

                global_options = 0;
                return 0;
            }
            global_options = 0;
            return 0;
        }

        if(str_compare(*(argv), "-p")){
            global_options = global_options ^ 0x0800000000000000;
            argv++;
            aCount++;
            if(str_compare(*(argv), "-k")){
                argv++;
                aCount++;
                if(hexCheck(*(argv))){
                    global_options = global_options ^ hexVal;
                    argv++;
                    aCount++;
                    if(aCount > argc || *(argv) == NULL)
                        return 1;
                    global_options = 0;
                    return 0;
                }
                global_options = 0;
                return 0;
            }
            global_options = 0;
            return 0;
        }
    }
    global_options = 0;
    return 0;
}


/**
 * @brief  Recodes a Sun audio (.au) format audio stream, reading the stream
 * from standard input and writing the recoded stream to standard output.
 * @details  This function reads a sequence of bytes from the standard
 * input and interprets it as digital audio according to the Sun audio
 * (.au) format.  A selected transformation (determined by the global variable
 * "global_options") is applied to the audio stream and the transformed stream
 * is written to the standard output, again according to Sun audio format.
 *
 * @param  argv  Command-line arguments, for constructing modified annotation.
 * @return 1 if the recoding completed successfully, 0 otherwise.
 */
unsigned int x;
int recode(char **argv) {
    AUDIO_HEADER h;
    AUDIO_HEADER *hp;
    hp = &h;
    read_header(hp);

    unsigned long one = 1;
    char *ap;
    ap = input_annotation;
    x = 0;

    if(!(global_options & (one << 59))){
        while(*argv){
            while(*(*(argv))){
                *(ap) = *(*(argv));
                (*(argv))++;
                ap++;
                x++;
            }
            *(ap) = ' ';
            ap++;
            argv++;
            x++;
        }

        if(((hp->data_offset)-24)+x > ANNOTATION_MAX){
            return 0;
        }
        if((hp->data_offset)-24 != 0){
            *(ap) = '\n';
            ap++;
            x++;
        }

    }
    int temp = 0;
    while((x+temp) %8 != 0){
        temp++;
    }
    x = x + temp;
    write_header(hp);
    x = x - temp;

    unsigned int aSize = (hp->data_offset)-24;
    read_annotation(ap, aSize);
    ap = ap - x;
    aSize = aSize + x;
    write_annotation(ap, aSize);

    while((aSize)%8 != 0){
        putchar('\0');
        aSize++;
    }

    if(global_options & (one << 62)){
        unsigned long mask = (unsigned long) ((1 << 10)-1) << 48;
        unsigned long l = global_options & mask;
        int factor = (l >> 48) + 1;
        int bytes_per_sample = hp->encoding - 1;
        int frames = (hp->data_size)/(hp->channels * bytes_per_sample);
        int fCount = 0;
        while(fCount < frames){
            int *fp = (int *) input_frame;
            int channels = hp->channels;
            read_frame(fp, channels, bytes_per_sample);
            if(fCount % factor == 0){
                write_frame(fp, channels, bytes_per_sample);
            }
            fCount++;
        }
    }

    if(global_options & (one << 61)){
        unsigned long mask = (unsigned long) ((1 << 10)-1) << 48;
        unsigned long l = global_options & mask;
        int factor = (l >> 48) + 1;
        int bytes_per_sample = hp->encoding - 1;
        int frames = (hp->data_size)/(hp->channels * bytes_per_sample);
        int fCount = 0;
        int channels = hp->channels;
        int *pp = (int *) previous_frame;
        int *ip = (int *) input_frame;
        int *op = (int *) output_frame;
        read_frame(pp, channels, bytes_per_sample);
        write_frame(pp, channels, bytes_per_sample);
        while(fCount < frames){
            int f = 1;
            read_frame(ip, channels, bytes_per_sample);
            while(f < factor){
                if(channels == 1){
                    int v1 = (signed)*(pp);
                    int v2 = (signed)*(ip);
                    int v3 = v1 + (v2 - v1)*f/factor;
                    *(op) = v3;
                    write_frame(op, channels, bytes_per_sample);
                }
                if(channels == 2){
                    int v1 = *(pp);
                    int v2 = *(ip);
                    int v3 = v1 + (v2 - v1)*f/factor;
                    *(op) = v3;
                    op++;
                    pp++;
                    ip++;
                    v1 = *(pp);
                    v2 = *(ip);
                    v3 = v1 + (v2 - v1)*f/factor;
                    *(op) = v3;
                    op--;
                    write_frame(op, channels, bytes_per_sample);
                    ip--;
                    pp--;
                }
                f++;
            }
            write_frame(ip, channels, bytes_per_sample);
            *(pp) = *(ip);
            pp++;
            ip++;
            *(pp) = *(ip);
            pp--;
            ip--;
            fCount++;
        }
    }

    if(global_options & (one << 60)){

        int bytes_per_sample = hp->encoding - 1;
        int frames = (hp->data_size)/(hp->channels * bytes_per_sample);
        int fCount = 0;
        int channels = hp->channels;
        int *ip = (int *) input_frame;
        int *op = (int *) output_frame;
        int k = global_options & 0xFFFFFFFF;
        mysrand(k);
        while(fCount < frames){
            read_frame(ip, channels, bytes_per_sample);
            if(channels == 1){
                int num = myrand32();
                *(op) = *(ip) ^ num;
                write_frame(op, channels, bytes_per_sample);
            }
            if(channels == 2){
                int num = myrand32();
                *(op) = *(ip) ^ num;
                op++;
                ip++;
                num = myrand32();
                *(op) = *(ip) ^ num;
                op--;
                ip--;
                write_frame(op, channels, bytes_per_sample);
            }
            fCount++;
        }
    }
    return 1;
}

int read_header(AUDIO_HEADER *hp){
    int mNum = convInt();
    if(mNum != 779316836)
        return 0;
    hp->magic_number = mNum;

    unsigned int offset = convInt();
    if(offset - 24 > ANNOTATION_MAX)
        return 0;
    if(offset % 8 != 0)
        return 0;
    hp->data_offset = offset;
    hp->data_size = (unsigned int) convInt();
    unsigned int encoding = convInt();
    if(encoding < 2 || encoding > 5)
        return 0;
    hp->encoding = encoding;
    hp->sample_rate = convInt();
    unsigned int channels = convInt();
    hp->channels = channels;
    return 1;
}

int write_header(AUDIO_HEADER *hp){
    unsigned int m = hp->magic_number;
    putchar((m >> 24)&0xff);
    putchar((m >> 16)&0xff);
    putchar((m >> 8)&0xff);
    putchar(m & 0xff);

    unsigned int off = hp->data_offset;
    off =  off + x;
    putchar((off >> 24)&0xff);
    putchar((off >> 16)&0xff);
    putchar((off >> 8)&0xff);
    putchar(off & 0xff);

    unsigned long one = 1;
    unsigned int size = hp->data_size;
    unsigned long mask = (unsigned long) ((1 << 10)-1) << 48;
    unsigned long l = global_options & mask;
    int factor = (l >> 48) + 1;
    if(global_options & (one << 62))
        size = size / factor;
    if(global_options & (one << 61))
        size = size * factor;
    putchar((size >> 24)&0xff);
    putchar((size >> 16)&0xff);
    putchar((size >> 8)&0xff);
    putchar(size & 0xff);

    unsigned int encode = hp->encoding;
    putchar((encode >> 24)&0xff);
    putchar((encode >> 16)&0xff);
    putchar((encode >> 8)&0xff);
    putchar(encode & 0xff);

    unsigned int rate = hp->sample_rate;
    putchar((rate >> 24)&0xff);
    putchar((rate >> 16)&0xff);
    putchar((rate >> 8)&0xff);
    putchar(rate & 0xff);

    unsigned int chan = hp->channels;
    putchar((chan >> 24)&0xff);
    putchar((chan >> 16)&0xff);
    putchar((chan >> 8)&0xff);
    putchar(chan & 0xff);

    return 1;
}

int read_annotation(char *ap, unsigned int size){

    int count = 0;

    if(size == 0){
        *(ap) = '\0';
        return 1;
    }

    while(count < size-1){
        *(ap) = getchar();
        count++;
        ap++;
    }

    char end = getchar();
    if(end == '\0'){
        *(ap) = end;
        return 1;
    }

    return 0;
}

int write_annotation(char *ap, unsigned int size){
    unsigned int count = 0;
    while(count < size){
        putchar(*(ap));
        ap++;
        count++;
    }
    return 1;
}

int read_frame(int *fp, int channels, int bytes_per_sample){
    unsigned long one = 1;

    if(channels == 1){
        if(bytes_per_sample == 1){
            if(global_options & (one << 61)){
                char c = getchar();
                int value = c;
                *(fp) = value;
                return 1;
            }
            else{
                unsigned char c = getchar();
                int value = c;
                *(fp) = value;
                return 1;
            }
        }
        if(bytes_per_sample == 2){
            if(global_options & (one << 61)){
                char c1 = getchar();
                char c2 = getchar();
                int value = (c1 << 8) | c2;
                *(fp) = value;
                return 1;
            }
            else{
                unsigned char c1 = getchar();
                unsigned char c2 = getchar();
                int value = (c1 << 8) | c2;
                *(fp) = value;
                return 1;
            }
        }
        if(bytes_per_sample == 3){
            if(global_options & (one << 61)){
                char c1 = getchar();
                char c2 = getchar();
                char c3 = getchar();
                int value = (c1 << 16) | (c2 << 8) | c3;
                *(fp) = value;
                return 1;
            }
            else{
                unsigned char c1 = getchar();
                unsigned char c2 = getchar();
                unsigned char c3 = getchar();
                int value = (c1 << 16) | (c2 << 8) | c3;
                *(fp) = value;
                return 1;
            }
        }
        if(bytes_per_sample == 4){
            if(global_options & (one << 61)){
                char c1 = getchar();
                char c2 = getchar();
                char c3 = getchar();
                char c4 = getchar();
                int value = (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
                *(fp) = value;
                return 1;
            }
            else{
                unsigned char c1 = getchar();
                unsigned char c2 = getchar();
                unsigned char c3 = getchar();
                unsigned char c4 = getchar();
                int value = (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
                *(fp) = value;
                return 1;
            }
        }
    }

    if(channels == 2){
        if(bytes_per_sample == 1){
            if(global_options & (one << 61)){
                char c1 = getchar();
                char c2 = getchar();
                int value = c1;
                *(fp) = value;
                fp++;
                value = c2;
                *(fp) = value;
                return 1;
            }
            else{
                unsigned char c1 = getchar();
                unsigned char c2 = getchar();
                int value = c1;
                *(fp) = value;
                fp++;
                value = c2;
                *(fp) = value;
                return 1;
            }
        }
        if(bytes_per_sample == 2){
            if(global_options & (one << 61)){
                char c1 = getchar();
                char c2 = getchar();
                char c3 = getchar();
                char c4 = getchar();
                int value =  (c1 << 8) | c2;
                *(fp) = value;
                fp++;
                value = (c3 << 8) | c4;
                *(fp) = value;
                return 1;
            }
            else{
                unsigned char c1 = getchar();
                unsigned char c2 = getchar();
                unsigned char c3 = getchar();
                unsigned char c4 = getchar();
                int value =  (c1 << 8) | c2;
                *(fp) = value;
                fp++;
                value = (c3 << 8) | c4;
                *(fp) = value;
                return 1;
            }
        }
        if(bytes_per_sample == 3){
            if(global_options & (one << 61)){
                char c1 = getchar();
                char c2 = getchar();
                char c3 = getchar();
                char c4 = getchar();
                char c5 = getchar();
                char c6 = getchar();
                int value = (c1 << 16) | (c2 << 8) | c3;
                *(fp) = value;
                fp++;
                value = (c4 << 16) | (c5 << 8) | c6;
                *(fp) = value;
                return 1;
            }
            else{
                unsigned char c1 = getchar();
                unsigned char c2 = getchar();
                unsigned char c3 = getchar();
                unsigned char c4 = getchar();
                unsigned char c5 = getchar();
                unsigned char c6 = getchar();
                int value = (c1 << 16) | (c2 << 8) | c3;
                *(fp) = value;
                fp++;
                value = (c4 << 16) | (c5 << 8) | c6;
                *(fp) = value;
                return 1;
            }
        }
        if(bytes_per_sample == 4){
            if(global_options & (one << 61)){
                char c1 = getchar();
                char c2 = getchar();
                char c3 = getchar();
                char c4 = getchar();
                char c5 = getchar();
                char c6 = getchar();
                char c7 = getchar();
                char c8 = getchar();
                int value = (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
                *(fp) = value;
                fp++;
                value = (c5 << 24) | (c6 << 16) | (c7 << 8) | c8;
                *(fp) = value;
                return 1;
            }
            else{
                unsigned char c1 = getchar();
                unsigned char c2 = getchar();
                unsigned char c3 = getchar();
                unsigned char c4 = getchar();
                unsigned char c5 = getchar();
                unsigned char c6 = getchar();
                unsigned char c7 = getchar();
                unsigned char c8 = getchar();
                int value = (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
                *(fp) = value;
                fp++;
                value = (c5 << 24) | (c6 << 16) | (c7 << 8) | c8;
                *(fp) = value;
                return 1;
            }
        }
    }
    return 0;
}

int write_frame(int *fp, int channels, int bytes_per_sample){
    if(channels == 1){
        if(bytes_per_sample == 1){
            unsigned char c = *(fp) & 0xff;
            putchar(c);
            return 1;
        }
        if(bytes_per_sample == 2){
            unsigned char c1 = (*(fp) >> 8) & 0xff;
            unsigned char c2 = *(fp) & 0xff;
            putchar(c1);
            putchar(c2);
            return 1;
        }
        if(bytes_per_sample == 3){
            unsigned char c1 = (*(fp) >> 16) & 0xff;
            unsigned char c2 = (*(fp) >> 8) & 0xff;
            unsigned char c3 = *(fp) & 0xff;
            putchar(c1);
            putchar(c2);
            putchar(c3);
            return 1;
        }
        if(bytes_per_sample == 4){
            unsigned char c1 = (*(fp) >> 24) & 0xff;
            unsigned char c2 = (*(fp) >> 16) & 0xff;
            unsigned char c3 = (*(fp) >> 8) & 0xff;
            unsigned char c4 = *(fp) & 0xff;
            putchar(c1);
            putchar(c2);
            putchar(c3);
            putchar(c4);
            return 1;
        }
    }

    if(channels == 2){
        if(bytes_per_sample == 1){
            unsigned char c = *(fp) & 0xff;
            putchar(c);
            fp++;
            c = *(fp) & 0xff;
            putchar(c);
            return 1;
        }
        if(bytes_per_sample == 2){
            unsigned char c1 = (*(fp) >> 8) & 0xFF;
            unsigned char c2 = *(fp) & 0xFF;
            putchar(c1);
            putchar(c2);
            fp++;
            c1 = (*(fp) >> 8) & 0xFF;
            c2 = *(fp) & 0xFF;
            putchar(c1);
            putchar(c2);
            return 1;
        }
        if(bytes_per_sample == 3){
            unsigned char c1 = (*(fp) >> 16) & 0xff;
            unsigned char c2 = (*(fp) >> 8) & 0xff;
            unsigned char c3 = *(fp) & 0xff;
            putchar(c1);
            putchar(c2);
            putchar(c3);
            fp++;
            c1 = (*(fp) >> 16) & 0xff;
            c2 = (*(fp) >> 8) & 0xff;
            c3 = *(fp) & 0xff;
            putchar(c1);
            putchar(c2);
            putchar(c3);
            return 1;
        }
        if(bytes_per_sample == 4){
            unsigned char c1 = (*(fp) >> 24) & 0xff;
            unsigned char c2 = (*(fp) >> 16) & 0xff;
            unsigned char c3 = (*(fp) >> 8) & 0xff;
            unsigned char c4 = *(fp) & 0xff;
            putchar(c1);
            putchar(c2);
            putchar(c3);
            putchar(c4);
            fp++;
            c1 = (*(fp) >> 24) & 0xff;
            c2 = (*(fp) >> 16) & 0xff;
            c3 = (*(fp) >> 8) & 0xff;
            c4 = *(fp) & 0xff;
            putchar(c1);
            putchar(c2);
            putchar(c3);
            putchar(c4);
            return 1;
        }
    }
    return 0;
}

int convInt(){
    return ((getchar()<<24) | (getchar()<<16) | (getchar()<<8) | getchar());
}

