#ifndef PRULOGICANALYZER_H_
#define PRULOGICANALYZER_H_


int logicAnalyzer();
volatile void* getPruMmapAddr(void);
void freePruMmapAddr(volatile void* pPruBase);
void generateBoolArr();
void trimWhiteSpace(char * str);
void* getNext8();



#endif

