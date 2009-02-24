// Example EPICS CA server

#include "exServer.h"
#include "envDefs.h"

#include <iostream>
#include <fstream>

using namespace std;

casParser::casParser(const char* fl, const char* fl_out)
{
  name_fl = fl;
  name_fl_out = fl_out;  
  numberOfPVs = 0;
  //define number of PVs and define EPICS variables
  numbOfPVsDef();
}

casParser::~casParser()
{
  if(numberOfPVs != 0 ){
    for(int i = 0; i < numberOfPVs; i++){
      delete [] namesOfPVs[i];
    }
    delete [] namesOfPVs;
  }
}

void casParser::numbOfPVsDef()
{
  int i=0;
  
  char line[MAX_LENGTH_STRING+1];
  char* ln;
  char* ln_r;

  ifstream filePVin(name_fl);
  if(!filePVin){
     std::cerr << "Parser: Cannot open " << name_fl << " \n";
     std::cerr << "Stop. \n";
     exit(1);
  }

  while(!filePVin.eof()){
    filePVin.getline(line,MAX_LENGTH_STRING);
    if(line != NULL ) {
      ln = strtok(line," =\n\r\0");
      ln_r = strtok(NULL,"");
      if( ln != NULL ) {
        if( strncmp(ln,"name",4) == 0 ){ 
          i++;
	}
	else{
	  if(!(strncmp(ln,"#",1) == 0)){
	    if( strncmp(ln,"EPICS_CA_AUTO_ADDR_LIST",MAX_LENGTH_STRING) == 0 ){ 
	      epicsEnvSet("EPICS_CA_AUTO_ADDR_LIST"  ,ln_r);
	    }
	    if( strncmp(ln,"EPICS_CA_ADDR_LIST",     MAX_LENGTH_STRING) == 0 ){
	      epicsEnvSet("EPICS_CA_ADDR_LIST"       ,ln_r);
	    }
	    if( strncmp(ln,"EPICS_CA_SERVER_PORT",   MAX_LENGTH_STRING) == 0 ){
	      epicsEnvSet("EPICS_CA_SERVER_PORT"     ,ln_r);
	    }
	  }
	}
      }
    }
  }

  filePVin.close();
  numberOfPVs = i;

}

int casParser::getNumbOfPVs()
{
  return numberOfPVs;
}

void casParser::createPVs(exServer *pCAS)
{  
  int i=0;
  int numPVs = getNumbOfPVs();
  if( numPVs == 0 ) return;
  namesOfPVs = new char*[numPVs];
  pCAS->setNumbOfPV(numPVs);
  for(i=0; i< numPVs; i++){
    namesOfPVs[i]= new char[MAX_LENGTH_PV_NAME];
  }
    
  char line[MAX_LENGTH_STRING+1];
  char* ln;
  char* ln_r;


  double d;
  aitFloat64 errLev;
  aitFloat64 uppCtrlLimit;
  aitFloat64 lowCtrlLimit;
  unsigned i_size;
  double scanPeriodIn;
  char chName[MAX_LENGTH_PV_NAME+1];

  ifstream filePVin(name_fl);
  if(!filePVin){
     std::cerr << "Parser: Cannot open " << name_fl << " \n";
     std::cerr << "Stop. \n";
     exit(1);
  }

  ofstream filePVout;
  if( name_fl_out != "" ){
    filePVout.open(name_fl_out);
  }

  i=0;
  while(!filePVin.eof()){
    filePVin.getline(line,MAX_LENGTH_STRING);
    if(line != NULL ) {
      ln = strtok(line," =\n\r\0");
      ln_r = strtok(NULL," =\n\r\0");
      if( ln != NULL ) {
         if( strncmp(ln,"name",4) == 0 ){
	  errLev  = 0.;
          uppCtrlLimit =   1.0e+32;
          lowCtrlLimit = - 1.0e+32;
          i_size = 1u;
          scanPeriodIn = 2.0;
          strncpy(chName,ln_r,MAX_LENGTH_PV_NAME);
          ln = strtok(NULL," =\n\r\0");
          ln_r = strtok(NULL," =\n\r\0");
            while(ln){
              //std::cerr <<"   debug ln="<<ln<<" ln_r="<<ln_r<<"\n";
              if(strncmp(ln,"uppCtrlLim",6) == 0 && ln_r != NULL){
                if (sscanf(ln_r, "%lf", &d)==1) {
                 uppCtrlLimit = d;
                } 
	      }
              if(strncmp(ln,"lowCtrlLim",6) == 0 && ln_r != NULL){
                if (sscanf(ln_r, "%lf", &d)==1) {
                 lowCtrlLimit = d;
                } 
	      }
              if(strncmp(ln,"size",4) == 0 && ln_r != NULL){
                if (sscanf(ln_r, "%lf", &d)==1) {
                 i_size = (unsigned) d;
                } 
	      }
              if(strncmp(ln,"scanPeriod",4) == 0 && ln_r != NULL){
                if (sscanf(ln_r, "%lf", &d)==1) {
                 scanPeriodIn = d;
                } 
	      }
              if(strncmp(ln,"errLev",4) == 0 && ln_r != NULL){
                if (sscanf(ln_r, "%lf", &d)==1) {
                 errLev = d;
                } 
	      }
  
              ln = strtok(NULL," =\n\r\0");
              ln_r = strtok(NULL," =\n\r\0");
	    }
            strncpy(namesOfPVs[i],chName,MAX_LENGTH_PV_NAME);
            pCAS->addPV(scanPeriodIn,namesOfPVs[i],errLev,uppCtrlLimit,lowCtrlLimit,excasIoSync,i_size);
            if( name_fl != "" ){
	      filePVout << "PV="<< namesOfPVs[i];
              filePVout << "\terrLev=" << errLev;
              filePVout << "\tlimits="<< lowCtrlLimit <<"\t"<<uppCtrlLimit;
              filePVout << "\tsize=" << i_size <<"\tscanPeriod="<< scanPeriodIn <<"\n";
            }
            i++;
	 }

      }
    }    
  }

  if( name_fl_out != "" ){
    filePVout.close();
  }
  filePVin.close();
}
