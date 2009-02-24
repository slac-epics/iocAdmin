#include <cstdio>
#include <cstdlib>
#include <cadef.h> /* EPICS specific */

#define USAGE "usage: writePV <PV name> <value1> [<value2> ...]"

void an_event_handler(evargs);

int main(int argc, char **argv)
{
  if(argc < 2){
                  printf("%s\n", USAGE);
                  exit(-1);
  }

  printf("=========Start Program========== \n");

  int  status;
  chid chan;
  double* value_f = (double *) malloc ( sizeof(double)*(argc-2));
  dbr_double_t* value;

  float tmp_f;

  for(int i = 0; i < (argc-2); i++){
     sscanf(argv[i+2],"%f",&tmp_f);
     value_f[i] = (double) tmp_f;
  }

  value = (dbr_double_t*) value_f;

  printf("debug value =");
  for(int i = 0; i < (argc-2); i++){
    printf(" %f ",value_f[i]); 
  }
  printf("\n");

          /* 2. Initialize Channel Access. */
          status = ca_task_initialize();
          SEVCHK(status, "Unable to initialize");
          if (status != ECA_NORMAL)
                  exit(-1);

          /* 3. Make search request. */
          status = ca_search(argv[1], &chan);
          if (status != ECA_NORMAL){
                  printf("%s: problem establishing connection to %s.", ca_message(status), argv[1]);
                  exit(-1);
          }

          /* 4. Send the request and wait for channel to be found. */
          status = ca_pend_io(1.0);
          if (status != ECA_NORMAL){
                  printf("%s: Not Found\n", argv[1]);
                  exit(-1);
          }

          printf("description of PV  \n");
          printf("name: %s \n",ca_name(chan));
          printf("type(%d) \n",ca_field_type(chan));
          printf("nelements(%d)\n",(int) ca_element_count(chan));
          printf("host(%s)  \n",ca_host_name(chan));
          printf("read(%d)  \n",ca_read_access(chan));
          printf("write(%d) \n",ca_write_access(chan));
          printf("state(%d) \n",ca_state(chan) );

	  int n_out_elem = (int) ca_element_count(chan);
	  double* value_f_out = (double *) malloc ( sizeof(double)*(n_out_elem));
	  dbr_double_t* value_out;
	  value_out = (dbr_double_t*) value_f_out;

	  int n_in_elem = (int) ca_element_count(chan);
	  double* value_f_in = (double *) malloc ( sizeof(double)*(n_in_elem));
	  dbr_double_t* value_in;
	  value_in = (dbr_double_t*) value_f_in;

          status = ca_array_get( DBR_DOUBLE, n_out_elem, chan, value_in);
          ca_pend_event(0.5);

	  printf("debug before value =");
	  for(int i = 0; i < n_in_elem; i++){
	    printf(" %f ",(double) value_in[i]); 
	  }
	  printf("\n");

          status = ca_array_put_callback( DBR_DOUBLE, (argc-2), chan , value, an_event_handler, NULL);
          ca_pend_event(0.5);

          status = ca_array_get( DBR_DOUBLE, n_out_elem, chan, value_out);
          ca_pend_event(0.5);


	  printf("debug after value  =");
	  for(int i = 0; i < n_out_elem; i++){
	    printf(" %f ",(double) value_out[i]); 
	  }
	  printf("\n");

	  int i_return = 0;
	  return i_return;
  }


void an_event_handler(struct event_handler_args handler_args)
   {
     
           if (handler_args.status != ECA_NORMAL){
                   printf("channel %s: put operation not sucessful.\n",
                                   ca_name(handler_args.chid));
                   SEVCHK(handler_args.status, NULL);
           }
           else
	     //printf("channel %s: put (or get) operation completed.\n",
             //                      ca_name(handler_args.chid)); 
           return;
   }
