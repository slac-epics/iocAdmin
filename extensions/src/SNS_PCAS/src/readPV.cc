#include <cstring>
#include <cstdio>

#include "cadef.h"
#include "epicsTime.h"
#include "dbDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

 void print_info(chtype type, const void *pbuffer, unsigned count);

 void fetchIt(struct event_handler_args args);

#ifdef __cplusplus
}
#endif

int main(int argc, char **argv)
{
  //print error and return if arguments are invalid
  if(argc !=2 ){
    printf("usage: %s <PV name> \n", argv[0]);
    printf("the following arguments were received\n");
      while(argc>0) {
	  printf("%s\n",argv[0]);
	  argv++; argc--;
      }
    return -1;
  }

  chid chan_id;
  int status;
  int i;
  status = ca_search(argv[1], &chan_id);
  SEVCHK(status,NULL);
  status = ca_pend_io(5.0);
  if(status != ECA_NORMAL){
    SEVCHK(ca_clear_channel(chan_id),NULL);
    printf("Not Found %s\n", argv[1]);
    return -1;
  }

  printf("==========START reading===============\n");
  printf("name:\t%s\n", ca_name(chan_id));
  printf("native type:\t%d\n", ca_field_type(chan_id));
  printf("native count:\t%lu\n", ca_element_count(chan_id));

  i = DBR_DOUBLE;
  status = ca_array_get_callback(
	   i, 
	   ca_element_count(chan_id),
	   chan_id, 
	   fetchIt, 
	   NULL);
  SEVCHK(status, NULL);


  i = DBR_TIME_DOUBLE;
  status = ca_array_get_callback(
	   i, 
	   ca_element_count(chan_id),
	   chan_id, 
	   fetchIt, 
	   NULL);
  SEVCHK(status, NULL);


  i = DBR_CTRL_DOUBLE;
  status = ca_array_get_callback(
	   i, 
	   ca_element_count(chan_id),
	   chan_id, 
	   fetchIt, 
	   NULL);
  SEVCHK(status, NULL);




  //wait for the operation to complete before returning 
  ca_pend_event (5.0);
  SEVCHK(ca_clear_channel(chan_id),NULL);
  printf("=======STOP=========\n");
  return 0;

}

//====================================================================
//The function 
//====================================================================
void fetchIt(struct event_handler_args args)
{
  if (args.status == ECA_NORMAL) {
    print_info(args.type,args.dbr,args.count);
  }
  else {
    printf ("%s: err resp to get cb was \"%s\"\n",
    __FILE__, ca_message(args.status));
  }
}

//====================================================================
//The function prints all information about channel
//====================================================================
void print_info(chtype type, const void *pbuffer, unsigned count)
{
  int i;
  char tsString[50];
  printf("============================================================\n");
  int type_pr = (int) type;
  printf("type_id= %d \t type= %s \t count = %d \n",type_pr, dbr_text[type], count);

  if(type == DBR_DOUBLE){
    dbr_double_t *pvalue = (dbr_double_t *) pbuffer;
     if(count==1) printf(" Value: ");     
     for (i = 0; i < (int) count; i++,pvalue++){
       if(count!=1 && (i%5 == 0)) printf("\n ");
       printf(" %12.7f ",(double)(*pvalue));
     }
    printf("\n"); 
  }


  if(type == DBR_TIME_DOUBLE){
     struct dbr_time_double *pvalue = (struct dbr_time_double *)pbuffer;
     dbr_double_t *pdouble = &pvalue->value;
     epicsTimeToStrftime(tsString,sizeof(tsString),
                         "%Y/%m/%d %H:%M:%S.%06f",&pvalue->stamp);
     printf("status=%2d severity=%2d",pvalue->status,pvalue->severity);
     printf("\tTimeStamp: %s \n",tsString);
     if(count==1) printf(" Value: ");
       for (i = 0; i < (int) count; i++,pdouble++){
	 if(count!=1 && (i%5 == 0)) printf("\n ");
	 printf(" %12.7f ",(double)(*pdouble));
       }
    printf("\n");
  } 

  if(type == DBR_CTRL_DOUBLE){
     struct dbr_ctrl_double *pvalue = (struct dbr_ctrl_double *)pbuffer;
     dbr_double_t *pdouble = &pvalue->value;
     printf(" status = %2d \n severity=%2d \n units= %.8s \n",pvalue->status,pvalue->severity,
			pvalue->units);
     printf(" precision          = %3d \n",pvalue->precision);
     printf(" upper_disp_limit   = %12.5f \n",(float)(pvalue->upper_disp_limit));
     printf(" lower_disp_limit   = %12.5f \n",(float)(pvalue->lower_disp_limit));
     printf(" upper_alarm_limit  = %12.5f \n",(float)(pvalue->upper_alarm_limit));
     printf(" lower_alarm_limit  = %12.5f \n",(float)(pvalue->lower_alarm_limit));
     printf(" upper_warning_limit= %12.5f \n",(float)(pvalue->upper_warning_limit));
     printf(" lower_warning_limit= %12.5f \n",(float)(pvalue->lower_warning_limit));
     printf(" upper_ctrl_limit   = %12.5f \n",(float)(pvalue->upper_ctrl_limit));
     printf(" lower_ctrl_limit   = %12.5f \n",(float)(pvalue->lower_ctrl_limit));
     if(count==1) printf(" Value: ");
      for (i = 0; i < (int) count; i++,pdouble++){
	 if(count!=1 && (i%5 == 0)) printf("\n ");
	 printf(" %12.7f ",(double)(*pdouble));
      }
    printf("\n");
  }


 
}
