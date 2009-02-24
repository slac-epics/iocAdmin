// ==============================================================================
//
//  Abs:  Function prototypes for printing out real numbers
//
//  Name: format_real.hh
//
//  Rem:  Prototypes for printing out real numbers.  Instead of relying in 
//        database developers to actually properly setup the PREC field of
//        their process variable use a rather generous precision.  Relying 
//        on the PREC field in the past has caused me much grief when people
//        actually BLAME me for not `correctly' saving their channel's value.
//
//  Auth: 25-Jul-2002, Mike Zelazny (zelazny@slac.stanford.edu)
//  Rev:  
//
// --------------------------------------------------------------------------------
//
//  Mod: (newest to oldest):
//
// ==============================================================================
//
#if !defined( format_real_hh )

int format_real (dbr_double_t , char * , size_t );
int format_real (dbr_float_t , char * , size_t );

#define format_real_hh
#endif // guard
