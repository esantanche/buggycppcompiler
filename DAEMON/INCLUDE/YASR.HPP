//
// Yet Another String Replacer
// This object allows for loading a file and replacing some fields with values
// provided by the programmer. The user has two ways to define the fields to be 
// replaced: in the constructor specify a file containing a set of pairs 
// (fieldname: fieldvalue) or using the method replaceField(char *fieldName, char *fieldValue)
// In the original file the field that have to be replaced have to be surrounded by
// a '$' pair.
//
// Example: This is a line of my maskFile containing the field $Town.......
//
// the user might use replaceField("Town", "Atlanta");
// or construct the MMBYasr object with a file containing the line Town: Atlanta.
//
// At the moment, ':' is a reserved char and cannot be used in second case (I just
// consider the first occurrence of ':' on each line.
//

#ifndef YASR_HPP
#define YASR_HPP 

#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>

#ifdef OS2_ENVIRONMENT
#include <sys\types.h>
#include <sys\stat.h>
#endif

#ifdef AIX_ENVIRONMENT
#include <sys/stat.h>
#endif


class FilePair {

    char  *line;
    char  *fName, *fValue;
    FILE  *fp;
    int   status;
 
    void loadLine();

   public:

    enum FilePairFlags   { ERROR=001, FEOF_FLAG=002, BADFORMAT_FLAG=004, 
                           FERR_FLAG=010, FOUND_FLAG=020 };

    FilePair(char *fNameStringPairs);
    ~FilePair();
 
    char *getfName()        { return fName; }
    char *getfValue()       { return fValue; }

    void nextRecord();
    void firstRecord();
    void lookUp(char *name);

    int  isFail()           { return status & ERROR; }
    int  isFeof()           { return status & FEOF_FLAG; }
    int  isFerror()         { return status & FERR_FLAG; }
    int  found()            { return status & FOUND_FLAG; }
    int  isBadFormat()      { return status & BADFORMAT_FLAG; }

};


class Yasr {

    char *templateData;   // This contains the data of the file that has
                          // to be processed. 
    int   status;
 
    int   templateLen;
    int   templateDataAlloc;

    void  reallocNewData(int quantity);

   public:

    enum YasrFlags   { ERROR=001, TEMPLATE_LOADED=002 };

    Yasr();
    ~Yasr();

    char *getData()         { *(templateData+templateLen)='\0'; return templateData; }
    int  isFail()           { return status & ERROR; }
    int  isLoaded()         { return status & TEMPLATE_LOADED; }

    void loadTemplateDataFromFile(char *fNameMaskFile);
    void saveDataToFile(char *fNameOutputFile);

    void replaceField(char *fieldName, char *fieldValue);
    void replaceField(char *fNameStringPairs);
};


#endif

