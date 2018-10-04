
/*
 * Allocate storage for the various data structures
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "global.h"
#include "gradedb.h"
#include "stats.h"
#include "allocate.h"
#include "error.h"

Professor *newprofessor()
{
        Professor *p;
        if((p = (Professor *)malloc(sizeof(Professor))) == NULL)
                fatal(memerr);
        return(p);
}

Assistant *newassistant()
{
        Assistant *a;
        if((a = (Assistant *)malloc(sizeof(Assistant))) == NULL)
                fatal(memerr);
        return(a);
}

Student *newstudent()
{
        Student *s;
        if((s = (Student *)malloc(sizeof(Student))) == NULL)
                fatal(memerr);
        return(s);
}

Section *newsection()
{
        Section *s;
        if((s = (Section *)malloc(sizeof(Section))) == NULL)
                fatal(memerr);
        return(s);
}

Assignment *newassignment()
{
        Assignment *a;
        if((a = (Assignment *)malloc(sizeof(Assignment))) == NULL)
                fatal(memerr);
        return(a);
}

Course *newcourse()
{
        Course *c;
        if((c = (Course *)malloc(sizeof(Course))) == NULL)
                fatal(memerr);
        return(c);
}

Score *newscore()
{
        Score *s;
        if((s = (Score *)malloc(sizeof(Score))) == NULL)
                fatal(memerr);
        return(s);
}
int sCount = 0;

char *newstring(tp, size)
char *tp;
int size;
{
        char *s, *cp;
        if((s = (char *)malloc(size)) == NULL)
                fatal(memerr);
        if((sCount)%32 == 0){
               sList = sList-sCount;
               sList = (char **)realloc(sList, sizeof(char*)*(sCount+32));
               sList = sList + sCount;
        }
        cp = s;
        while(size-- > 0) *cp++ = *tp++;
        *(sList) = s;
        sList++;
        sCount++;
        return(s);
}

void free_strings(){
        sList = sList - sCount;
        int ct = 0;
        while(ct != sCount){
                free(*(sList));
                sList++;
                ct++;
        }
        sList = sList-sCount;
        free(sList);
}

int fCount = 0;

Freqs *newfreqs()
{
        Freqs *f;
        if((f = (Freqs *)malloc(sizeof(Freqs))) == NULL)
                fatal(memerr);
        if((fCount)%32 == 0){
               fList = fList-fCount;
               fList = (Freqs **)realloc(fList, sizeof(Freqs*)*(fCount+32));
               fList = fList + fCount;
        }
        *(fList) = f;
        fList++;
        fCount++;
        return(f);
}

void initializeList(){
        fList = (Freqs **)malloc(sizeof(Freqs*) * 32);
}
void initializeSList(){
        sList = (char **)malloc(sizeof(char*) * 32);
}

void free_freqs(){
        fList = fList - fCount;
        int ct = 0;
        while(ct != fCount){
                free(*(fList));
                fList++;
                ct++;
        }
        fList = fList-fCount;
        free(fList);
}

Classstats *newclassstats()
{
        Classstats *c;
        if((c = (Classstats *)malloc(sizeof(Classstats))) == NULL)
                fatal(memerr);
        return(c);

}

Sectionstats *newsectionstats()
{
        Sectionstats *s;
        if((s = (Sectionstats *)malloc(sizeof(Sectionstats))) == NULL)
                fatal(memerr);
        return(s);

}

Stats *newstats()
{
        Stats *s;
        if((s = (Stats *)malloc(sizeof(Stats))) == NULL)
                fatal(memerr);
        return(s);
}

Ifile *newifile()
{
        Ifile *f;
        if((f = (Ifile *)malloc(sizeof(Ifile))) == NULL)
                fatal(memerr);
        return(f);
}
