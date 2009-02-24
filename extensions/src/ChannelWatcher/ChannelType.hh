// ==============================================================================
//
//  Abs:  Converts Channel Access data type to a string.
//
//  Name: ChannelType.hh
//
//  Rem:  Prototypes for ChannelType functions to covert a Channel Access data 
//        type from db_Access.h to a human readable string.
//
//  Auth: 30-Sep-2002, Mike Zelazny (zelazny)
//  Rev:
//
// ------------------------------------------------------------------------------
//
//  Mod:  (newest to oldest):
//
// ==============================================================================
//

#if !defined( ChannelType_hh )

char * ChannelType (chtype type);
chtype ChannelType (char * type);

#define ChannelType_hh
#endif // guard
