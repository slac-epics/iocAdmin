%{

/**************************************************************************
 *
 *     Author:	Jim Kowalkowski
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 *      This software was produced under  U.S. Government contracts:
 *      (W-7405-ENG-36) at the Los Alamos National Laboratory,
 *      and (W-31-109-ENG-38) at Argonne National Laboratory.
 *
 *      Initial development by:
 *              The Controls and Automation Group (AT-8)
 *              Ground Test Accelerator
 *              Accelerator Technology Division
 *              Los Alamos National Laboratory
 *
 *      Co-developed with
 *              The Controls and Computing Group
 *              Accelerator Systems Division
 *              Advanced Photon Source
 *              Argonne National Laboratory
 *
 * Modification Log:
 * -----------------
 * .01	10-29-93	jbk	initial version
 *
 ***********************************************************************/

#include <stdio.h>
#include <string.h>

static int line_num;
static int yyerror();

/* one of these per report column */
struct fields
{
	char *name;
	int size;
};

extern char *optarg;
extern int optind;

#define DEFAULT_TOTAL 3
struct fields default_fields[] =
 { {"DESC",30},{"DTYP",30},{"OUT.INP",30},{NULL,0} };

struct fields* the_fields;
char** the_values;
int total_fields,type_len,name_len;
char form[2000];
char curr_rec[50];
char curr_type[50];

%}

%start database

%token <Str> COMMA
%token <Str> WORD VALUE 
%token <Str> FIELD
%left O_BRACE C_BRACE O_PAREN C_PAREN
%left DATABASE CONTAINER RECORD
%left NOWHERE 

%union
{
    int	Int;
	char Char;
	char *Str;
	double Real;
}

%%

database:	DATABASE d_head d_body
	| DATABASE d_head /* jbk added for graphical thing */
	| db_components
	;

d_head:	O_PAREN WORD C_PAREN
	{ free($2); }
	| O_PAREN WORD COMMA VALUE C_PAREN
	{ free($2); free($4); }
	;

d_body:	O_BRACE nowhere_records db_components C_BRACE
	;

/* nowhere is here for back compatability */

nowhere_records: /* null */
	| NOWHERE n_head n_body
	;

n_head: O_PAREN C_PAREN
	;

n_body: O_BRACE records C_BRACE
	;

db_components: /* null */
	| db_components container
	| db_components record
	;

container: CONTAINER c_head c_body
	;

c_head: O_PAREN WORD C_PAREN
	{ free($2); }
	;

c_body: O_BRACE db_components C_BRACE
	;

records: /* null */
	| records record
	;

record:	RECORD r_head r_body
	{
		int i;
		char temp[50];

		sprintf(temp,"%%-%ds%%-%ds",type_len,name_len);
		printf(temp,curr_type,curr_rec);
		/* printf("%-15s%-30s",curr_type,curr_rec); */
		for(i=0;i<total_fields;i++) printf("%s",the_values[i]);
		printf("\n");
	}
	;

r_head:	O_PAREN WORD COMMA WORD C_PAREN
	{
		int i;
		for(i=0;i<total_fields;i++)
		{
			memset(the_values[i],' ',the_fields[i].size);
			the_values[i][the_fields[i].size]='\0';
		}
		strcpy(curr_type,$2);
		strcpy(curr_rec,$4);
		free($2); free($4);
	}
	| O_PAREN WORD COMMA VALUE C_PAREN
	{
		int i;
		for(i=0;i<total_fields;i++)
		{
			memset(the_values[i],' ',the_fields[i].size);
			the_values[i][the_fields[i].size]='\0';
		}
		strcpy(curr_type,$2);
		strcpy(curr_rec,$4);
		free($2); free($4);
	}
	;

r_body:	/* null */
	| O_BRACE fields C_BRACE
	;

fields: /* null */
	| fields field
	;

field:	FIELD O_PAREN WORD COMMA VALUE C_PAREN
	{
		int i;
		char puken[50];
		char* pc;

		for(i=0;i<total_fields;i++)
		{
			strcpy(puken,the_fields[i].name);

			for(pc=strtok(puken,".");pc;pc=strtok(NULL,"."))
			{
				if( !strcmp(pc,$3) )
					strncpy(the_values[i],$5,strlen($5)<the_fields[i].size?strlen($5):the_fields[i].size-1);
			}
		}
		free($3); free($5);
	}
	;

%%
 
#include "dbreport_lex.c"
 
static int yyerror(str)
char  *str;
{ fprintf(stderr,"db file parse, Error line %d : %s\n",line_num, yytext); return 0; }

/*********************************************************************/
 
int main(int argc, char** argv)
{
	int i,c;
	int e=0;
	char* dbname = NULL;
	char* pc;
	char hold[20],thing[50];
	FILE* get_from_here = stdin;

	type_len=15;
	name_len=30;

	while(e==0 && (c=getopt(argc,argv,"f:t:n:"))!=-1)
	{
		switch(c)
		{
		case 'f': dbname = (char*)strdup(optarg); break; /* db name */
		case 't':
			/* type field length max */
			if(sscanf(optarg,"%d",&type_len)!=1)
			{
				fprintf(stderr,"illegal type field size --> %s\n",argv[optind]);
				return -1;
			}
			break;
		case 'n':
			if(sscanf(optarg,"%d",&name_len)!=1)
			{
				fprintf(stderr,"illegal name field size --> %s\n",argv[optind]);
				return -1;
			}
			break;
		default: e=1; break;
		}
	}

	if(e==1)
	{
		fprintf(stderr,
			"usage: %s [-t type_length] [-n name_length] [-f dbname.db] field.size field.size ...\n",argv[0]);
		fprintf(stderr,
			"\nWhere dbname.db is the name of a db file to report,\n");
		fprintf(stderr,
			"if no -f specified, then db file read from standard in.\n");
		fprintf(stderr,
			"\nfield.size is the actual name of the field to print\n");
		fprintf(stderr,
			"size is the max size allowed for the value in the report\n");
		fprintf(stderr,
			"you can have more than one field used in a line on the report\n");
		fprintf(stderr, "by specifying field.field.size (ex: OUT.INP.23 )\n");
		fprintf(stderr,
			"\nname_length is max length of the name field in the report\n");
		fprintf(stderr,
			"\ntype_length is max length of the type field in the report\n");
		return -1;
	}

	if(dbname)
	{
		if((get_from_here=fopen(dbname,"r"))==NULL)
		{
			fprintf(stderr,"cannot open db file %s\n",dbname);
			return -1;
		}
	}

	if(optind<argc)
	{
		/* the user specified fields (argc-optind) */
		the_fields=
			(struct fields*)malloc(1+sizeof(struct fields)*(argc-optind));
		the_values= (char**)malloc(sizeof(char*)*(argc-optind));

		for(i=0;optind<argc;i++,optind++)
		{
			strcpy(hold,argv[optind]);
			if((pc=strrchr(hold,'.'))==NULL)
			{
				fprintf(stderr,"illegal field --> %s\n",argv[optind]);
				return -1;
			}

			*pc++='\0';
			the_fields[i].name=strdup(hold);

			if(sscanf(pc,"%d",&the_fields[i].size)!=1)
			{
				fprintf(stderr,"illegal field size --> %s\n",argv[optind]);
				return -1;
			}
			the_values[i]=(char*)malloc(the_fields[i].size+1);
		}
		the_fields[i].name=NULL;
		the_fields[i].size=0;
		total_fields=i;
	}
	else
	{
		/* the user wants the default report */
		the_fields=default_fields;
		the_values=(char**)malloc(sizeof(char*)*DEFAULT_TOTAL);
		for(i=0;i<DEFAULT_TOTAL;i++)
		{
			the_values[i]=(char*)malloc(the_fields[i].size+1);
		}
		total_fields=DEFAULT_TOTAL;
	}

	sprintf(thing,"%%-%ds%%-%ds",type_len,name_len);
	printf(thing,"TYPE","NAME"); /* printf("%-15s%-30s","TYPE","NAME"); */

	for(i=0;i<total_fields;i++)
	{
		/* printf("%s:%d\n",the_fields[i].name,the_fields[i].size); */
		memset(the_values[i],' ',the_fields[i].size);
		the_values[i][the_fields[i].size]='\0';
		strncpy(the_values[i],the_fields[i].name,strlen(the_fields[i].name));
		printf("%s",the_values[i]);
	}
	printf("\n\n");

	yyin=get_from_here;
	yyparse();

	fclose(get_from_here);

	return 0;
}
