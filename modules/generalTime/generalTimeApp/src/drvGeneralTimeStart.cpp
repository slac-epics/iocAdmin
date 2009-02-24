/* This is a temporary file to init generalTime during loading munch file */
/* Eventually we don't need this file because we will rename generalTime_Init to iocClockInit */
extern "C" {
long generalTime_Init();
}

/* this is to automatically call generalTime_Init when load munch file */
/* change the unused flag to global is for reference from another file */
/* to guarantee linker will include this */
int	generalTime_Init_Flag = generalTime_Init();

