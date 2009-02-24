#if !defined( DefaultRepositoryInfo_hh )

typedef struct {
  char * Value;             // NULL for none
  void * ValueNative;       // NULL for none
  chtype type;              // required if RestoreValueNative is given
  int count;                // required if RestoreValueNative is given
  } DefaultRepositoryInfo_ts;

#define DefaultRepositoryInfo_hh
#endif // guard
